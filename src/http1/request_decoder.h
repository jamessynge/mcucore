#ifndef MCUCORE_SRC_HTTP1_REQUEST_DECODER_H_
#define MCUCORE_SRC_HTTP1_REQUEST_DECODER_H_

// http1::RequestDecoder is an HTTP/1.1 Request Message (but not body)
// decoder for tiny web servers, i.e. those intended to run on a very low-memory
// system, such as on a microcontroller. HTTP/1.1 is specified in RFC 9112
// (June, 2022), which obsoletes RFC 7230 (June, 2014).
//
//    https://www.rfc-editor.org/rfc/rfc9112.html
//
// The aim is to be able to decode most HTTP/1.1 requests, though with the
// limitation that it can't interpret 'tokens' that are longer than the maximum
// size buffer that the system can support (i.e. if the name of a header is
// longer than that limit, the listener will be informed of the issue, and the
// header name and value will be skipped over).
//
// This originated as RequestDecoder in TinyAlpacaServer, which was targeted at
// a very specific set of URL paths and header names.
//
// Because there is no standard format for the query string, processing it is
// delegated to the listener. However, given application/x-www-form-urlencoded
// is the most common format in which query strings are encoded, and is the
// manner in which form data is submitted from a browser (and is the encoded
// specified by the ASCOM Alpaca standard), a decoder is provided by
// WwwFormUrlEncodedDecoder.
//
// Similarly, there isn't a single format for header values, so parsing them is
// delegated to the listener.
//
// NOTE: While I was tempted to avoid using virtual functions in the listener
// API in order to avoid virtual-function-tables, which are stored in RAM when
// compiled by avr-gcc, I decided that doing so was likely to increase the
// complexity of this class (e.g. multiple listener methods, and perhaps
// multiple context pointers) and thus of the application using this class.
// Until I have evidence otherwise, I'll use a single listener, an instance of a
// class that requires virtual functions.
//
// Author: james.synge@gmail.com

#include "http1/request_decoder_constants.h"
#include "strings/progmem_string.h"
#include "strings/string_view.h"

// I don't normally want to have many nested namespaces, but the decoder has
// helper classes and several enum definitions, with names that I'd like to keep
// relatively short, and yet not readily conflict with other similar enums, so
// I've chosen to use nested namespaces for the decoder and helpers.
namespace mcucore {
namespace http1 {

// Forward declarations.
struct ActiveDecodingState;
struct RequestDecoderListener;

// The decoder will use an instance of one of the classes derived from
// BaseListenerCallbackData to provide data to the listener at key points in the
// decoding process.
struct BaseListenerCallbackData {
  explicit BaseListenerCallbackData(const ActiveDecodingState& state)
      : state(state) {}

  // Puts the decoder into an error state, with the effect that it stops
  // decoding and returns kInternalError.
  void StopDecoding() const;

  // Returns a view of the data passed into RequestDecoder::DecodeBuffer.
  StringView GetFullDecoderInput() const;

 protected:
  // Switch out of the mode where we decode the query string. The decoder will
  // instead pass the query string (if any) to the listener without decoding it
  // into parameter names and values, nor with any decoding of percent-encoded
  // characters. It is only valid to call this method when a kPathEndQueryStart
  // event is being handled by the listener.
  void SkipQueryStringDecoding() const;

 private:
  const ActiveDecodingState& state;
};

struct OnEventData : BaseListenerCallbackData {
  using BaseListenerCallbackData::SkipQueryStringDecoding;

  EEvent event;
};

struct OnPartialTextData : BaseListenerCallbackData {
  EPartialToken token;
  EPartialTokenPosition position;
  StringView text;
};

struct OnCompleteTextData : BaseListenerCallbackData {
  // This ctor exists only to help with implementing tests, in particular for
  // CollapsingRequestDecoderListener, which creates one OnCompleteTextData
  // instance for each string of related OnPartialText calls.
  explicit OnCompleteTextData(const BaseListenerCallbackData& partial_data)
      : BaseListenerCallbackData(partial_data) {}
  EToken token;
  StringView text;
};

struct OnErrorData : BaseListenerCallbackData {
  ProgmemString message;
  StringView undecoded_input;
};

// Note that percent encoding is not been removed from text before it is passed
// to the listener.
struct RequestDecoderListener {
  virtual ~RequestDecoderListener() {}

  // `data.event` has occurred; usually means that some particular fixed text
  // has been matched.
  virtual void OnEvent(const OnEventData& data) = 0;

  // `data.token` has been matched; it's complete value is in `data.text`.
  virtual void OnCompleteText(const OnCompleteTextData& data) = 0;

  // Some portion of `data.token` has been matched, with that portion in
  // `data.text`. The text may be empty, so far just for positions kFirst and
  // kLast.
  virtual void OnPartialText(const OnPartialTextData& data) = 0;

  // An error has occurred, as described in `data.message`.
  virtual void OnError(const OnErrorData& data) = 0;
};

class RequestDecoderImpl {
 public:
  using DecodeFunction = EDecodeBufferStatus (*)(ActiveDecodingState& state);

  RequestDecoderImpl();

  // Reset the decoder, ready to decode a new request.
  void Reset();

  // Decodes some or all of the contents of buffer, calling the specified
  // listener as entities are matched. If buffer_is_full is true, then it is
  // assumed that size represents the maximum amount of data that can be
  // provided at once.
  EDecodeBufferStatus DecodeBuffer(StringView& buffer,
                                   RequestDecoderListener& listener,
                                   bool buffer_is_full);

 private:
  friend class ActiveDecodingState;

  // Pointer to the next function to be used for decoding. Changes as different
  // entities in a request header is matched.
  DecodeFunction decode_function_;
};

// Decodes HTTP/1.1 request headers.
class RequestDecoder : /*private*/ RequestDecoderImpl {
 public:
  using RequestDecoderImpl::RequestDecoderImpl;

  using RequestDecoderImpl::DecodeBuffer;
  using RequestDecoderImpl::Reset;
};

// Placing these functions into namespace mcucore_http1_internal so they can be
// called by unit tests. They are not a part of the public API of the decoder.
namespace mcucore_http1_internal {

// Match characters allowed BY THIS DECODER in a method name.
bool IsMethodChar(const char c);

// Match characters allowed in a path segment.
bool IsPChar(const char c);
bool IsPCharExceptPercent(const char c);

// Match characters allowed in a query string.
bool IsQueryChar(const char c);

// Match characters allowed in the parameter name of a query string, excluding
// percent and plus, which are handled separately.
bool IsParamNameCharExceptPercentAndPlus(const char c);

// Match characters allowed in the parameter value of a query string.
bool IsParamValueCharExceptPercentAndPlus(const char c);

// Match characters allowed in a token (e.g. a header name).
bool IsTokenChar(const char c);

// Match characters allowed in a header value, per RFC7230, Section 3.2.6.
bool IsFieldContent(const char c);

// Return the index of the first character that doesn't match the test function.
// Returns StringView::kMaxSize if not found.
StringView::size_type FindFirstNotOf(const StringView& view,
                                     bool (*test)(char));

// Removes leading whitespace characters, returns true when the first character
// is not whitespace.
bool SkipLeadingOptionalWhitespace(StringView& view);

// Remove optional whitespace from the end of the view, which is non-empty.
// Returns true if optional whitespace was removed.
bool TrimTrailingOptionalWhitespace(StringView& view);

}  // namespace mcucore_http1_internal
}  // namespace http1
}  // namespace mcucore

#endif  // MCUCORE_SRC_HTTP1_REQUEST_DECODER_H_
