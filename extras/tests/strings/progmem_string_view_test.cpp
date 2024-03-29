#include "strings/progmem_string_view.h"

#include <cstring>
#include <iosfwd>
#include <limits>
#include <sstream>
#include <string>

#include "absl/strings/string_view.h"
#include "extras/test_tools/print_to_std_string.h"
#include "extras/test_tools/progmem_string_view_utils.h"
#include "extras/test_tools/string_view_utils.h"
#include "extras/test_tools/test_strings.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mcucore_platform.h"
#include "print/hex_escape.h"
#include "print/o_print_stream.h"
#include "strings/string_compare.h"
#include "strings/string_view.h"

namespace mcucore {
namespace test {
namespace {

using ::testing::IsEmpty;

#define LOWER_STR "some\\thing\twith\r\n\b\f\"quotes\"."
#define MIXED_STR "Some\\thing\tWith\r\n\b\f\"Quotes\"."
#define UPPER_STR "SOME\\THING\tWITH\r\n\b\f\"QUOTES\"."

constexpr char kLowerStr[] AVR_PROGMEM = LOWER_STR;
constexpr char kMixedStr[] AVR_PROGMEM = MIXED_STR;
constexpr char kUpperStr[] AVR_PROGMEM = UPPER_STR;

constexpr StringView kLowerView(LOWER_STR);
constexpr StringView kMixedView(MIXED_STR);
constexpr StringView kUpperView(UPPER_STR);

constexpr char kLowerHexEscaped[] =
    "\"some\\\\thing\\x09with\\r\\n\\x08\\x0C\\\"quotes\\\".\"";

TEST(ProgmemStringViewTest, CreateFromAvrProgmemCharArray) {
  ProgmemStringView psv(kLowerStr);
  EXPECT_EQ(psv.size(), kLowerView.size());
  EXPECT_EQ(psv.progmem_ptr(), reinterpret_cast<PGM_VOID_P>(kLowerStr));
  EXPECT_EQ(psv, kLowerView);
  EXPECT_EQ(psv.at(0), LOWER_STR[0]);
  EXPECT_EQ(psv.substr(1, 4), kLowerView.substr(1, 4));
}

TEST(ProgmemStringViewTest, CreateFromProgmemStringData) {
  ProgmemStringView psv(MCU_PSD(MIXED_STR));
  EXPECT_EQ(psv.size(), kMixedView.size());
  EXPECT_EQ(psv.progmem_ptr(),
            reinterpret_cast<PGM_VOID_P>(MCU_PSD_TYPE(MIXED_STR)::kData));
  EXPECT_EQ(psv, kMixedView);
  EXPECT_EQ(psv.at(0), MIXED_STR[0]);
  EXPECT_EQ(psv.substr(5, 3), kMixedView.substr(5, 3));
}

TEST(ProgmemStringViewTest, LowerComparison) {
  ProgmemStringView psv(kLowerStr);
  EXPECT_EQ(psv.size(), std::strlen(kLowerStr));
  EXPECT_EQ(psv.size(), kLowerView.size());
  EXPECT_EQ(psv, kLowerView);

  EXPECT_NE(psv, kMixedView);
  EXPECT_NE(psv, kUpperView);

  // Make a copy so that we know that operator== isn't just comparing pointers.
  std::string str(kLowerStr);
  StringView view = MakeStringView(str);
  EXPECT_EQ(psv, view);

  // This prefix will share the same pointer, but not the same length.
  EXPECT_NE(psv, kLowerView.prefix(8));

  // Case-insensitively equal to all of the variants.
  EXPECT_TRUE(CaseEqual(psv, view));
  EXPECT_TRUE(CaseEqual(psv, kLowerView));
  EXPECT_TRUE(CaseEqual(psv, kMixedView));
  EXPECT_TRUE(CaseEqual(psv, kUpperView));

  // Equal to kLowerStr if we lower-case the ProgmemStringView, but not equal to
  // the other variants.
  EXPECT_TRUE(LoweredEqual(psv, view));
  EXPECT_TRUE(LoweredEqual(psv, kLowerView));
  EXPECT_FALSE(LoweredEqual(psv, kMixedView));
  EXPECT_FALSE(LoweredEqual(psv, kUpperView));

  // Not case-insensitively equal to an empty string, nor to prefixes of itself.
  EXPECT_FALSE(CaseEqual(psv, StringView()));
  EXPECT_FALSE(CaseEqual(psv, StringView("")));
  EXPECT_FALSE(CaseEqual(psv, kLowerView.prefix(1)));
  EXPECT_FALSE(CaseEqual(psv, kLowerView.prefix(kLowerView.size() - 1)));

  // at() will return the appropriate character.
  EXPECT_EQ(psv.at(0), 's');
  EXPECT_EQ(psv.at(psv.size() - 1), '.');
}

TEST(ProgmemStringViewTest, MixedComparison) {
  ProgmemStringView psv(kMixedStr);
  EXPECT_EQ(psv.size(), std::strlen(kMixedStr));
  EXPECT_EQ(psv.size(), kMixedView.size());
  EXPECT_EQ(psv, kMixedView);

  EXPECT_NE(psv, kLowerView);
  EXPECT_NE(psv, kUpperView);

  // Make a copy so that we know that operator== isn't just comparing pointers.
  std::string str(kMixedStr);
  StringView view = MakeStringView(str);
  EXPECT_EQ(psv, view);

  // This prefix will share the same pointer, but not the same length.
  EXPECT_NE(psv, kMixedView.prefix(8));

  // Case-insensitively equal to all of the variants.
  EXPECT_TRUE(CaseEqual(psv, view));
  EXPECT_TRUE(CaseEqual(psv, kLowerView));
  EXPECT_TRUE(CaseEqual(psv, kMixedView));
  EXPECT_TRUE(CaseEqual(psv, kUpperView));

  // Not case-insensitively equal to an empty string, nor to prefixes of itself.
  EXPECT_FALSE(CaseEqual(psv, StringView("")));
  EXPECT_FALSE(CaseEqual(psv, StringView()));
  EXPECT_FALSE(CaseEqual(psv, kMixedView.prefix(1)));
  EXPECT_FALSE(CaseEqual(psv, kMixedView.prefix(kMixedView.size() - 1)));

  // Equal to kLowerStr if we lower-case the ProgmemStringView, but not equal to
  // the other variants.
  EXPECT_FALSE(LoweredEqual(psv, view));
  EXPECT_TRUE(LoweredEqual(psv, kLowerView));
  EXPECT_FALSE(LoweredEqual(psv, kMixedView));
  EXPECT_FALSE(LoweredEqual(psv, kUpperView));

  // at() will return the appropriate character.
  EXPECT_EQ(psv.at(0), 'S');
  EXPECT_EQ(psv.at(psv.size() - 1), '.');
}

TEST(ProgmemStringViewTest, UpperComparison) {
  ProgmemStringView psv(kUpperStr);
  EXPECT_EQ(psv.size(), std::strlen(kUpperStr));
  EXPECT_EQ(psv.size(), kUpperView.size());
  EXPECT_EQ(psv, kUpperView);

  EXPECT_NE(psv, kMixedView);
  EXPECT_NE(psv, kLowerView);

  // Make a copy so that we know that operator== isn't just comparing pointers.
  std::string str(kUpperStr);
  StringView view = MakeStringView(str);
  EXPECT_EQ(psv, view);

  // This prefix will share the same pointer, but not the same length.
  EXPECT_NE(psv, kUpperView.prefix(8));

  // Case-insensitively equal to all of the variants.
  EXPECT_TRUE(CaseEqual(psv, view));
  EXPECT_TRUE(CaseEqual(psv, kLowerView));
  EXPECT_TRUE(CaseEqual(psv, kMixedView));
  EXPECT_TRUE(CaseEqual(psv, kUpperView));

  // Not case-insensitively equal to an empty string, nor to prefixes of itself.
  EXPECT_FALSE(CaseEqual(psv, StringView("")));
  EXPECT_FALSE(CaseEqual(psv, kUpperView.prefix(1)));
  EXPECT_FALSE(CaseEqual(psv, kUpperView.prefix(kUpperView.size() - 1)));

  // Equal to kLowerStr if we lower-case the ProgmemStringView, but not equal to
  // the other variants.
  EXPECT_FALSE(LoweredEqual(psv, view));
  EXPECT_TRUE(LoweredEqual(psv, kLowerView));
  EXPECT_FALSE(LoweredEqual(psv, kMixedView));
  EXPECT_FALSE(LoweredEqual(psv, kUpperView));

  // at() will return the appropriate character.
  EXPECT_EQ(psv.at(0), 'S');
  EXPECT_EQ(psv.at(psv.size() - 1), '.');
}

TEST(ProgmemStringViewTest, Copy) {
  ProgmemStringView psv(kMixedStr);
  EXPECT_EQ(psv.size(), sizeof(kMixedStr) - 1);

  // Make a buffer that is one byte bigger than needed for CopyTo so that we can
  // add a NUL at the end.
  char buffer[sizeof kMixedStr] = "";
  buffer[kMixedView.size()] = 0;
  EXPECT_EQ(psv.size() + 1, sizeof buffer);
  EXPECT_THAT(std::string_view(buffer), IsEmpty());

  // Can't copy if the destination size is too small.
  EXPECT_FALSE(psv.CopyTo(buffer, 0));
  EXPECT_THAT(std::string_view(buffer), IsEmpty());
  EXPECT_FALSE(psv.CopyTo(buffer, psv.size() - 1));
  EXPECT_THAT(std::string_view(buffer), IsEmpty());

  // And can copy if the destination is the right size or larger.
  EXPECT_TRUE(psv.CopyTo(buffer, psv.size()));
  EXPECT_EQ(std::string_view(buffer), kMixedStr);
  EXPECT_TRUE(psv.CopyTo(buffer, sizeof buffer));
  EXPECT_EQ(std::string_view(buffer), kMixedStr);
}

TEST(ProgmemStringViewTest, Contains) {
#define CONTAINS_EXAMPLE "!#$%&'*+-.^_`|~"
  const std::string kStdStringExample(CONTAINS_EXAMPLE);
  {
    const auto psv = MCU_PSV(CONTAINS_EXAMPLE);
    char c = std::numeric_limits<char>::min();
    do {
      if (kStdStringExample.find(c) == std::string::npos) {  // NOLINT
        EXPECT_FALSE(psv.contains(c));
      } else {
        EXPECT_TRUE(psv.contains(c));
      }
    } while (c++ < std::numeric_limits<char>::max());
  }
  {
    char c = std::numeric_limits<char>::min();
    do {
      if (kStdStringExample.find(c) == std::string::npos) {  // NOLINT
        EXPECT_FALSE(MCU_PSV(CONTAINS_EXAMPLE).contains(c));
      } else {
        EXPECT_TRUE(MCU_PSV(CONTAINS_EXAMPLE).contains(c));
      }
    } while (c++ < std::numeric_limits<char>::max());
  }
}

TEST(ProgmemStringViewTest, PrintTo) {
  {
    ProgmemStringView psv(kMixedStr);
    mcucore::test::PrintToStdString out;
    EXPECT_EQ(psv.printTo(out), psv.size());
    EXPECT_EQ(out.str(), kMixedStr);
  }
  {
    ProgmemStringView psv(TEST_STR_255);
    mcucore::test::PrintToStdString out;
    EXPECT_EQ(psv.printTo(out), psv.size());
    EXPECT_EQ(out.str(), TEST_STR_255);
  }
}

TEST(ProgmemStringViewTest, StreamMixed) {
  ProgmemStringView psv(kMixedStr);

  mcucore::test::PrintToStdString p2ss;
  OPrintStream out(p2ss);
  out << psv;
  EXPECT_EQ(p2ss.str(), kMixedStr);

  std::ostringstream oss;
  oss << psv;
  EXPECT_EQ(oss.str(), kMixedStr);
}

TEST(ProgmemStringViewTest, StreamUpper) {
  ProgmemStringView psv(kUpperStr);

  mcucore::test::PrintToStdString p2ss;
  OPrintStream out(p2ss);
  out << psv;
  EXPECT_EQ(p2ss.str(), kUpperStr);

  std::ostringstream oss;
  oss << psv;
  EXPECT_EQ(oss.str(), kUpperStr);
}

TEST(ProgmemStringViewTest, StreamHexEscaped) {
  ProgmemStringView psv(kLowerStr);
  mcucore::test::PrintToStdString p2ss;
  OPrintStream out(p2ss);
  out << HexEscaped(psv);
  EXPECT_EQ(p2ss.str(), kLowerHexEscaped);
}

TEST(ProgmemStringViewTest, Equality) {
  ProgmemStringView psv1("abc");
  EXPECT_EQ(psv1, psv1);
  EXPECT_TRUE(psv1.Identical(psv1));

  std::string abc("abc");
  ProgmemStringView psv2(abc.data(), abc.size());
  EXPECT_EQ(psv1, psv2);
  EXPECT_FALSE(psv1.Identical(psv2));

  ProgmemStringView psv3(abc.data(), abc.size());
  EXPECT_EQ(psv1, psv3);
  EXPECT_EQ(psv2, psv3);
  EXPECT_TRUE(psv2.Identical(psv3));
}

TEST(ProgmemStringViewTest, Inequality) {
  // Same length, different values, but not in the first character.
  EXPECT_NE(ProgmemStringView("aBc"), ProgmemStringView("abc"));
  EXPECT_NE(ProgmemStringView("abc"), ProgmemStringView("aBc"));

  // Same length, different values, in the first character.
  EXPECT_NE(ProgmemStringView("abc"), ProgmemStringView("Abc"));

  // Different length.
  EXPECT_NE(ProgmemStringView("abc"), ProgmemStringView("ab"));
  EXPECT_NE(ProgmemStringView("ab"), ProgmemStringView("abc"));

  // One empty.
  EXPECT_NE(ProgmemStringView("abc"), ProgmemStringView(""));
  EXPECT_NE(ProgmemStringView(""), ProgmemStringView("abc"));
}

TEST(ProgmemStringViewTest, LoweredEqual) {
  EXPECT_TRUE(ProgmemStringView(TEST_STR_32).LoweredEqual(TEST_STR_32, 32));
  EXPECT_FALSE(ProgmemStringView(TEST_STR_32).LoweredEqual(TEST_STR_31, 31));
  EXPECT_FALSE(ProgmemStringView(TEST_STR_32).LoweredEqual(TEST_STR_33, 33));
}

TEST(ProgmemStringViewTest, IsPrefixOf) {
  EXPECT_FALSE(ProgmemStringView(TEST_STR_32).IsPrefixOf(TEST_STR_31, 31));
  EXPECT_TRUE(ProgmemStringView(TEST_STR_32).IsPrefixOf(TEST_STR_32, 32));
  EXPECT_TRUE(ProgmemStringView(TEST_STR_32).IsPrefixOf(TEST_STR_33, 33));

  // Same length, different values.
  EXPECT_FALSE(ProgmemStringView("ab").IsPrefixOf("cd", 2));
}

TEST(ProgmemStringDataTest, McuPsvPrintTo) {
  mcucore::test::PrintToStdString out;
  EXPECT_EQ(MCU_PSV("Hey There").printTo(out), 9);
  EXPECT_EQ(out.str(), "Hey There");
  EXPECT_EQ(MCU_PSV("Hey There").size(), 9);
}

TEST(ProgmemStringDataTest, StreamMcuPsv) {
  mcucore::test::PrintToStdString out;
  OPrintStream strm(out);
  strm << MCU_PSV("Hey There");
  EXPECT_EQ(out.str(), "Hey There");
}

TEST(ProgmemStringDataTest, McuPsvToProgmemStringView) {
  ProgmemStringView progmem_string_view = MCU_PSV("Hey There");
  EXPECT_EQ(progmem_string_view.size(), 9);
  mcucore::test::PrintToStdString out;
  EXPECT_EQ(progmem_string_view.printTo(out), 9);
  EXPECT_EQ(out.str(), "Hey There");
}

}  // namespace
}  // namespace test
}  // namespace mcucore
