#include "string_compare.h"

namespace mcucore {

bool operator==(const Literal& a, const StringView& b) {
  return a.equal(b.data(), b.size());
}
bool operator==(const StringView& a, const Literal& b) { return b == a; }
bool ExactlyEqual(const Literal& a, const StringView& b) { return a == b; }

bool operator!=(const Literal& a, const StringView& b) { return !(a == b); }
bool operator!=(const StringView& a, const Literal& b) { return !(b == a); }

bool CaseEqual(const Literal& a, const StringView& b) {
  return a.case_equal(b.data(), b.size());
}
bool CaseEqual(const StringView& a, const Literal& b) {
  return CaseEqual(b, a);
}

bool StartsWith(const StringView& text, const Literal& prefix) {
  return prefix.is_prefix_of(text.data(), text.size());
}

bool operator==(const ProgmemStringView& a, const StringView& b) {
  return a.Equal(b.data(), b.size());
}
bool operator==(const StringView& a, const ProgmemStringView& b) {
  return b == a;
}
bool ExactlyEqual(const ProgmemStringView& a, const StringView& b) {
  return a == b;
}
bool operator!=(const ProgmemStringView& a, const StringView& b) {
  return !(a == b);
}
bool operator!=(const StringView& a, const ProgmemStringView& b) {
  return !(b == a);
}
bool CaseEqual(const ProgmemStringView& a, const StringView& b) {
  return a.CaseEqual(b.data(), b.size());
}
bool CaseEqual(const StringView& a, const ProgmemStringView& b) {
  return CaseEqual(b, a);
}
bool LoweredEqual(const ProgmemStringView& a, const StringView& b) {
  return a.LoweredEqual(b.data(), b.size());
}
bool StartsWith(const StringView& text, const ProgmemStringView& prefix) {
  return prefix.IsPrefixOf(text.data(), text.size());
}

}  // namespace mcucore