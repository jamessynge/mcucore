#include "extras/test_tools/json_test_utils.h"

#include <stddef.h>

#include "extras/host/arduino/print.h"
#include "json/json_encoder.h"
#include "print/any_printable.h"

namespace mcucore {
namespace test {

void AddArrayElement(JsonArrayEncoder& encoder,
                     const JsonElementSourceFunction& func) {
  ElementSourceFunctionAdapter source(func);
  encoder.AddArrayElement(source);
}

void AddObjectElement(JsonArrayEncoder& encoder,
                      const JsonPropertySourceFunction& func) {
  PropertySourceFunctionAdapter source(func);
  encoder.AddObjectElement(source);
}

// static
void JsonEncodeArray(const JsonElementSourceFunction& func, Print& out) {
  ElementSourceFunctionAdapter source(func);
  JsonArrayEncoder::Encode(source, out);
}

// static
size_t JsonEncodedArraySize(const JsonElementSourceFunction& func) {
  ElementSourceFunctionAdapter source(func);
  return JsonArrayEncoder::EncodedSize(source);
}

void AddArrayProperty(JsonObjectEncoder& encoder, const AnyPrintable& name,
                      const JsonElementSourceFunction& func) {
  ElementSourceFunctionAdapter source(func);
  encoder.AddArrayProperty(name, source);
}

void AddObjectProperty(JsonObjectEncoder& encoder, const AnyPrintable& name,
                       const JsonPropertySourceFunction& func) {
  PropertySourceFunctionAdapter source(func);
  encoder.AddObjectProperty(name, source);
}

// static
void JsonEncodeObject(const JsonPropertySourceFunction& func, Print& out) {
  PropertySourceFunctionAdapter source(func);
  JsonObjectEncoder::Encode(source, out);
}

// static
size_t JsonEncodedObjectSize(const JsonPropertySourceFunction& func) {
  PropertySourceFunctionAdapter source(func);
  return JsonObjectEncoder::EncodedSize(source);
}

}  // namespace test
}  // namespace mcucore
