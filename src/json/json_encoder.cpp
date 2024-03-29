#include "json/json_encoder.h"

// Author: james.synge@gmail.com

#include <math.h>

#include "log/log.h"
#include "print/counting_print.h"
#include "print/o_print_stream.h"
#include "strings/progmem_string_view.h"

namespace mcucore {
namespace {

void PrintCharJsonEscaped(Print& out, const char c) {
  if (isPrintable(c)) {
    if (c == '"') {
      out.print('\\');
      out.print('"');
    } else if (c == '\\') {
      out.print('\\');
      out.print('\\');
    } else {
      out.print(c);
    }
  } else if (c == '\b') {
    out.print('\\');
    out.print('b');
  } else if (c == '\f') {
    out.print('\\');
    out.print('f');
  } else if (c == '\n') {
    out.print('\\');
    out.print('n');
  } else if (c == '\r') {
    out.print('\\');
    out.print('r');
  } else if (c == '\t') {
    out.print('\\');
    out.print('t');
  } else {
    // This used to be a DCHECK, but a VLOG is better because the character
    // could come from client input.
    MCU_VLOG(4) << MCU_PSD("Unsupported JSON character: ") << BaseHex
                << (c + 0);
  }
}

// Wraps a Print instance, forwards output to that instance with JSON escaping
// applied. Note that this does NOT add double quotes before and after the
// output. This class also does NOT count the length of strings because the
// callers don't need that, so we don't waste the time or space doing so.
class PrintJsonEscaped : public Print {
 public:
  explicit PrintJsonEscaped(Print& wrapped) : wrapped_(wrapped) {}

  // These are the two abstract virtual methods in Arduino's Print class. I'm
  // treating the uint8_t 'b' as an ASCII char.
  size_t write(uint8_t b) override {
    PrintCharJsonEscaped(wrapped_, static_cast<char>(b));
    return 1;  // Not necessarily correct.
  }

  size_t write(const uint8_t* buffer, size_t size) override {
    for (size_t ndx = 0; ndx < size; ++ndx) {
      PrintCharJsonEscaped(wrapped_, static_cast<char>(buffer[ndx]));
    }
    return size;  // Not necessarily correct.
  }

  // Export the other write methods.
  using Print::write;

