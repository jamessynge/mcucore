#include "extras/host/arduino/pgmspace.h"

#include <strings.h>

#include <cstring>

uint8_t pgm_read_byte_near(const uint8_t* ptr) { return *ptr; }

uint32_t pgm_read_dword_far(const uint32_t* ptr) { return *ptr; }

const void* pgm_read_ptr_far(const void* ptr) {
  void* non_const_void_ptr = const_cast<void*>(ptr);
  void** void_ptr_ptr = reinterpret_cast<void**>(non_const_void_ptr);
  return *void_ptr_ptr;
}

const void* pgm_read_ptr_near(const void* ptr) { return pgm_read_ptr_far(ptr); }

const void* memchr_P(const void* s, int ch, size_t n) {
  return std::memchr(s, ch, n);
}

int memcmp_P(const void* lhs, const void* rhs, size_t count) {
  return std::memcmp(lhs, rhs, count);
}

void* memcpy_P(void* dest, const void* src, size_t n) {
  return std::memcpy(dest, src, n);
}

int strncasecmp_P(const char* s1, const char* s2, size_t n) {
  // Note that strncasecmp is NOT a part of the C or C++ standard libraries,
  // though it is a part of POSIX.
  return strncasecmp(s1, s2, n);
}

const char* strrchr_P(const char* s, int ch) { return std::strrchr(s, ch); }
