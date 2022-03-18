"""
Main loop for ctypesgen.
"""

import optparse
import sys

from ctypesgen import (
    messages as msgs,
    options as core_options,
    parser as core_parser,
    printer_python,
    printer_json,
    processor,
    version,
)


def find_names_in_modules(modules):
    names = set()
    for module in modules:
        try:
            mod = __import__(module)
        except Exception:
            pass
        else:
            names.update(dir(mod))
    return names


def option_callback_W(option, opt, value, parser):
    # Options preceded by a "-Wl," are simply treated as though the "-Wl,"
    # is not there? I don't understand the purpose of this code...
    if len(value) < 4 or value[0:3] != "l,-":
        raise optparse.BadOptionError("not in '-Wl,<opt>' form: %s%s" % (opt, value))
    opt = value[2:]
    if opt not in ["-L", "-R", "--rpath"]:
        raise optparse.BadOptionError("-Wl option must be -L, -R" " or --rpath, not " + value[2:])
    # Push the linker option onto the list for further parsing.
    parser.rargs.insert(0, value)


def option_callback_libdir(option, opt, value, parser):
    # There are two sets of linker search paths: those for use at compile time
    # and those for use at runtime. Search paths specified with -L, -R, or
    # --rpath are added to both sets.
    parser.values.compile_libdirs.append(value)
    parser.values.runtime_libdirs.append(value)


