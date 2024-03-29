#include "extras/test_tools/json_decoder.h"

#include <stddef.h>

#include <charconv>
#include <cmath>
#include <cstdint>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>  // NOLINT
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/charconv.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include "mcucore_platform.h"
#include "util/task/status_macros.h"

namespace mcucore {
namespace test {
namespace {

std::string CharToStr(char c) { return std::string(1, c); }

std::string CharToQuotedStr(char c) { return '\'' + std::string(1, c) + '\''; }

// Remove leading whitespace, i.e. at the start of str.
void SkipLeadingWhitespace(std::string_view& str) {
  auto pos = str.find_first_not_of(" \t\r\n");
  if (pos == std::string_view::npos) {
    str.remove_prefix(str.size());
  } else {
    str.remove_prefix(pos);
  }
}

// Remove leading whitespace, i.e. at the start of str. By interior we mean that
// we're parsing something that may start with whitespace, but after that there
// should be some character, not the end of the string. Returns an error if str
// is empty after removing leading whitespace.
absl::Status SkipInteriorWhitespace(std::string_view& str) {
  SkipLeadingWhitespace(str);
  if (str.empty()) {
    return absl::InvalidArgumentError("Expected a value, not end-of-input.");
  } else {
    return absl::OkStatus();
  }
}

// Skip leading whitespace, confirm that the next character is the specified
// delimiter (e.g. a comma), and if so skip it and return OK. If unable to
// confirm that, returns an error.
absl::Status SkipRequiredDelimiter(std::string_view& str,
                                   const char delimiter) {
  RETURN_IF_ERROR(SkipInteriorWhitespace(str))
      << "Expected delimiter " << CharToQuotedStr(delimiter)
      << ", not end-of-input";
  if (str[0] != delimiter) {
    return absl::InvalidArgumentError(
        absl::StrCat("Expected delimiter ", CharToQuotedStr(delimiter),
                     ", not ", CharToQuotedStr(str[0])));
  }
  str.remove_prefix(1);
  return absl::OkStatus();
}

// Skip leading whitespace, confirm that the next characters after that match
// word, and if so skip word and return OK. If unable to confirm that, returns
// an error.
absl::Status SkipRequiredWord(std::string_view& str,
                              const std::string_view word) {
  RETURN_IF_ERROR(SkipInteriorWhitespace(str)) << "; expected " << word;
  if (!absl::StartsWith(str, word)) {
    return absl::InvalidArgumentError(
        absl::StrCat("Expected ", word, ", not '", str, "'"));
  }
  str.remove_prefix(word.size());
  return absl::OkStatus();
}

// Forward declaration so that we can have recursion.
absl::StatusOr<JsonValue> ParseValue(std::string_view& str);

absl::StatusOr<JsonValue> ParseString(std::string_view& str) {
  RETURN_IF_ERROR(SkipRequiredDelimiter(str, '"'));
  std::string result;
  while (!str.empty()) {
    char c = str[0];
    str.remove_prefix(1);
    if (!(' ' <= c && c < 127)) {
      return absl::InvalidArgumentError(
          absl::StrCat("Not a valid string char: ", CharToQuotedStr(c)));
    } else if (c == '"') {
      // End of string.
      return JsonValue(result);
    } else if (c == '\\') {
      if (str.empty()) {
        return absl::InvalidArgumentError(
            "Expected escaped character, not end-of-input");
      }
      c = str[0];
      str.remove_prefix(1);
      switch (c) {
        case '"':
          result.append(1, '"');
          break;
        case '\\':
          result.append(1, '\\');
          break;
        case '/':
          result.append(1, '/');
          break;
        case 'b':
          result.append(1, '\b');
          break;
        case 'f':
          result.append(1, '\f');
          break;
        case 'n':
          result.append(1, '\n');
          break;
        case 'r':
          result.append(1, '\r');
          break;
        case 't':
          result.append(1, '\t');
          break;
        case 'u':
          return absl::InvalidArgumentError(absl::StrCat(
              "Decoding of unicode code points not supported: \\u", str));
        default:
          return absl::InvalidArgumentError(
              absl::StrCat("Not a valid string escape: \\", CharToStr(c), str));
      }
    } else {
      result.append(1, c);
    }
  }
  return absl::InvalidArgumentError("Expected end of string, not end-of-input");
}

absl::StatusOr<JsonValue> ParseNull(std::string_view& str) {
  RETURN_IF_ERROR(SkipRequiredWord(str, "null"));
  return JsonValue(nullptr);
}

absl::StatusOr<JsonValue> ParseTrue(std::string_view& str) {
  RETURN_IF_ERROR(SkipRequiredWord(str, "true"));
  return JsonValue(true);
}

absl::StatusOr<JsonValue> ParseFalse(std::string_view& str) {
  RETURN_IF_ERROR(SkipRequiredWord(str, "false"));
  return JsonValue(false);
}

absl::StatusOr<JsonValue> ParseNumber(std::string_view& str) {
  RETURN_IF_ERROR(SkipInteriorWhitespace(str)) << "; expected number";

  // First decode as a floating-point number.
  double value;
  auto fc_result = absl::from_chars(str.data(), str.data() + str.size(), value);
  if (fc_result.ec != std::errc()) {
    return absl::InvalidArgumentError("Expected a number");
  }
  CHECK_GT(fc_result.ptr, str.data());
  auto size = fc_result.ptr - str.data();
  CHECK_LE(size, str.size());
  JsonValue result(value);

  // Just in case it is an integer (in the Alpaca setting, this is actually the
  // most common case), try decoding the exact same string as an integer.
  std::string_view num_str = str.substr(0, size);
  if (num_str.find_first_of(".eE") == std::string_view::npos) {
    // Looks to be an integer.
    int64_t integer;
    auto integer_fc_result =
        std::from_chars(str.data(), str.data() + str.size(), integer);
    if (integer_fc_result.ec == std::errc() &&
        integer_fc_result.ptr == fc_result.ptr) {
      // Yup, can be decoded as an integer.
      result = JsonValue(integer);
    }
  }

  str.remove_prefix(size);
  return result;
}

absl::StatusOr<JsonValue> ParseObject(std::string_view& str) {
  RETURN_IF_ERROR(SkipRequiredDelimiter(str, '{'));
  std::shared_ptr<JsonObject> object = std::make_shared<JsonObject>();
  RETURN_IF_ERROR(SkipInteriorWhitespace(str));
  if (str[0] != '}') {
    while (true) {
      ASSIGN_OR_RETURN(auto name, ParseString(str));
      RETURN_IF_ERROR(SkipRequiredDelimiter(str, ':'));
      ASSIGN_OR_RETURN(auto value, ParseValue(str));
      object->insert_or_assign(name.as_string(), value);
      RETURN_IF_ERROR(SkipInteriorWhitespace(str));
      if (str[0] == '}') {
        break;
      }
      RETURN_IF_ERROR(SkipRequiredDelimiter(str, ','));
    }
  }
  RETURN_IF_ERROR(SkipRequiredDelimiter(str, '}'));
  return JsonValue(object);
}

absl::StatusOr<JsonValue> ParseArray(std::string_view& str) {
  RETURN_IF_ERROR(SkipRequiredDelimiter(str, '['));
  std::shared_ptr<JsonArray> array = std::make_shared<JsonArray>();
  RETURN_IF_ERROR(SkipInteriorWhitespace(str));
  if (str[0] != ']') {
    while (true) {
      ASSIGN_OR_RETURN(auto value, ParseValue(str));
      array->push_back(value);
      RETURN_IF_ERROR(SkipInteriorWhitespace(str));
      if (str[0] == ']') {
        break;
      }
      RETURN_IF_ERROR(SkipRequiredDelimiter(str, ','));
    }
  }
  RETURN_IF_ERROR(SkipRequiredDelimiter(str, ']'));
  return JsonValue(array);
}

absl::StatusOr<JsonValue> ParseValue(std::string_view& str) {
  RETURN_IF_ERROR(SkipInteriorWhitespace(str));
  const char c = str[0];
  if (c == '[') {
    return ParseArray(str);
  } else if (c == '{') {
    return ParseObject(str);
  } else if (c == '"') {
    return ParseString(str);
  } else if (c == 'n') {
    return ParseNull(str);
  } else if (c == 't') {
    return ParseTrue(str);
  } else if (c == 'f') {
    return ParseFalse(str);
  } else {
    return ParseNumber(str);
  }
}

}  // namespace

bool JsonObject::HasKey(const std::string& key) const {
  return find(key) != end();
}

JsonValue JsonObject::GetValue(const std::string& key) const {
  auto iter = find(key);
  if (iter == end()) {
    return JsonValue();
  } else {
    return iter->second;
  }
}

JsonValue::JsonValue() : value_(Undefined{}) {}
JsonValue::JsonValue(nullptr_t) : value_(nullptr) {}
JsonValue::JsonValue(bool v) : value_(v) {}
JsonValue::JsonValue(int64_t v) : value_(v) {}
JsonValue::JsonValue(int v) : value_(static_cast<int64_t>(v)) {}
JsonValue::JsonValue(double v) : value_(v) {}
JsonValue::JsonValue(const std::string& v) : value_(v) {}
JsonValue::JsonValue(std::string_view v) : value_(std::string(v)) {}
JsonValue::JsonValue(const char* v) : value_(std::string(v)) {}
JsonValue::JsonValue(std::shared_ptr<JsonObject> v) : value_(v) {}
JsonValue::JsonValue(JsonObject v) : value_(std::make_shared<JsonObject>(v)) {}
JsonValue::JsonValue(std::shared_ptr<JsonArray> v) : value_(v) {}
JsonValue::JsonValue(JsonArray v) : value_(std::make_shared<JsonArray>(v)) {}

absl::StatusOr<JsonValue> JsonValue::Parse(std::string_view str) {
  auto full_str = str;
  ASSIGN_OR_RETURN(auto result, ParseValue(str));
  // str should be empty except for trailing whitespace.
  SkipLeadingWhitespace(str);
  if (str.empty()) {
    return result;
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Expected only trailing whitespace at offset ",
                   str.data() - full_str.data()));
}

JsonValue::EType JsonValue::type() const {
  return static_cast<EType>(value_.index());
}

bool JsonValue::as_bool() const { return std::get<kBool>(value_); }

int64_t JsonValue::as_integer() const { return std::get<kInteger>(value_); }

double JsonValue::as_double() const { return std::get<kDouble>(value_); }

const std::string& JsonValue::as_string() const {
  return std::get<kString>(value_);
}

const JsonObject& JsonValue::as_object() const {
  return *std::get<kObject>(value_);
}

const JsonArray& JsonValue::as_array() const {
  return *std::get<kArray>(value_);
}

bool JsonValue::HasKey(const std::string& key) const {
  return type() == kObject && as_object().HasKey(key);
}

JsonValue JsonValue::GetValue(const std::string& key) const {
  if (type() == kObject) {
    return as_object().GetValue(key);
  }
  return JsonValue();
}

bool JsonValue::HasIndex(size_t index) const {
  return type() == kArray && as_array().size() > index;
}

JsonValue JsonValue::GetElement(size_t index) const {
  if (type() == kArray && as_array().size() > index) {
    return as_array()[index];
  }
  return JsonValue();
}

absl::StatusOr<JsonValue> JsonValue::GetValueOfType(const std::string& key,
                                                    EType required_type) const {
  // TODO(jamessynge): Add ToString for EType.
  if (!is_object()) {
    return absl::FailedPreconditionError(
        absl::StrCat("Expected an object, but is: ", ToDebugString()));
  }
  if (!HasKey(key)) {
    return absl::FailedPreconditionError(
        absl::StrCat("Object does not have key '", key, "'"));
  }
  auto value = GetValue(key);
  if (value.type() != required_type) {
    return absl::FailedPreconditionError(
        absl::StrCat("Value of key '", key,
                     "' is not of expected type; is: ", value.ToDebugString()));
  }
  return value;
}

absl::Status JsonValue::HasKeyOfType(const std::string& key,
                                     EType required_type) const {
  return GetValueOfType(key, required_type).status();
}

size_t JsonValue::size() const {
  switch (type()) {
    case JsonValue::kString:
      return as_string().size();
    case JsonValue::kObject:
      return as_object().size();
    case JsonValue::kArray:
      return as_array().size();
    default:
      LOG(DFATAL) << ToDebugString();
      return 0;
  }
}

std::string JsonValue::ToDebugString() const {
  std::ostringstream oss;
  oss << *this;
  return oss.str();
}

std::ostream& operator<<(std::ostream& os, const JsonValue& jv) {
  os << "{type: ";

  switch (jv.type()) {
    case JsonValue::kUnset:
      return os << "Unset}";

    case JsonValue::kNull:
      return os << "Null}";

    case JsonValue::kBool:
      os << "Bool, value: ";
      if (jv.as_bool()) {
        return os << "true}";
      } else {
        return os << "false}";
      }

    case JsonValue::kInteger:
      return os << "Integer, value: " << jv.as_integer() << "}";

    case JsonValue::kDouble:
      return os << "Double, value: " << jv.as_double() << "}";

    case JsonValue::kString:
      return os << "String, value: \"" << absl::CHexEscape(jv.as_string())
                << "\"}";

    case JsonValue::kObject:
      return os << "Object, value: " << ::testing::PrintToString(jv.as_object())
                << "}";

    case JsonValue::kArray:
      return os << "Array, value: " << ::testing::PrintToString(jv.as_array())
                << "}";
  }
  MCU_UNREACHABLE;
}

bool operator==(const JsonValue& jv, nullptr_t) {
  return jv.type() == JsonValue::kNull;
}

bool operator==(const JsonValue& jv, const bool v) {
  return jv.type() == JsonValue::kBool && jv.as_bool() == v;
}

bool operator==(const JsonValue& jv, const int v) {
  return jv == static_cast<int64_t>(v);
}

bool operator==(const JsonValue& jv, const int64_t v) {
  if (jv.type() == JsonValue::kInteger) {
    return jv.as_integer() == v;
  }
  if (jv.type() == JsonValue::kDouble) {
    return jv.as_double() == v;
  }
  return false;
}

bool operator==(const JsonValue& jv, const double v) {
  if (jv.type() != JsonValue::kDouble) {
    if (jv.type() == JsonValue::kInteger) {
      return jv.as_integer() == v;
    }
    return false;
  }
  double a = jv.as_double(), b = v;
  if (a == b) {
    return true;
  }
  if (std::signbit(a) != std::signbit(b)) {
    return false;
  }
  if (!std::isnormal(a) || !std::isnormal(b)) {
    return false;
  }
  if (std::nexttoward(a, b) == b) {
    return true;
  }
  if (std::nexttoward(b, a) == a) {
    return true;
  }
  return false;
}

bool operator==(const JsonValue& jv, const std::string_view v) {
  return jv.type() == JsonValue::kString && jv.as_string() == v;
}

bool operator==(const JsonValue& jv, const JsonArray& v) {
  return jv.type() == JsonValue::kArray && jv.as_array() == v;
}

bool operator==(const JsonValue& jv, const JsonObject& v) {
  return jv.type() == JsonValue::kObject && jv.as_object() == v;
}

bool operator==(const JsonValue& a, const JsonValue& b) {
  if (a.type() != b.type()) {
    return false;
  }
  switch (a.type()) {
    case JsonValue::kUnset:
      return true;
    case JsonValue::kNull:
      return true;
    case JsonValue::kBool:
      return a.as_bool() == b.as_bool();
    case JsonValue::kInteger:
      return a.as_integer() == b.as_integer();
    case JsonValue::kDouble:
      // Using the operator==(JsonValue, double) above as it handles dealing
      // with essentially equal values.
      return a == b.as_double();
    case JsonValue::kString:
      return a.as_string() == b.as_string();
    case JsonValue::kObject:
      return a.as_object() == b.as_object();
    case JsonValue::kArray:
      return a.as_array() == b.as_array();
  }
  MCU_UNREACHABLE;
}

}  // namespace test
}  // namespace mcucore
