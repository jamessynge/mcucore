#ifndef MCUCORE_EXTRAS_TEST_TOOLS_JSON_TEST_UTILS_H_
#define MCUCORE_EXTRAS_TEST_TOOLS_JSON_TEST_UTILS_H_

// To make it easier for tests to drive the JSON encoders, we provide these
// helpers.
//
// Author: james.synge@gmail.com

#include <stddef.h>

#include <functional>
#include <utility>

#include "json/json_encoder.h"
#include "mcucore_platform.h"
#include "print/any_printable.h"

namespace mcucore {
namespace test {

using JsonElementSourceFunction = std::function<void(JsonArrayEncoder&)>;
using JsonPropertySourceFunction = std::function<void(JsonObjectEncoder&)>;

class ElementSourceFunctionAdapter : public JsonElementSource {
 public:
  explicit ElementSourceFunctionAdapter(JsonElementSourceFunction func)
      : func_(std::move(func)) {}
  void AddTo(JsonArrayEncoder& encoder) const override { func_(encoder); }

 private:
  JsonElementSourceFunction func_;
};

class PropertySourceFunctionAdapter : public JsonPropertySource {
 public:
  explicit PropertySourceFunctionAdapter(JsonPropertySourceFunction func)
      : func_(std::move(func)) {}
  void AddTo(JsonObjectEncoder& encoder) const override { func_(encoder); }

 private:
  JsonPropertySourceFunction func_;
};

void AddArrayElement(JsonArrayEncoder& encoder,
                     const JsonElementSourceFunction& func);
void AddObjectElement(JsonArrayEncoder& encoder,
                      const JsonPropertySourceFunction& func);

void JsonEncodeArray(const JsonElementSourceFunction& func, Print& out);
size_t JsonEncodedArraySize(const JsonElementSourceFunction& func);

void AddArrayProperty(JsonObjectEncoder& encoder, const AnyPrintable& name,
                      const JsonElementSourceFunction& func);
void AddObjectProperty(JsonObjectEncoder& encoder, const AnyPrintable& name,
                       const JsonPropertySourceFunction& func);

void JsonEncodeObject(const JsonPropertySourceFunction& func, Print& out);
size_t JsonEncodedObjectSize(const JsonPropertySourceFunction& func);

}  // namespace test
}  // namespace mcucore

#endif  // MCUCORE_EXTRAS_TEST_TOOLS_JSON_TEST_UTILS_H_
