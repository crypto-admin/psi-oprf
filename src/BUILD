# Copyright 2017 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

licenses(["notice"])  # 3-clause BSD

package(default_visibility = ["//visibility:public"])

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

# The following three rules demonstrate the usage of the cc_grpc_library rule in
# in a mode compatible with the native proto_library and cc_proto_library rules.


cc_binary(
    name = "psiServer",
    srcs = ["psiServer.cc",
            "psiReceiver.cc",
            "psiReceiver.h",
            "common.h",
            "common.cc",
    ],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//src/proto:ot_cc_grpc",
        "//src/crypto:ecc",
        # http_archive made this label available for binding
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_gflags_gflags//:gflags",
    ],
)

cc_binary(
    name = "psiClient",
    srcs = ["psiClient.cc",
            "psiSender.h",
        "psiSender.cc",
        "common.h",
        "common.cc",
        ],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//src/proto:ot_cc_grpc",
        "//src/crypto:ecc",
        # http_archive made this label available for binding
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_gflags_gflags//:gflags",
    ],
)