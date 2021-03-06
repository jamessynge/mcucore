#ifndef MCUCORE_EXTRAS_TEST_TOOLS_HTTP_REQUEST_H_
#define MCUCORE_EXTRAS_TEST_TOOLS_HTTP_REQUEST_H_

// Provides a simple class for building an HTTP request start line and headers.
//
// Author: james.synge@gmail.com

#include <map>
#include <string>

#include "absl/status/statusor.h"
#include "extras/test_tools/case_insensitive_less.h"

namespace mcucore {
namespace test {

struct HttpRequest {
  explicit HttpRequest(const std::string& path);
  HttpRequest(const std::string& method, const std::string& path);

  // Add common headers and parameters.
  void AddCommonParts();

  // Adds headers such as Content-Type and Content-Length.
  void AddCommonHeaders();

  void AddHeaderIfUnset(const std::string& name, const std::string& value);

  // Replaces any existing value for the named header. If the value is empty,
  // then ToString will NOT include it when constructing the request message.
  // This allows for preventing AddCommonHeaders or ToString from choosing to
  // set the "Content-Type" and/or "Content-Length" headers.
  void SetHeader(const std::string& name, const std::string& value);

  bool HasHeader(const std::string& name) const;

  // Adds parameters such as ClientID and ClientTransactionID.
  void AddCommonParameters();
  void AddParameterIfUnset(const std::string& name, const std::string& value);
  void SetParameter(const std::string& name, const std::string& value);
  bool HasParameter(const std::string& name) const;
  absl::StatusOr<std::string> GetParameter(const std::string& name) const;

  // Produces the string to be send to the server.
  std::string ToString();

  std::string method = "GET";
  std::string path;
  std::string version = "HTTP/1.1";
  std::map<std::string, std::string, mcucore::test::CaseInsensitiveLess>
      headers;
  std::map<std::string, std::string, mcucore::test::CaseInsensitiveLess>
      parameters;
};

}  // namespace test
}  // namespace mcucore

#endif  // MCUCORE_EXTRAS_TEST_TOOLS_HTTP_REQUEST_H_
