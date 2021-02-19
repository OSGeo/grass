#!/usr/bin/env python3
from __future__ import print_function


import os
import sys
from . import test  # So we can find the path to local files in the printer package
import time

import ctypesgencore.libraryloader  # So we can get the path to it
from ctypesgencore.ctypedescs import *
from ctypesgencore.descriptions import *
from ctypesgencore.messages import *


def path_to_local_file(name, known_local_module=test):
    basedir = os.path.dirname(known_local_module.__file__)
    return os.path.join(basedir, name)


class WrapperPrinter:

    def __init__(self, outpath, options, data):
        status_message("Writing to %s." % outpath)

        self.file = open(outpath, "w")
        self.options = options

        if self.options.strip_build_path and \
                self.options.strip_build_path[-1] != os.path.sep:
            self.options.strip_build_path += os.path.sep

        self.print_header()
        print(file=self.file)

        self.print_preamble()
        print(file=self.file)

        self.print_loader()
        print(file=self.file)

        self.print_group(self.options.libraries,
                         "libraries", self.print_library)
        self.print_group(self.options.modules, "modules", self.print_module)

        method_table = {
            'function': self.print_function,
            'macro': self.print_macro,
            'struct': self.print_struct,
            'struct-body': self.print_struct_members,
            'typedef': self.print_typedef,
            'variable': self.print_variable,
            'enum': self.print_enum,
            'constant': self.print_constant
        }

        for kind, desc in data.output_order:
            if desc.included:
                method_table[kind](desc)
                print(file=self.file)

        self.print_group(self.options.inserted_files, "inserted files",
                         self.insert_file)

    def print_group(self, list, name, function):
        if list:
            print("# Begin %s" % name, file=self.file)
            print(file=self.file)
            for obj in list:
                function(obj)
            print(file=self.file)
            print("# %d %s" % (len(list), name), file=self.file)
            print("# End %s" % name, file=self.file)
        else:
            print("# No %s" % name, file=self.file)
        print(file=self.file)

    def srcinfo(self, src):
        if src is None:
            print(file=self.file)
        else:
            filename, lineno = src
            if filename in ("<built-in>", "<command line>"):
                print("# %s" % filename, file=self.file)
            else:
                if self.options.strip_build_path and \
                        filename.startswith(self.options.strip_build_path):
                    filename = filename[len(self.options.strip_build_path):]
                print("# %s: %s" % (filename, lineno), file=self.file)

    def template_subs(self):
        template_subs = {
            'date': time.ctime(),
            'argv': ' '.join([x for x in sys.argv if not x.startswith("--strip-build-path")]),
            'name': os.path.basename(self.options.headers[0])
        }

        for opt, value in self.options.__dict__.items():
            if isinstance(value, str):
                template_subs[opt] = value
            elif isinstance(value, (list, tuple)):
                template_subs[opt] = (os.path.sep).join(value)
            else:
                template_subs[opt] = repr(value)

        return template_subs

    def print_header(self):
        template_file = None

        if self.options.header_template:
            path = self.options.header_template
            try:
                template_file = open(path, "r")
            except IOError:
                error_message("Cannot load header template from file \"%s\" "
                              " - using default template." % path, cls='missing-file')

        if not template_file:
            path = path_to_local_file("defaultheader.py")
            template_file = open(path, "r")

        template_subs = self.template_subs()
        self.file.write(template_file.read() % template_subs)

        template_file.close()

    def print_preamble(self):
        path = path_to_local_file("preamble.py")

        print("# Begin preamble", file=self.file)
        print(file=self.file)
        preamble_file = open(path, "r")
        self.file.write(preamble_file.read())
        preamble_file.close()
        print(file=self.file)
        print("# End preamble", file=self.file)

    def print_loader(self):
        print("_libs = {}", file=self.file)
        print("_libdirs = %s" % self.options.compile_libdirs, file=self.file)
        print(file=self.file)
        print("# Begin loader", file=self.file)
        print(file=self.file)
        path = path_to_local_file("libraryloader.py",
                                  ctypesgencore.libraryloader)
        loader_file = open(path, "r")
        self.file.write(loader_file.read())
        loader_file.close()
        print(file=self.file)
        print("# End loader", file=self.file)
        print(file=self.file)
        print("add_library_search_dirs([%s])" %
              ", ".join([repr(d) for d in self.options.runtime_libdirs]), file=self.file)

    def print_library(self, library):
        print('_libs["%s"] = load_library("%s")' % (library, library), file=self.file)

    def print_module(self, module):
        print('from %s import *' % name, file=self.file)

    def print_constant(self, constant):
        print('%s = %s' %
              (constant.name, constant.value.py_string(False)), end=' ', file=self.file)
        self.srcinfo(constant.src)

    def print_typedef(self, typedef):
        print('%s = %s' %
              (typedef.name, typedef.ctype.py_string()), end=' ', file=self.file)
        self.srcinfo(typedef.src)

    def print_struct(self, struct):
        self.srcinfo(struct.src)
        base = {'union': 'Union', 'struct': 'Structure'}[struct.variety]
        print('class %s_%s(%s):' %
              (struct.variety, struct.tag, base), file=self.file)
        print('    pass', file=self.file)

    def print_struct_members(self, struct):
        if struct.opaque:
            return

        # is this supposed to be packed?
        if struct.packed:
            print('{}_{}._pack_ = 1'.format(struct.variety, struct.tag),
                  file=self.file)

        # handle unnamed fields.
        unnamed_fields = []
        names = set([x[0] for x in struct.members])
        anon_prefix = "unnamed_"
        n = 1
        for mi in range(len(struct.members)):
            mem = list(struct.members[mi])
            if mem[0] is None:
                while True:
                    name = "%s%i" % (anon_prefix, n)
                    n += 1
                    if name not in names:
                        break
                mem[0] = name
                names.add(name)
                if type(mem[1]) is CtypesStruct:
                    unnamed_fields.append(name)
                struct.members[mi] = mem

        print('%s_%s.__slots__ = [' % (struct.variety, struct.tag), file=self.file)
        for name, ctype in struct.members:
            print("    '%s'," % name, file=self.file)
        print(']', file=self.file)

        if len(unnamed_fields) > 0:
            print ('%s_%s._anonymous_ = [' % (struct.variety,
                                              struct.tag), file=self.file)
            for name in unnamed_fields:
                print ("    '%s'," % name, file=self.file)
            print (']', file=self.file)

        print('%s_%s._fields_ = [' % (struct.variety, struct.tag), file=self.file)
        for name, ctype in struct.members:
            if isinstance(ctype, CtypesBitfield):
                print("    ('%s', %s, %s)," %
                      (name, ctype.py_string(), ctype.bitfield.py_string(False)), file=self.file)
            else:
                print("    ('%s', %s)," % (name, ctype.py_string()), file=self.file)
        print(']', file=self.file)

    def print_enum(self, enum):
        print('enum_%s = c_int' % enum.tag, end=' ', file=self.file)
        self.srcinfo(enum.src)
        # Values of enumerator are output as constants.

    def print_function(self, function):
        if function.variadic:
            self.print_variadic_function(function)
        else:
            self.print_fixed_function(function)

    def print_fixed_function(self, function):
        self.srcinfo(function.src)

        # If we know what library the function lives in, look there.
        # Otherwise, check all the libraries.
        if function.source_library:
            print("if hasattr(_libs[%r], %r):" %
                  (function.source_library, function.c_name()), file=self.file)
            print("    %s = _libs[%r].%s" %
                  (function.py_name(), function.source_library, function.c_name()), file=self.file)
        else:
            print("for _lib in six.itervalues(_libs):", file=self.file)
            print("    if not hasattr(_lib, %r):" % function.c_name(), file=self.file)
            print("        continue", file=self.file)
            print("    %s = _lib.%s" %
                  (function.py_name(), function.c_name()), file=self.file)

        # Argument types
        print("    %s.argtypes = [%s]" % (function.py_name(),
                                          ', '.join([a.py_string() for a in function.argtypes])), file=self.file)

        # Return value
        if function.restype.py_string() == "String":
            print("    if sizeof(c_int) == sizeof(c_void_p):", file=self.file)
            print("        %s.restype = ReturnString" %
                  (function.py_name()), file=self.file)
            print("    else:", file=self.file)
            print("        %s.restype = %s" %
                  (function.py_name(), function.restype.py_string()), file=self.file)
            print("        %s.errcheck = ReturnString" %
                  (function.py_name()), file=self.file)
        else:
            print("    %s.restype = %s" %
                  (function.py_name(), function.restype.py_string()), file=self.file)
            if function.errcheck:
                print ("    %s.errcheck = %s" %
                       (function.py_name(), function.errcheck.py_string()), file=self.file)

        if not function.source_library:
            print("    break", file=self.file)

    def print_variadic_function(self, function):
        self.srcinfo(function.src)
        if function.source_library:
            print("if hasattr(_libs[%r], %r):" %
                  (function.source_library, function.c_name()), file=self.file)
            print("    _func = _libs[%r].%s" %
                  (function.source_library, function.c_name()), file=self.file)
            print("    _restype = %s" % function.restype.py_string(), file=self.file)
            print("    _errcheck = %s" % function.errcheck.py_string(), file=self.file)
            print("    _argtypes = [%s]" %
                  ', '.join([a.py_string() for a in function.argtypes]), file=self.file)
            print("    %s = _variadic_function(_func,_restype,_argtypes,_errcheck)" %
                  function.py_name(), file=self.file)
        else:
            print("for _lib in _libs.values():", file=self.file)
            print("    if hasattr(_lib, %r):" % function.c_name(), file=self.file)
            print("        _func = _lib.%s" %
                  (function.c_name()), file=self.file)
            print("        _restype = %s" % function.restype.py_string(), file=self.file)
            print("        _errcheck = %s" % function.errcheck.py_string(), file=self.file)
            print("        _argtypes = [%s]" %
                  ', '.join([a.py_string() for a in function.argtypes]), file=self.file)
            print("        %s = _variadic_function(_func,_restype,_argtypes,_errcheck)" %
                  function.py_name(), file=self.file)

    def print_variable(self, variable):
        self.srcinfo(variable.src)
        if variable.source_library:
            print('try:', file=self.file)
            print('    %s = (%s).in_dll(_libs[%r], %r)' %
                  (variable.py_name(),
                   variable.ctype.py_string(),
                   variable.source_library,
                   variable.c_name()), file=self.file)
            print('except:', file=self.file)
            print('    pass', file=self.file)
        else:
            print("for _lib in _libs.values():", file=self.file)
            print('    try:', file=self.file)
            print('        %s = (%s).in_dll(_lib, %r)' %
                  (variable.py_name(),
                   variable.ctype.py_string(),
                   variable.c_name()), file=self.file)
            print("        break", file=self.file)
            print('    except:', file=self.file)
            print('        pass', file=self.file)

    def print_macro(self, macro):
        if macro.params:
            self.print_func_macro(macro)
        else:
            self.print_simple_macro(macro)

    def print_simple_macro(self, macro):
        # The macro translator makes heroic efforts but it occasionally fails.
        # We want to contain the failures as much as possible.
        # Hence the try statement.
        self.srcinfo(macro.src)
        print("try:", file=self.file)
        print("    %s = %s" % (macro.name, macro.expr.py_string(True)), file=self.file)
        print("except:", file=self.file)
        print("    pass", file=self.file)

    def print_func_macro(self, macro):
        self.srcinfo(macro.src)
        print("def %s(%s):" %
              (macro.name, ", ".join(macro.params)), file=self.file)
        print("    return %s" % macro.expr.py_string(True), file=self.file)

    def insert_file(self, filename):
        try:
            inserted_file = open(filename, "r")
        except IOError:
            error_message("Cannot open file \"%s\". Skipped it." % filename,
                          cls='missing-file')

        print("# Begin \"%s\"" % filename, file=self.file)
        print(file=self.file)
        self.file.write(inserted_file.read())
        print(file=self.file)
        print("# End \"%s\"" % filename, file=self.file)

        inserted_file.close()
