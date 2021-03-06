#ifndef MCUCORE_EXTRAS_HOST_ARDUINO_WSTRING_H_
#define MCUCORE_EXTRAS_HOST_ARDUINO_WSTRING_H_

// This provides just enough of WString.h (from Arduino) for my needs when
// testing on host.

// There is no actual definition of class __FlashStringHelper, it is just used
// to provide a distinct type for a pointer to a char array stored in flash.
class __FlashStringHelper;

// It turns out that absl/meta/type_traits.h uses the symbol F in a template
// definition, and the Arduino definition interferes. Commenting out here; see
// MCU_FLASHSTR in progmem_string_data.h for an alternative solution that
// collapses multiple copies of a string literal at build time, and FLASHSTR in
// mcucore_platform.h for a version of F that works without using such a short,
// collision prone name.
//
// #define F(string_literal) \
//   (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))

#endif  // MCUCORE_EXTRAS_HOST_ARDUINO_WSTRING_H_
