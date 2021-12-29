#ifndef MCUCORE_SRC_PROGMEM_STRING_H_
#define MCUCORE_SRC_PROGMEM_STRING_H_

// ProgmemString encapsulates a pointer to a string literal; i.e. a pointer to a
// NUL-terminated string stored in program memory. Avoiding the use of 'const
// char*' as the type of a string literal helps deal with processor
// architectures with multiple address spaces, e.g. the Microchip AVR series,
// where address X alone isn't sufficient to identify whether X refers to RAM,
// Program Memory (e.g. flash), or some other space.
//
// NOTE: So far I've written this using char* pointers, which, IIUC, are limited
// to the first 64KB of flash on AVR chips. I don't know what guarantees there
// are about the placement of variables marked PROGMEM, in particular whether
// there is an attempt to place them early in the address space.
//
// Author: james.synge@gmail.com

#include "inline_literal.h"
#include "mcucore_platform.h"

namespace mcucore {

class ProgmemString {
 public:
  ProgmemString(const __FlashStringHelper* str)  // NOLINT
      : ptr_(reinterpret_cast<const char*>(str)) {}

  template <char... C, class PSS = progmem_data::ProgmemStringData<C...>>
  explicit constexpr ProgmemString(PSS str) : ptr_(PSS::kData) {}

  // Returns true if other has the same value.
  bool operator==(const ProgmemString& other) const;

  // Returns true if other has a different value.
  bool operator!=(const ProgmemString& other) const {
    return !(*this == other);
  }

  // Return a __FlashStringHelper*, the type used by Arduino's Print class to
  // help avoid copying strings into RAM.
  const __FlashStringHelper* ToFlashStringHelper() const {
    return reinterpret_cast<const __FlashStringHelper*>(ptr_);
  }

  // Print the string to the provided Print instance. This is not a virtual
  // function (i.e. not an override of Printable::printTo) because that would
  // remove the ability for this to have any constexpr ctor in C++ < 20.
  inline size_t printTo(Print& out) const {
    return out.print(ToFlashStringHelper());
  }

 private:
  // Pointer to a string in program memory (i.e. flash), rather than in RAM.
  const char* ptr_;
};

}  // namespace mcucore

#endif  // MCUCORE_SRC_PROGMEM_STRING_H_
