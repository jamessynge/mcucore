#ifndef MCUCORE_SRC_PRINT_MISC_H_
#define MCUCORE_SRC_PRINT_MISC_H_

// Miscellaneous print functions, in a separate compilation unit to avoid
// dependency cycles.
//
// Author: james.synge@gmail.com

#include "mcucore_platform.h"

namespace mcucore {

// Utility function supporting the printing of enumerator names, in this case
// handling an enum value that is not a defined enumerator.
size_t PrintUnknownEnumValueTo(const __FlashStringHelper* name, uint32_t v,
                               Print& out);

}  // namespace mcucore

#endif  // MCUCORE_SRC_PRINT_MISC_H_