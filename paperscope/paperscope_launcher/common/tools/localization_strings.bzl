load("//third_party/bazel_rules/rules_cc/cc:cc_library.bzl", "cc_library")

def localization_strings_genrule(
        name,
        srcs,
        visibility,
        namespace,
        resource_file):
    """A genrule that creates C++ files for localized strings.

    This is a helper function that creates a genrule to generate localization
    string files from a set of resource files. It is used by the
    `localization_strings` macro.

    Args:
        name: The name of the target.
        srcs: A list of source files containing the localized strings.
        visibility: The visibility of the generated files.
        namespace: The namespace for the generated C++/Java/Objective-C code.
        resource_file: The path to the primary resource file (e.g.,
          en.lproj/cardboard.strings).
    """

    native.genrule(
        name = "gen_" + name,
        outs = [
            namespace + "_strings.h",
            namespace + "_strings.cc",
        ],
        srcs = srcs,
        cmd = ("$(location //vr/apps/cardboard/demo/common/tools:genstrings)" +
               " --resource_file=$(location %s)" % resource_file +
               " --namespace=" + namespace +
               " --out_dir=$(@D)" +
               " --language=cc"),
        tools = [
            "//vr/apps/cardboard/demo/common/tools:genstrings",
        ],
        visibility = visibility,
    )
    native.filegroup(
        name = "srcs_filegroup_" + name,
        srcs = [
            namespace + "_strings.cc",
        ],
        visibility = visibility,
    )
    native.filegroup(
        name = "hdrs_filegroup_" + name,
        srcs = [
            namespace + "_strings.h",
        ],
        visibility = visibility,
    )

def localization_strings(
        name,
        srcs,
        visibility,
        namespace,
        resource_file,
        copts = None):
    localization_strings_genrule(
        name,
        srcs,
        visibility,
        namespace,
        resource_file,
    )

    cc_library(
        name = name,
        srcs = [":srcs_filegroup_" + name],
        hdrs = [":hdrs_filegroup_" + name],
        copts = copts,
        visibility = visibility,
        deps = [
            "//vr/apps/cardboard/demo/common/strings:localized_strings",
        ],
    )
