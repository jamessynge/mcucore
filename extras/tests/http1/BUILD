# Tests of mcucore/src/http1/...

cc_test(
    name = "request_decoder_internals_test",
    srcs = ["request_decoder_internals_test.cc"],
    deps = [
        "//googletest:gunit_main",
        "//mcucore/src/http1:request_decoder",
    ],
)

cc_test(
    name = "request_decoder_test",
    srcs = ["request_decoder_test.cc"],
    deps = [
        "//absl/strings",
        "//googletest:gunit_main",
        "//mcucore/extras/test_tools:test_has_failed",
        "//mcucore/extras/test_tools/http1:collapsing_request_decoder_listener",
        "//mcucore/extras/test_tools/http1:mock_request_decoder_listener",
        "//mcucore/extras/test_tools/http1:string_utils",
        "//mcucore/src/http1:request_decoder",
        "//mcucore/src/http1:request_decoder_constants",
        "//mcucore/src/strings:string_view",
    ],
)