def main(givenargs=None):
    usage = "usage: %prog [options] /path/to/header.h ..."
    op = optparse.OptionParser(usage=usage, version=version.VERSION_NUMBER)

    # Parameters
    op.add_option(
        "-o",
        "--output",
        dest="output",
        metavar="FILE",
        help="write wrapper to FILE [default stdout]",
    )
    op.add_option(
        "-l",
        "--library",
        dest="libraries",
        action="append",
        default=[],
        metavar="LIBRARY",
        help="link to LIBRARY",
    )
    op.add_option(
        "",
        "--include",
        dest="other_headers",
        action="append",
        default=[],
        metavar="HEADER",
        help="include system header HEADER (e.g. stdio.h or stdlib.h)",
    )
    op.add_option(
        "-m",
        "--module",
        "--link-module",
        action="append",
        dest="modules",
        metavar="MODULE",
        default=[],
        help="use symbols from Python module MODULE",
    )
    op.add_option(
        "-I",
        "--includedir",
        dest="include_search_paths",
        action="append",
        default=[],
        metavar="INCLUDEDIR",
        help="add INCLUDEDIR as a directory to search for headers",
    )
    op.add_option(
        "-W",
        action="callback",
        callback=option_callback_W,
        metavar="l,OPTION",
        type="str",
        help="where OPTION is -L, -R, or --rpath",
    )
    op.add_option(
        "-L",
        "-R",
        "--rpath",
        "--libdir",
        action="callback",
        callback=option_callback_libdir,
        metavar="LIBDIR",
        type="str",
        help="Add LIBDIR to the search path (both compile-time and run-time)",
    )
    op.add_option(
        "",
        "--compile-libdir",
        action="append",
        dest="compile_libdirs",
        metavar="LIBDIR",
        default=[],
        help="Add LIBDIR to the compile-time library search path.",
    )
    op.add_option(
        "",
        "--runtime-libdir",
        action="append",
        dest="runtime_libdirs",
        metavar="LIBDIR",
        default=[],
        help="Add LIBDIR to the run-time library search path.",
    )
    op.add_option(
        "",
        "--no-embed-preamble",
        action="store_false",
        dest="embed_preamble",
        default=True,
        help="Do not embed preamble and loader in output file. "
        "Defining --output as a file and --output-language to "
        "Python is a prerequisite.",
    )

    # Parser options
    op.add_option(
        "",
        "--cpp",
        dest="cpp",
        default="gcc -E",
        help="The command to invoke the c preprocessor, including any "
        "necessary options (default: gcc -E)",
    )
    op.add_option(
        "",
        "--allow-gnu-c",
        action="store_true",
        dest="allow_gnu_c",
        default=False,
        help="Specify whether to undefine the '__GNUC__' macro, "
        "while invoking the C preprocessor.\n"
        "(default: False. i.e. ctypesgen adds an implicit undefine using '-U __GNUC__'.)\n"
        "Specify this flag to avoid ctypesgen undefining '__GNUC__' as shown above.",
    )
    op.add_option(
        "-D",
        "--define",
        action="append",
        dest="cpp_defines",
        metavar="MACRO",
        default=[],
        help="Add a definition to the preprocessor via commandline",
    )
    op.add_option(
        "-U",
        "--undefine",
        action="append",
        dest="cpp_undefines",
        metavar="NAME",
        default=[],
        help="Instruct the preprocessor to undefine the specified macro via commandline",
    )
    op.add_option(
        "",
        "--save-preprocessed-headers",
        metavar="FILENAME",
        dest="save_preprocessed_headers",
        default=None,
        help="Save the preprocessed headers to the specified FILENAME",
    )
    op.add_option(
        "",
        "--optimize-lexer",
        dest="optimize_lexer",
        action="store_true",
        default=False,
        help="Run the lexer in optimized mode.  This mode requires write "
        "access to lextab.py file stored within the ctypesgen package.",
    )

    # Processor options
    op.add_option(
        "-a",
        "--all-headers",
        action="store_true",
        dest="all_headers",
        default=False,
        help="include symbols from all headers, including system headers",
    )
    op.add_option(
        "",
        "--builtin-symbols",
        action="store_true",
        dest="builtin_symbols",
        default=False,
        help="include symbols automatically generated by the preprocessor",
    )
    op.add_option(
        "",
        "--no-macros",
        action="store_false",
        dest="include_macros",
        default=True,
        help="Don't output macros.",
    )
    op.add_option(
        "",
        "--no-undefs",
        action="store_false",
        dest="include_undefs",
        default=True,
        help="Do not remove macro definitions as per #undef directives",
    )
    op.add_option(
        "-i",
        "--include-symbols",
        action="append",
        dest="include_symbols",
        metavar="REGEXPR",
        default=[],
        help="Regular expression for symbols to always include.  Multiple "
        "instances of this option will be combined into a single expression "
        "doing something like '(expr1|expr2|expr3)'.",
    )
    op.add_option(
        "-x",
        "--exclude-symbols",
        action="append",
        dest="exclude_symbols",
        metavar="REGEXPR",
        default=[],
        help="Regular expression for symbols to exclude.  Multiple instances "
        "of this option will be combined into a single expression doing "
        "something like '(expr1|expr2|expr3)'.",
    )
    op.add_option(
        "",
        "--no-stddef-types",
        action="store_true",
        dest="no_stddef_types",
        default=False,
        help="Do not support extra C types from stddef.h",
    )
    op.add_option(
        "",
        "--no-gnu-types",
        action="store_true",
        dest="no_gnu_types",
        default=False,
        help="Do not support extra GNU C types",
    )
    op.add_option(
        "",
        "--no-python-types",
        action="store_true",
        dest="no_python_types",
        default=False,
        help="Do not support extra C types built in to Python",
    )

    # Printer options
    op.add_option(
        "",
        "--header-template",
        dest="header_template",
        default=None,
        metavar="TEMPLATE",
        help="Use TEMPLATE as the header template in the output file.",
    )
    op.add_option(
        "",
        "--strip-build-path",
        dest="strip_build_path",
        default=None,
        metavar="BUILD_PATH",
        help="Strip build path from header paths in the wrapper file.",
    )
    op.add_option(
        "",
        "--insert-file",
        dest="inserted_files",
        default=[],
        action="append",
        metavar="FILENAME",
        help="Add the contents of FILENAME to the end of the wrapper file.",
    )
    op.add_option(
        "",
        "--output-language",
        dest="output_language",
        metavar="LANGUAGE",
        default="py",
        choices=("py", "py32", "py27", "py25", "json"),
        help="Choose output language (`py'[default], `py32', `py27', `py25', or "
        "`json').  The implementation for py32 does appear to be "
        "compatible down to at least Python2.7.15.  py25 and py27 are in "
        "any case _not_ compatible with >= Python3.  The default choice "
        "(py) attempts to select `py32', `py27', or `py25' based on the "
        "version of Python that runs this script.",
    )
    op.add_option(
        "-P",
        "--strip-prefix",
        dest="strip_prefixes",
        default=[],
        action="append",
        metavar="REGEXPR",
        help="Regular expression to match prefix to strip from all symbols.  "
        "Multiple instances of this option will be combined into a single "
        "expression doing something like '(expr1|expr2|expr3)'.",
    )

    # Error options
    op.add_option(
        "",
        "--all-errors",
        action="store_true",
        default=False,
        dest="show_all_errors",
        help="Display all warnings and errors even " "if they would not affect output.",
    )
    op.add_option(
        "",
        "--show-long-errors",
        action="store_true",
        default=False,
        dest="show_long_errors",
        help="Display long error messages " "instead of abbreviating error messages.",
    )
    op.add_option(
        "",
        "--no-macro-warnings",
        action="store_false",
        default=True,
        dest="show_macro_warnings",
        help="Do not print macro warnings.",
    )
    op.add_option(
        "",
        "--debug-level",
        dest="debug_level",
        default=0,
        type="int",
        help="Run ctypesgen with specified debug level (also applies to yacc parser)",
    )

    op.set_defaults(**core_options.default_values)

    (options, args) = op.parse_args(givenargs)
    options.headers = args

    # Figure out what names will be defined by imported Python modules
    options.other_known_names = find_names_in_modules(options.modules)

    # Required parameters
    if len(args) < 1:
        msgs.error_message("No header files specified", cls="usage")
        sys.exit(1)

    if len(options.libraries) == 0:
        msgs.warning_message("No libraries specified", cls="usage")

    # Check output language
    printer = None
    if options.output_language.startswith("py"):
        printer = printer_python.WrapperPrinter
    elif options.output_language == "json":
        printer = printer_json.WrapperPrinter
    else:
        msgs.error_message("No such output language `" + options.output_language + "'", cls="usage")
        sys.exit(1)

    # Step 1: Parse
    descriptions = core_parser.parse(options.headers, options)

    # Step 2: Process
    processor.process(descriptions, options)

    # Step 3: Print
    printer(options.output, options, descriptions)

    msgs.status_message("Wrapping complete.")

    # Correct what may be a common mistake
    if descriptions.all == []:
        if not options.all_headers:
            msgs.warning_message(
                "There wasn't anything of use in the "
                "specified header file(s). Perhaps you meant to run with "
                "--all-headers to include objects from included sub-headers? ",
                cls="usage",
            )
