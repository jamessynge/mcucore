#include "extras/host/arduino/pgmspace.h"

#include <cstring>
#include <string_view>

#include "gtest/gtest.h"

namespace {

TEST(ProgramSpace, memchr_P) {
  const char kStr[] = "abc";
  EXPECT_EQ(memchr_P(kStr, 'a', 3), kStr + 0);
  EXPECT_EQ(memchr_P(kStr, 'b', 3), kStr + 1);
  EXPECT_EQ(memchr_P(kStr, 'c', 3), kStr + 2);

  EXPECT_EQ(memchr_P(kStr, '\0', 3), nullptr);
  EXPECT_EQ(memchr_P(kStr, ' ', 3), nullptr);
  EXPECT_EQ(memchr_P(kStr, 'A', 3), nullptr);
  EXPECT_EQ(memchr_P(kStr, 'Z', 3), nullptr);
  EXPECT_EQ(memchr_P(kStr, 'd', 3), nullptr);
}

TEST(ProgramSpace, memcmp_P) {
  EXPECT_EQ(memcmp_P("abc", "abd", 2), 0);
  EXPECT_EQ(memcmp_P("abc", "abc", 3), 0);

  EXPECT_LT(memcmp_P("abc", "abd", 3), 0);
  EXPECT_GT(memcmp_P("abd", "abc", 3), 0);
}

TEST(ProgramSpace, strncasecmp_P) {
  EXPECT_EQ(strncasecmp_P("A", "Z", 0), 0);
  EXPECT_EQ(strncasecmp_P("abc", "abc", 3), 0);
  EXPECT_EQ(strncasecmp_P("abcdef", "abcghi", 3), 0);

  EXPECT_LT(strncasecmp_P("abcdef", "abcghi", 4), 0);
  EXPECT_LT(strncasecmp_P("abcdef", "abcghi", 5), 0);
  EXPECT_LT(strncasecmp_P("abcdef", "abcghi", 6), 0);
  EXPECT_LT(strncasecmp_P("abcdef", "abcghi", 7), 0);

  EXPECT_GT(strncasecmp_P("abcghi", "abcdef", 4), 0);
  EXPECT_GT(strncasecmp_P("abcghi", "abcdef", 5), 0);
  EXPECT_GT(strncasecmp_P("abcghi", "abcdef", 6), 0);
  EXPECT_GT(strncasecmp_P("abcghi", "abcdef", 7), 0);

  EXPECT_EQ(strncasecmp_P("A", "A", 1), 0);
  EXPECT_EQ(strncasecmp_P("A", "a", 1), 0);
  EXPECT_EQ(strncasecmp_P("a", "A", 1), 0);
  EXPECT_EQ(strncasecmp_P("a", "a", 1), 0);

  EXPECT_LT(strncasecmp_P("a", "Z", 1), 0);
  EXPECT_LT(strncasecmp_P("A", "Z", 1), 0);
  EXPECT_LT(strncasecmp_P("A", "z", 1), 0);
  EXPECT_LT(strncasecmp_P("a", "z", 1), 0);

  EXPECT_GT(strncasecmp_P("123 z", "123 a", 10), 0);
  EXPECT_GT(strncasecmp_P("123 z", "123 A", 10), 0);
  EXPECT_GT(strncasecmp_P("123 Z", "123 a", 10), 0);
  EXPECT_GT(strncasecmp_P("123 Z", "123 A", 10), 0);
}

TEST(ProgramSpace, memcpy_P) {
  constexpr char from_buffer[] = "123 abc";
  char to_buffer[sizeof from_buffer] = "";

  EXPECT_NE(std::string_view(to_buffer, sizeof to_buffer), "123 abc");
  EXPECT_EQ(memcpy_P(to_buffer, from_buffer, sizeof from_buffer), to_buffer);
  EXPECT_EQ(std::string_view(to_buffer, std::strlen("123 abc")), "123 abc");
}

}  // namespace