 private:
  Print& wrapped_;
};

void PrintJsonEscapedStringTo(const Printable& value, Print& raw_output) {
  PrintJsonEscaped out(raw_output);
  raw_output.print('"');
  value.printTo(out);
  raw_output.print('"');
}

void PrintBoolean(Print& out, const bool value) {
  if (value) {
    MCU_PSV("true").printTo(out);
  } else {
    MCU_PSV("false").printTo(out);
  }
}

template <typename T>
void PrintInteger(Print& out, const T value) {
  // +0 to cause promotion of chars, if they're passed in.
  out.print(value + static_cast<uint16_t>(0));
}

// Prints the floating point value to out, if possible. If not, prints a JSON
// string that "describes" the situation. This is similar to various JSON
// libraries, which inspired this. TBD whether this is a good idea in the ASCOM
// Alpaca setting.
template <typename T>
void PrintFloatingPoint(Print& out, const T value) {
  // Haven't got std::isnan or std::isfinite in the Arduino environment, so
  // using the C versions in <math.h>.
  if (isnan(value)) {
    PrintJsonEscapedStringTo(AnyPrintable(MCU_PSV("NaN")), out);
  } else if (!isfinite(value)) {
    if (value > 0) {
      PrintJsonEscapedStringTo(AnyPrintable(MCU_PSV("Inf")), out);
    } else {
      PrintJsonEscapedStringTo(AnyPrintable(MCU_PSV("-Inf")), out);
    }
  } else {
    // We're assuming that the Print object is configured to match JSON
    // requirements for the formatting of numbers.
    //
    // NOTE: That assumption isn't very good. Arduino's Print::print(double)
    // method prints at most 2 digits to the right of the decimal point by
    // default, even if the number is 0.000123, so that example prints as "0.".
    //
    // TODO(jamessynge): Improve the printing of floating point values such that
    // we don't lose too much precision. Note that JSON allows for scientific
    // notation, which can allow for reasonable precision without requiring a
    // lot of digits be output. Consider using log10, frexp, etc. to determine
    // the base-10 exponent that applies, and thus whether to shift to
    // scientific notation or not.
    //
    // For ideas, see:
    // https://jkorpela.fi/c/eng.html
    // https://stackoverflow.com/a/19083594
    // https://stackoverflow.com/a/28334452
    // https://stackoverflow.com/a/17175504
    out.print(value);
  }
}

}  // namespace

JsonElementSource::~JsonElementSource() {}

JsonPropertySource::~JsonPropertySource() {}

AbstractJsonEncoder::AbstractJsonEncoder(Print& out)
    : out_(out), first_(true) {}

void AbstractJsonEncoder::StartItem() {
  if (first_) {
    first_ = false;
  } else {
    out_.print(',');
    out_.print(' ');
  }
}

void AbstractJsonEncoder::EncodeChildArray(const JsonElementSource& source) {
  JsonArrayEncoder encoder(out_);
  source.AddTo(encoder);
}

void AbstractJsonEncoder::EncodeChildObject(const JsonPropertySource& source) {
  JsonObjectEncoder encoder(out_);
  source.AddTo(encoder);
}

void AbstractJsonEncoder::PrintString(const Printable& printable) {
  PrintJsonEscapedStringTo(printable, out_);
}

////////////////////////////////////////////////////////////////////////////////

JsonArrayEncoder::JsonArrayEncoder(Print& out) : AbstractJsonEncoder(out) {
  out_.print('[');
}

JsonArrayEncoder::~JsonArrayEncoder() { out_.print(']'); }

void JsonArrayEncoder::AddIntElement(const int32_t value) {
  StartItem();
  PrintInteger(out_, value);
}

void JsonArrayEncoder::AddUIntElement(const uint32_t value) {
  StartItem();
  PrintInteger(out_, value);
}

void JsonArrayEncoder::AddFloatElement(float value) {
  StartItem();
  PrintFloatingPoint(out_, value);
}

void JsonArrayEncoder::AddDoubleElement(double value) {
  StartItem();
  PrintFloatingPoint(out_, value);
}

void JsonArrayEncoder::AddBooleanElement(const bool value) {
  StartItem();
  PrintBoolean(out_, value);
}

void JsonArrayEncoder::AddStringElement(const AnyPrintable& value) {
  StartItem();
  PrintString(value);
}

void JsonArrayEncoder::AddStringElement(const Printable& value) {
  StartItem();
  PrintString(value);
}

void JsonArrayEncoder::AddArrayElement(const JsonElementSource& source) {
  StartItem();
  EncodeChildArray(source);
}

void JsonArrayEncoder::AddObjectElement(const JsonPropertySource& source) {
  StartItem();
  EncodeChildObject(source);
}

// static
void JsonArrayEncoder::Encode(const JsonElementSource& source, Print& out) {
  JsonArrayEncoder encoder(out);
  source.AddTo(encoder);
}

// static
size_t JsonArrayEncoder::EncodedSize(const JsonElementSource& source) {
  PrintNoOp no_op;
  CountingPrint counter(no_op);
  Encode(source, counter);
  return counter.count();
}

////////////////////////////////////////////////////////////////////////////////

JsonObjectEncoder::JsonObjectEncoder(Print& out) : AbstractJsonEncoder(out) {
  out_.print('{');
}

JsonObjectEncoder::~JsonObjectEncoder() { out_.print('}'); }

void JsonObjectEncoder::StartProperty(const AnyPrintable& name) {
  StartItem();
  PrintString(name);
  out_.print(':');
  out_.print(' ');
}

void JsonObjectEncoder::AddIntProperty(const AnyPrintable& name,
                                       int32_t value) {
  StartProperty(name);
  PrintInteger(out_, value);
}

void JsonObjectEncoder::AddUIntProperty(const AnyPrintable& name,
                                        uint32_t value) {
  StartProperty(name);
  PrintInteger(out_, value);
}

void JsonObjectEncoder::AddFloatProperty(const AnyPrintable& name,
                                         float value) {
  StartProperty(name);
  PrintFloatingPoint(out_, value);
}

void JsonObjectEncoder::AddDoubleProperty(const AnyPrintable& name,
                                          double value) {
  StartProperty(name);
  PrintFloatingPoint(out_, value);
}

void JsonObjectEncoder::AddBooleanProperty(const AnyPrintable& name,
                                           const bool value) {
  StartProperty(name);
  PrintBoolean(out_, value);
}

void JsonObjectEncoder::AddStringProperty(const AnyPrintable& name,
                                          const AnyPrintable& value) {
  StartProperty(name);
  PrintString(value);
}

void JsonObjectEncoder::AddStringProperty(const AnyPrintable& name,
                                          const Printable& value) {
  StartProperty(name);
  PrintString(value);
}

void JsonObjectEncoder::AddArrayProperty(const AnyPrintable& name,
                                         const JsonElementSource& source) {
  StartProperty(name);
  EncodeChildArray(source);
}

void JsonObjectEncoder::AddObjectProperty(const AnyPrintable& name,
                                          const JsonPropertySource& source) {
  StartProperty(name);
  EncodeChildObject(source);
}

// static
void JsonObjectEncoder::Encode(const JsonPropertySource& source, Print& out) {
  JsonObjectEncoder encoder(out);
  source.AddTo(encoder);
}

// static
size_t JsonObjectEncoder::EncodedSize(const JsonPropertySource& source) {
  PrintNoOp no_op;
  CountingPrint counter(no_op);
  Encode(source, counter);
  return counter.count();
}

}  // namespace mcucore
