licenses(["notice"])  # Apache 2

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_binary",
    "envoy_cc_library",
    "envoy_cc_test",
    "envoy_package",
)

envoy_package()

load("@envoy_api//bazel:api_build_system.bzl", "api_proto_library")

envoy_cc_binary(
    name = "envoy",
    repository = "@envoy",
    deps = [
        ":nats_streaming_filter_config_factory",
        "@envoy//source/exe:envoy_main_entry_lib",
    ],
)

api_proto_library(
    name = "nats_streaming_filter_proto",
    srcs = ["nats_streaming_filter.proto"],
)

envoy_cc_library(
    name = "nats_streaming_filter_config",
    hdrs = [
        "common/config/solo_well_known_names.h",
        "nats_streaming_filter_config.h",
    ],
    repository = "@envoy",
    deps = [
        ":nats_streaming_filter_proto_cc",
        "@envoy//source/exe:envoy_common_lib",
    ],
)

envoy_cc_library(
    name = "nats_streaming_filter_lib",
    srcs = [
        "metadata_subject_retriever.cc",
        "nats_streaming_filter.cc",
        "solo_filter_utility.cc",
    ],
    hdrs = [
        "metadata_subject_retriever.h",
        "nats_streaming_filter.h",
        "solo_filter_utility.h",
        "subject_retriever.h",
    ],
    repository = "@envoy",
    deps = [
        ":nats_streaming_filter_config",
        "//include/envoy/nats:publisher_interface",
        "@envoy//source/exe:envoy_common_lib",
    ],
)

envoy_cc_library(
    name = "nats_streaming_filter_config_factory",
    srcs = ["nats_streaming_filter_config_factory.cc"],
    hdrs = ["nats_streaming_filter_config_factory.h"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":nats_streaming_filter_lib",
        "//source/common/nats:publisher_lib",
        "@envoy//source/exe:envoy_common_lib",
    ],
)
