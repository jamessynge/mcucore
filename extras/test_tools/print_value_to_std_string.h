#ifndef MCUCORE_EXTRAS_TEST_TOOLS_PRINT_VALUE_TO_STD_STRING_H_
#define MCUCORE_EXTRAS_TEST_TOOLS_PRINT_VALUE_TO_STD_STRING_H_

// TODO(jamessynge): Rename to StreamValueToStdString, to better reflect what it
// does, thus what it helps to test.

// PrintToStdString helps with testing methods that accept a Print object and
// print/write to it. And PrintValueToStdString helps with formatting values for
// which we have Arduino compatible formatters, but not std::ostream formatters.
//
// Note that these are not in namespace mcucore::test so that they can be used
// by non-tests (i.e. for implementing operator<< for host use, but not
// necessarily tests).
//
// Author: james.synge@gmail.com

#include <stddef.h>
#include <stdint.h>

#include <sstream>
#include <string>

#include "extras/test_tools/print_to_std_string.h"
#include "o_print_stream.h"

namespace mcucore {

template <class T>
std::string PrintValueToStdString(const T& t) {
  mcucore::test::PrintToStdString out;
  OPrintStream strm(out);
  strm << t;
  return out.str();
}

}  // namespace mcucore

#endif  // MCUCORE_EXTRAS_TEST_TOOLS_PRINT_VALUE_TO_STD_STRING_H_
