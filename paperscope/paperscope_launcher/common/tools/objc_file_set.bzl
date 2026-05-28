"""Support for zipping a Fileset, retaining its directory structure."""

def _fileset_as_zip_impl(ctx):
    """Implementation of the FilesetAsZip rule.

    Args:
      ctx: The Skylark context.
    Returns:
      A list with the `DefaultInfo` provider that propagates a zip file for
      each `Fileset` in `srcs`.
    """
    srcs = ctx.files.srcs  # Must be fileset rules.
    outputs = []
    counter = 1

    for fileset in srcs:
        zipname = ctx.label.name + "." + str(counter) + ".zip"
        counter += 1

        output_zip = ctx.actions.declare_file(zipname)
        outputs.append(output_zip)

        message = "Generating %s archive" % (output_zip)
        sed_expr = "s|\\(%s/\\)\\(.*\\)|\\\\2=\\\\1\\\\2|g" % fileset.dirname

        # There's a //tools/zip with weird-ass args, but at least it's platform neutral.
        command_expr = '%s c %s $(find -L %s | sed "%s")' % (
            ctx.executable._zip.path,
            output_zip.path,
            fileset.path,
            sed_expr,
        )
        ctx.actions.run_shell(
            inputs = [ctx.executable._zip, fileset],
            outputs = [output_zip],
            command = command_expr,
            mnemonic = "EXPFilesetAsZipImpl",
            progress_message = "%s %s" % (message, ctx.label),
        )
    return [DefaultInfo(files = depset(outputs))]

fileset_as_zip = rule(
    implementation = _fileset_as_zip_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            mandatory = True,
        ),
        "_zip": attr.label(
            default = Label("//tools/zip:zipper"),
            cfg = "exec",
            allow_single_file = True,
            executable = True,
        ),
    },
)

def FilesetAsZip(name, srcs):
    fileset_as_zip(name = name, srcs = srcs)
