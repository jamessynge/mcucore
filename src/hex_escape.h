#ifndef MCUCORE_SRC_HEX_ESCAPE_H_
#define MCUCORE_SRC_HEX_ESCAPE_H_

// Support for printing strings (Printable's or similar) with non-printable
// ASCII characters hex escaped. Intended to produce output that is valid as a
// C/C++ string literal.
//
// Author: james.synge@gmail.com

#include "mcucore_platform.h"

namespace mcucore {

// Print |c| hex escaped to |out|. Actually backslash escapes backslash and
// double quote, and uses named escapes for newline (\n), carriage return (\t),
// etc.; for other control characters and non-ascii characters, uses a hex
// escape (e.g. \x01 or \xff).
size_t PrintCharHexEscaped(Print& out, const char c);

// Wraps a Print instance, forwards output to that instance with hex escaping
// applied. Note that this does NOT add double quotes before and after the
// output.
class PrintHexEscaped : public Print {
 public:
  explicit PrintHexEscaped(Print& wrapped);

  // These are the two abstract virtual methods in Arduino's Print class. I'm
  // presuming that the uint8_t 'b' is actually an ASCII char.
  size_t write(uint8_t b) override;
  size_t write(const uint8_t* buffer, size_t size) override;

  // Export the other write methods.
  using Print::write;

 private:
  Print& wrapped_;
};

template <class T>
class HexEscapedPrintable : public Printable {
 public:
  explicit HexEscapedPrintable(const T& wrapped) : wrapped_(wrapped) {}

  size_t printTo(Print& raw_out) const override {
    size_t count = raw_out.print('"');
    PrintHexEscaped out(raw_out);
    count += wrapped_.printTo(out);
    count += raw_out.print('"');
    return count;
  }

 private:
  const T& wrapped_;
};

template <typename T>
inline HexEscapedPrintable<T> HexEscaped(const T& like_printable) {
  return HexEscapedPrintable<T>(like_printable);
}

}  // namespace mcucore

#endif  // MCUCORE_SRC_HEX_ESCAPE_H_
