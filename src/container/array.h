#ifndef MCUCORE_SRC_CONTAINER_ARRAY_H_
#define MCUCORE_SRC_CONTAINER_ARRAY_H_

// Array is similar to std::array; it contains (owns) a C-style array of T of
// known (fixed) size, and provides support for iterating through the array. A
// key application is supporting PrintableCat / AnyPrintableArray.
//
// Author: james.synge@gmail.com

#include "log/log.h"
#include "mcucore_platform.h"

namespace mcucore {

template <typename T, size_t SIZE>
struct Array {
 public:
  using value_type = T;
  using size_type = size_t;
#ifndef ARDUINO_ARCH_AVR
  using difference_type = ptrdiff_t;
#endif  // !ARDUINO_ARCH_AVR
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = value_type*;
  using const_iterator = const value_type*;

  // Copy from the provided array.
  template <typename U, size_t N>
  void copy(const U (&data)[N]) {
    size_type limit = N > SIZE ? SIZE : N;
    for (size_type ndx = 0; ndx < limit; ++ndx) {
      elems_[ndx] = data[ndx];
    }
  }

  iterator begin() { return elems_; }
  const_iterator begin() const { return elems_; }
  iterator end() { return elems_ + SIZE; }
  const_iterator end() const { return elems_ + SIZE; }

  // Returns the number of elements in the array.
  constexpr size_type size() const { return SIZE; }
  constexpr bool empty() const { return false; }

  // Element access:
  reference operator[](size_type ndx) {
    MCU_DCHECK_LT(ndx, SIZE);
    return elems_[ndx];
  }
  const_reference operator[](size_type ndx) const {
    MCU_DCHECK_LT(ndx, SIZE);
    return elems_[ndx];
  }
  reference at(size_type ndx) {
    MCU_DCHECK_LT(ndx, SIZE);
    return elems_[ndx];
  }
  const_reference at(size_type ndx) const {
    MCU_DCHECK_LT(ndx, SIZE);
    return elems_[ndx];
  }

  // Returns a pointer to the first element of the underlying array.
  reference front() { return elems_[0]; }
  const_reference front() const { return elems_[0]; }
  reference back() { return elems_[SIZE - 1]; }
  const_reference back() const { return elems_[SIZE - 1]; }

  // Returns a pointer to the first element of the underlying array.
  pointer data() { return elems_; }
  constexpr const_pointer data() const { return elems_; }

  // The actual data.
  value_type elems_[SIZE];  // NOLINT
};

// Returns an Array with the size and values of the array passed as the single
// argument to MakeArray.
template <typename T, int SIZE, typename A = Array<T, SIZE>>
A MakeFromArray(const T (&data)[SIZE]) {
  A array;
  array.copy(data);
  return array;
}

// Returns an Array whose values are those of the arguments to MakeArray; the
// second and later arguments must all be implicitly convertible to the type of
// the first argument as that is the type used for the Array's elements.
template <typename... Ts, typename U, typename A = Array<U, 1 + sizeof...(Ts)>>
constexpr A MakeArray(U u, Ts... ts) {
  return A{u, ts...};
}

// Make an Array of one element.
template <typename T>
constexpr Array<T, 1> MakeArray(T a) {
  return {a};
}

}  // namespace mcucore

#endif  // MCUCORE_SRC_CONTAINER_ARRAY_H_
