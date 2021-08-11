#ifndef MCUCORE_SRC_STRING_COMPARE_H_
#define MCUCORE_SRC_STRING_COMPARE_H_

// Defines string comparison methods for Literal, ProgmemStringView and
// StringView instances. Ideally these allow us to write code that doesn't worry
// about whether a string is stored in RAM or PROGMEM, but in practice it isn't
// clear this is of great value.
//
// Author: james.synge@gmail.com

#include "literal.h"
#include "mcucore_platform.h"
#include "progmem_string_view.h"
#include "string_view.h"

namespace mcucore {

// Returns true if a Literal and a StringView have identical contents.
bool operator==(const Literal& a, const StringView& b);
bool operator==(const StringView& a, const Literal& b);
bool ExactlyEqual(const Literal& a, const StringView& b);

// Returns true if a Literal and a StringView are not identical.
bool operator!=(const Literal& a, const StringView& b);
bool operator!=(const StringView& a, const Literal& b);

// Returns true if a Literal and a StringView are the same, case-insensitively.
bool CaseEqual(const Literal& a, const StringView& b);
bool CaseEqual(const StringView& a, const Literal& b);

// Returns true if text starts with prefix.
bool StartsWith(const StringView& text, const Literal& prefix);

////////////////////////////////////////

// Returns true if a ProgmemStringView and a StringView have identical contents.
bool operator==(const ProgmemStringView& a, const StringView& b);
bool operator==(const StringView& a, const ProgmemStringView& b);
bool ExactlyEqual(const ProgmemStringView& a, const StringView& b);

// Returns true if a ProgmemStringView and a StringView are not identical.
bool operator!=(const ProgmemStringView& a, const StringView& b);
bool operator!=(const StringView& a, const ProgmemStringView& b);

// Returns true if a ProgmemStringView and a StringView are the same,
// case-insensitively.
bool CaseEqual(const ProgmemStringView& a, const StringView& b);
bool CaseEqual(const StringView& a, const ProgmemStringView& b);

// Returns true if a ProgmemStringView and a StringView are the same, after
// lower-casing the progmem string view.
bool LoweredEqual(const ProgmemStringView& a, const StringView& b);

// Returns true if text starts with prefix.
bool StartsWith(const StringView& text, const ProgmemStringView& prefix);

}  // namespace mcucore

#endif  // MCUCORE_SRC_STRING_COMPARE_H_