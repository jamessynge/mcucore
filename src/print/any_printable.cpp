#include "print/any_printable.h"

namespace mcucore {

AnyPrintable::AnyPrintable() : type_(AnyPrintable::kEmpty), signed_(0) {}

AnyPrintable::AnyPrintable(StringView value)
    : type_(AnyPrintable::kStringView), view_(value) {}

AnyPrintable::AnyPrintable(ProgmemString value)
    : type_(AnyPrintable::kFlashStringHelper),
      flash_string_helper_(value.ToFlashStringHelper()) {}

AnyPrintable::AnyPrintable(ProgmemStringView value)
    : type_(AnyPrintable::kProgmemStringView), psv_(value) {}

AnyPrintable::AnyPrintable(const __FlashStringHelper* value)
    : type_(AnyPrintable::kFlashStringHelper), flash_string_helper_(value) {}

AnyPrintable::AnyPrintable(Printable& value)
    : type_(AnyPrintable::kPrintable), printable_(&value) {}

AnyPrintable::AnyPrintable(const Printable& value)
    : type_(AnyPrintable::kPrintable), printable_(&value) {}

AnyPrintable::AnyPrintable(char value)
    : type_(AnyPrintable::kChar), char_(value) {}

AnyPrintable::AnyPrintable(int16_t value)
    : type_(AnyPrintable::kInteger), signed_(value) {}

AnyPrintable::AnyPrintable(uint16_t value)
    : type_(AnyPrintable::kUnsignedInteger), unsigned_(value) {}

AnyPrintable::AnyPrintable(int32_t value)
    : type_(AnyPrintable::kInteger), signed_(value) {}

AnyPrintable::AnyPrintable(uint32_t value)
    : type_(AnyPrintable::kUnsignedInteger), unsigned_(value) {}

AnyPrintable::AnyPrintable(float value)
    : type_(AnyPrintable::kFloat), float_(value) {}

AnyPrintable::AnyPrintable(double value)
    : type_(AnyPrintable::kDouble), double_(value) {}

AnyPrintable::AnyPrintable(const AnyPrintable& other) { *this = other; }

AnyPrintable::AnyPrintable(ArbitraryPrintFunction printer, const void* data)
    : type_(AnyPrintable::kArbitrary),
      arbitrary_{.printer = printer, .data = data} {}

AnyPrintable& AnyPrintable::operator=(const AnyPrintable& other) {
  type_ = other.type_;
  switch (type_) {
    case kEmpty:
      break;
    case kProgmemStringView:
      psv_ = other.psv_;
      break;
    case kStringView:
      view_ = other.view_;
      break;
    case kPrintable:
      printable_ = other.printable_;
      break;
    case kFlashStringHelper:
      flash_string_helper_ = other.flash_string_helper_;
      break;
    case kChar:
      char_ = other.char_;
      break;
    case kInteger:
      signed_ = other.signed_;
      break;
    case kUnsignedInteger:
      unsigned_ = other.unsigned_;
      break;
    case kFloat:
      float_ = other.float_;
      break;
    case kDouble:
      double_ = other.double_;
      break;
    case kArbitrary:
      arbitrary_ = other.arbitrary_;
      break;
  }

  return *this;
}

size_t AnyPrintable::printTo(Print& out) const {
  switch (type_) {
    case kEmpty:
      // break here and return below to allow for 100% line coverage. I.e. the
      // compiler/coverage analysis doesn't "believe" that type_ might not have
      // an invalid value.
      break;
    case kProgmemStringView:
      return psv_.printTo(out);
    case kStringView:
      return view_.printTo(out);
    case kPrintable:
      return printable_->printTo(out);
    case kFlashStringHelper:
      return out.print(flash_string_helper_);
    case kChar:
      return out.print(char_);
    case kInteger:
      return out.print(signed_);
    case kUnsignedInteger:
      return out.print(unsigned_);
    case kFloat:
      return out.print(float_);
    case kDouble:
      return out.print(double_);
    case kArbitrary:
      return arbitrary_.printer(out, arbitrary_.data);
  }
  return 0;
}

}  // namespace mcucore
