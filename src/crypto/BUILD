
package(default_visibility = ["//visibility:public"])

PIR_DEFAULT_INCLUDES = ["."]

PIR_DEFAULT_COPTS = ["-std=c++17"]

cc_library(
    name = "ecc",
    srcs = [
        "sm2.cpp",
        "sm2.h",
        "sm3_hash.cpp",
        "sm3_hash.h",
        "sm3.cpp",
        "sm3.h",
        "sm4.cpp",
        "sm4.h",
        "stringutil.cpp",
        "stringutil.h",
    ],
    hdrs = [
    ],
    copts = PIR_DEFAULT_COPTS,
    includes = PIR_DEFAULT_INCLUDES,
    deps = [
    ],
)
