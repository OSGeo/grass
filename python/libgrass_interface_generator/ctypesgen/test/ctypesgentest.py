# -*- coding: ascii -*-
# vim:ts=4:sw=4:softtabstop=4:smarttab:expandtab
import os
import sys
import io
import optparse
import glob
import json

try:
    # should succeed for py3
    from importlib import reload as reload_module
except:
    reload_module = reload

# ensure that we can load the ctypesgen library
PACKAGE_DIR = os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir)
sys.path.insert(0, PACKAGE_DIR)
import ctypesgen

"""ctypesgentest is a simple module for testing ctypesgen on various C constructs. It consists of a
single function, test(). test() takes a string that represents a C header file, along with some
keyword arguments representing options. It processes the header using ctypesgen and returns a tuple
containing the resulting module object and the output that ctypesgen produced."""

# set redirect_stdout to False if using console based debugger like pdb
redirect_stdout = True


def test(header, **more_options):

    assert isinstance(header, str)
    with open("temp.h", "w") as f:
        f.write(header)

    options = ctypesgen.options.get_default_options()
    options.headers = ["temp.h"]
    for opt, val in more_options.items():
        setattr(options, opt, val)

    if redirect_stdout:
        # Redirect output
        sys.stdout = io.StringIO()

    # Step 1: Parse
    descriptions = ctypesgen.parser.parse(options.headers, options)

    # Step 2: Process
    ctypesgen.processor.process(descriptions, options)

    # Step 3: Print
    printer = None
    if options.output_language.startswith("py"):
        ctypesgen.printer_python.WrapperPrinter("temp.py", options, descriptions)

        # Load the module we have just produced
        module = __import__("temp")
        # import twice, this hack ensure that "temp" is force loaded
        # (there *must* be a better way to do this)
        reload_module(module)
        retval = module

    elif options.output_language == "json":
        # for ease and consistency with test results, we are going to cheat by
        # resetting the anonymous tag number
        ctypesgen.ctypedescs.last_tagnum = 0
        ctypesgen.printer_json.WrapperPrinter("temp.json", options, descriptions)
        with open("temp.json") as f:
            JSON = json.load(f)
        retval = JSON
    else:
        raise RuntimeError("No such output language `" + options.output_language + "'")

    if redirect_stdout:
        # Un-redirect output
        output = sys.stdout.getvalue()
        sys.stdout.close()
        sys.stdout = sys.__stdout__
    else:
        output = ""

    return retval, output


def cleanup(filepattern="temp.*"):
    fnames = glob.glob(filepattern)
    for fname in fnames:
        os.unlink(fname)
