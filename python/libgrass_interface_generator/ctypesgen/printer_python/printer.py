import os
import os.path
import sys
import time
import shutil

from ctypesgen.ctypedescs import CtypesBitfield, CtypesStruct
from ctypesgen.expressions import ExpressionNode
from ctypesgen.messages import error_message, status_message


THIS_DIR = os.path.dirname(__file__)
CTYPESGEN_DIR = os.path.join(THIS_DIR, os.path.pardir)
PREAMBLE_PATH = os.path.join(THIS_DIR, "preamble.py")
DEFAULTHEADER_PATH = os.path.join(THIS_DIR, "defaultheader.py")
LIBRARYLOADER_PATH = os.path.join(CTYPESGEN_DIR, "libraryloader.py")


class WrapperPrinter:
    def __init__(self, outpath, options, data):
        status_message("Writing to %s." % (outpath or "stdout"))

        self.file = open(outpath, "w") if outpath else sys.stdout
        self.options = options
        self.has_unnamed_struct_member = False

        if self.options.strip_build_path and self.options.strip_build_path[-1] != os.path.sep:
            self.options.strip_build_path += os.path.sep

        if not self.options.embed_preamble and outpath:
            self._copy_preamble_loader_files(outpath)

        self.print_header()
        self.file.write("\n")

        self.print_preamble()
        self.file.write("\n")

        self.print_loader()
        self.file.write("\n")

        self.print_group(self.options.libraries, "libraries", self.print_library)
        self.print_group(self.options.modules, "modules", self.print_module)

        method_table = {
            "function": self.print_function,
            "macro": self.print_macro,
            "struct": self.print_struct,
            "struct-body": self.print_struct_members,
            "typedef": self.print_typedef,
            "variable": self.print_variable,
            "enum": self.print_enum,
            "constant": self.print_constant,
            "undef": self.print_undef,
        }

        for kind, desc in data.output_order:
            if desc.included:
                method_table[kind](desc)
                self.file.write("\n")

        self.print_group(self.options.inserted_files, "inserted files", self.insert_file)
        self.strip_prefixes()

        if self.has_unnamed_struct_member and outpath:
            self._add_remove_zero_bitfields()

        self.file.close()

        if self.has_unnamed_struct_member and outpath and sys.executable:
            os.system("{0} {1}".format(sys.executable, outpath))

    def print_group(self, list, name, function):
        if list:
            self.file.write("# Begin %s\n" % name)
            for obj in list:
                function(obj)
            self.file.write("\n")
            self.file.write("# %d %s\n" % (len(list), name))
            self.file.write("# End %s\n" % name)
        else:
            self.file.write("# No %s\n" % name)
        self.file.write("\n")

    def srcinfo(self, src):
        if src is None:
            self.file.write("\n")
        else:
            filename, lineno = src
            if filename in ("<built-in>", "<command line>"):
                self.file.write("# %s\n" % filename)
            else:
                if self.options.strip_build_path and filename.startswith(
                    self.options.strip_build_path
                ):
                    filename = filename[len(self.options.strip_build_path) :]
                self.file.write("# %s: %s\n" % (filename, lineno))

    def template_subs(self):
        template_subs = {
            "date": time.ctime(),
            "argv": " ".join([x for x in sys.argv if not x.startswith("--strip-build-path")]),
            "name": os.path.basename(self.options.headers[0]),
        }

        for opt, value in self.options.__dict__.items():
            if type(value) == str:
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
                error_message(
                    'Cannot load header template from file "%s" '
                    " - using default template." % path,
                    cls="missing-file",
                )

        if not template_file:
            template_file = open(DEFAULTHEADER_PATH, "r")

        template_subs = self.template_subs()
        self.file.write(template_file.read() % template_subs)

        template_file.close()

    def print_preamble(self):
        self.file.write("# Begin preamble for Python\n\n")
        if self.options.embed_preamble:
            with open(PREAMBLE_PATH, "r") as preamble_file:
                preamble_file_content = preamble_file.read()
                filecontent = preamble_file_content.replace("# ~POINTER~", "")
                self.file.write(filecontent)
        else:
            self.file.write("from .ctypes_preamble import *\n")
            self.file.write("from .ctypes_preamble import _variadic_function\n")

        self.file.write("\n# End preamble\n")

    def _copy_preamble_loader_files(self, path):
        if os.path.isfile(path):
            abspath = os.path.abspath(path)
            dst = os.path.dirname(abspath)
        else:
            error_message(
                "Cannot copy preamble and loader files",
                cls="missing-file",
            )
            return

        c_preamblefile = f"{dst}/ctypes_preamble.py"
        if os.path.isfile(c_preamblefile):
            return

        pointer = """def POINTER(obj):
    p = ctypes.POINTER(obj)

    # Convert None to a real NULL pointer to work around bugs
    # in how ctypes handles None on 64-bit platforms
    if not isinstance(p.from_param, classmethod):

        def from_param(cls, x):
            if x is None:
                return cls()
            else:
                return x

        p.from_param = classmethod(from_param)

    return p

"""

        with open(PREAMBLE_PATH) as preamble_file:
            preamble_file_content = preamble_file.read()
            filecontent = preamble_file_content.replace("# ~POINTER~", pointer)

        with open(c_preamblefile, "w") as f:
            f.write(filecontent)

        shutil.copy(LIBRARYLOADER_PATH, f"{dst}")
        os.rename(f"{dst}/libraryloader.py", f"{dst}/ctypes_loader.py")

    def print_loader(self):
        self.file.write("_libs = {}\n")
        self.file.write("_libdirs = %s\n\n" % self.options.compile_libdirs)
        self.file.write("# Begin loader\n\n")
        if self.options.embed_preamble:
            with open(LIBRARYLOADER_PATH, "r") as loader_file:
                self.file.write(loader_file.read())
        else:
            self.file.write("from .ctypes_loader import *\n")
        self.file.write("\n# End loader\n\n")
        self.file.write(
            "add_library_search_dirs([%s])"
            % ", ".join([repr(d) for d in self.options.runtime_libdirs])
        )
        self.file.write("\n")

    def print_library(self, library):
        self.file.write('_libs["%s"] = load_library("%s")\n' % (library, library))

    def print_module(self, module):
        self.file.write("from %s import *\n" % module)

    def print_constant(self, constant):
        self.file.write("%s = %s" % (constant.name, constant.value.py_string(False)))
        self.srcinfo(constant.src)

    def print_undef(self, undef):
        self.srcinfo(undef.src)
        self.file.write(
            "# #undef {macro}\n"
            "try:\n"
            "    del {macro}\n"
            "except NameError:\n"
            "    pass\n".format(macro=undef.macro.py_string(False))
        )

    def print_typedef(self, typedef):
        self.file.write("%s = %s" % (typedef.name, typedef.ctype.py_string()))
        self.srcinfo(typedef.src)

    def print_struct(self, struct):
        self.srcinfo(struct.src)
        base = {"union": "Union", "struct": "Structure"}[struct.variety]
        self.file.write("class %s_%s(%s):\n    pass\n" % (struct.variety, struct.tag, base))

    def print_struct_members(self, struct):
        if struct.opaque:
            return

        # is this supposed to be packed?
        if struct.attrib.get("packed", False):
            aligned = struct.attrib.get("aligned", [1])
            assert len(aligned) == 1, "cgrammar gave more than one arg for aligned attribute"
            aligned = aligned[0]
            if isinstance(aligned, ExpressionNode):
                # TODO: for non-constant expression nodes, this will fail:
                aligned = aligned.evaluate(None)
            self.file.write("{}_{}._pack_ = {}\n".format(struct.variety, struct.tag, aligned))

        # handle unnamed fields.
        unnamed_fields = []
        names = set([x[0] for x in struct.members])
        anon_prefix = "unnamed_"
        n = 1
        for mi in range(len(struct.members)):
            mem = list(struct.members[mi])
            if mem[0] is None:
                while True:
                    self.has_unnamed_struct_member = True
                    name = "%s%i" % (anon_prefix, n)
                    n += 1
                    if name not in names:
                        break
                mem[0] = name
                names.add(name)
                if type(mem[1]) is CtypesStruct:
                    unnamed_fields.append(name)
                struct.members[mi] = mem

        self.file.write("%s_%s.__slots__ = [\n" % (struct.variety, struct.tag))
        for name, ctype in struct.members:
            skip_unnamed = (
                "#unnamedbitfield_{0} ".format(struct.tag) if name.startswith(anon_prefix) else ""
            )
            self.file.write("    {0}'{1}',\n".format(skip_unnamed, name))
        self.file.write("]\n")

        if len(unnamed_fields) > 0:
            self.file.write("%s_%s._anonymous_ = [\n" % (struct.variety, struct.tag))
            for name in unnamed_fields:
                self.file.write("    '%s',\n" % name)
            self.file.write("]\n")

        self.file.write("%s_%s._fields_ = [\n" % (struct.variety, struct.tag))
        for name, ctype in struct.members:
            if isinstance(ctype, CtypesBitfield):
                skip_unnamed = (
                    "#unnamedbitfield_{0} ".format(struct.tag)
                    if name.startswith(anon_prefix)
                    else ""
                )
                self.file.write(
                    "    {0}('{1}', {2}, {3}),\n".format(
                        skip_unnamed, name, ctype.py_string(), ctype.bitfield.py_string(False)
                    )
                )
            else:
                self.file.write("    ('%s', %s),\n" % (name, ctype.py_string()))
        self.file.write("]\n")

    def print_enum(self, enum):
        self.file.write("enum_%s = c_int" % enum.tag)
        self.srcinfo(enum.src)
        # Values of enumerator are output as constants.

    def print_function(self, function):
        if function.variadic:
            self.print_variadic_function(function)
        else:
            self.print_fixed_function(function)

    def print_fixed_function(self, function):
        self.srcinfo(function.src)

        CC = "stdcall" if function.attrib.get("stdcall", False) else "cdecl"

        # If we know what library the function lives in, look there.
        # Otherwise, check all the libraries.
        if function.source_library:
            self.file.write(
                'if _libs["{L}"].has("{CN}", "{CC}"):\n'
                '    {PN} = _libs["{L}"].get("{CN}", "{CC}")\n'.format(
                    L=function.source_library, CN=function.c_name(), PN=function.py_name(), CC=CC
                )
            )
        else:
            self.file.write(
                "for _lib in _libs.values():\n"
                '    if not _lib.has("{CN}", "{CC}"):\n'
                "        continue\n"
                '    {PN} = _lib.get("{CN}", "{CC}")\n'.format(
                    CN=function.c_name(), PN=function.py_name(), CC=CC
                )
            )

        # Argument types
        self.file.write(
            "    %s.argtypes = [%s]\n"
            % (function.py_name(), ", ".join([a.py_string() for a in function.argtypes]))
        )

        # Return value
        if function.restype.py_string() == "String":
            self.file.write(
                "    if sizeof(c_int) == sizeof(c_void_p):\n"
                "        {PN}.restype = ReturnString\n"
                "    else:\n"
                "        {PN}.restype = {RT}\n"
                "        {PN}.errcheck = ReturnString\n".format(
                    PN=function.py_name(), RT=function.restype.py_string()
                )
            )
        else:
            self.file.write(
                "    %s.restype = %s\n" % (function.py_name(), function.restype.py_string())
            )
            if function.errcheck:
                self.file.write(
                    "    %s.errcheck = %s\n" % (function.py_name(), function.errcheck.py_string())
                )

        if not function.source_library:
            self.file.write("    break\n")

    def print_variadic_function(self, function):
        CC = "stdcall" if function.attrib.get("stdcall", False) else "cdecl"

        self.srcinfo(function.src)
        if function.source_library:
            self.file.write(
                'if _libs["{L}"].has("{CN}", "{CC}"):\n'
                '    _func = _libs["{L}"].get("{CN}", "{CC}")\n'
                "    _restype = {RT}\n"
                "    _errcheck = {E}\n"
                "    _argtypes = [{t0}]\n"
                "    {PN} = _variadic_function(_func,_restype,_argtypes,_errcheck)\n".format(
                    L=function.source_library,
                    CN=function.c_name(),
                    RT=function.restype.py_string(),
                    E=function.errcheck.py_string(),
                    t0=", ".join([a.py_string() for a in function.argtypes]),
                    PN=function.py_name(),
                    CC=CC,
                )
            )
        else:
            self.file.write(
                "for _lib in _libs.values():\n"
                '    if _lib.has("{CN}", "{CC}"):\n'
                '        _func = _lib.get("{CN}", "{CC}")\n'
                "        _restype = {RT}\n"
                "        _errcheck = {E}\n"
                "        _argtypes = [{t0}]\n"
                "        {PN} = _variadic_function(_func,_restype,_argtypes,_errcheck)\n".format(
                    CN=function.c_name(),
                    RT=function.restype.py_string(),
                    E=function.errcheck.py_string(),
                    t0=", ".join([a.py_string() for a in function.argtypes]),
                    PN=function.py_name(),
                    CC=CC,
                )
            )

    def print_variable(self, variable):
        self.srcinfo(variable.src)
        if variable.source_library:
            self.file.write(
                "try:\n"
                '    {PN} = ({PS}).in_dll(_libs["{L}"], "{CN}")\n'
                "except:\n"
                "    pass\n".format(
                    PN=variable.py_name(),
                    PS=variable.ctype.py_string(),
                    L=variable.source_library,
                    CN=variable.c_name(),
                )
            )
        else:
            self.file.write(
                "for _lib in _libs.values():\n"
                "    try:\n"
                '        {PN} = ({PS}).in_dll(_lib, "{CN}")\n'
                "        break\n"
                "    except:\n"
                "        pass\n".format(
                    PN=variable.py_name(), PS=variable.ctype.py_string(), CN=variable.c_name()
                )
            )

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
        self.file.write(
            "try:\n"
            "    {MN} = {ME}\n"
            "except:\n"
            "    pass\n".format(MN=macro.name, ME=macro.expr.py_string(True))
        )

    def print_func_macro(self, macro):
        self.srcinfo(macro.src)
        self.file.write(
            "def {MN}({MP}):\n"
            "    return {ME}\n".format(
                MN=macro.name, MP=", ".join(macro.params), ME=macro.expr.py_string(True)
            )
        )

    def strip_prefixes(self):
        if not self.options.strip_prefixes:
            self.file.write("# No prefix-stripping\n\n")
            return

        self.file.write(
            "# Begin prefix-stripping\n"
            "\n"
            "# Strip prefixes from all symbols following regular expression:\n"
            "# {expr}\n"
            "\n"
            "import re as __re_module\n"
            "\n"
            "__strip_expr = __re_module.compile('{expr}')\n"
            "for __k, __v in globals().copy().items():\n"
            "    __m = __strip_expr.match(__k)\n"
            "    if __m:\n"
            "        globals()[__k[__m.end():]] = __v\n"
            "        # remove symbol with prefix(?)\n"
            "        # globals().pop(__k)\n"
            "del __re_module, __k, __v, __m, __strip_expr\n"
            "\n"
            "# End prefix-stripping\n"
            "\n".format(expr="({})".format("|".join(self.options.strip_prefixes)))
        )

    def insert_file(self, filename):
        try:
            inserted_file = open(filename, "r")
        except IOError:
            error_message('Cannot open file "%s". Skipped it.' % filename, cls="missing-file")

        self.file.write(
            '# Begin "{filename}"\n'
            "\n{file}\n"
            '# End "{filename}"\n'.format(filename=filename, file=inserted_file.read())
        )

        inserted_file.close()

    def _add_remove_zero_bitfields(self):
        self.file.write(
            "#REMOVE_START\n"
            "def main():\n"
            "    zero_bitfield_list = list()\n"
            "    filename = os.path.abspath(__file__)\n"
            "\n"
            '    with open(filename, "r") as f:\n'
            "        regex = re.compile(\n"
            r'            r"([\s]*)(\#unnamedbitfield)_"'
            "\n"
            r'            r"(?P<struct_name>[a-zA-Z_].[a-zA-Z0-9_]*)\s(?P<expr>.*)\,"'
            "\n"
            "        )\n"
            "        for line in f:\n"
            "            m = regex.match(line)\n"
            "            if m:\n"
            '                struct_name = m.group("struct_name")\n'
            '                bitfield_expression = tuple(eval(m.group("expr")))\n'
            "\n"
            "                if len(bitfield_expression) == 3 and bitfield_expression[2] == 0:\n"
            "                    member = bitfield_expression[0]\n"
            "                    zero_bitfield_list.append((struct_name, member))\n"
            "\n"
            '    with open(filename, "r+") as f:\n'
            "        filedata = f.read()\n"
            "\n"
            "        for (struct_name, member) in zero_bitfield_list:\n"
            "            pat = re.compile(\n"
            r"""                r"( *)#unnamedbitfield_{0}( '| \('){1}.*\n".format("""
            "\n"
            "                    struct_name, member\n"
            "                )\n"
            "            )\n"
            '            filedata = pat.sub("", filedata)\n'
            "\n"
            r'        regex = re.compile(r"#REMOVE_START.*#REMOVE_END\n", re.DOTALL)'
            "\n"
            '        filedata = regex.sub("", filedata)\n'
            "\n"
            r"""        regex = re.compile(r"#unnamedbitfield_[^'\(]*")"""
            "\n"
            '        filedata = regex.sub("", filedata)\n'
            "\n"
            "        f.seek(0)\n"
            "        f.write(filedata)\n"
            "        f.truncate()\n"
            "\n"
            "\n"
            'if __name__ == "__main__":\n'
            "    main()\n"
            "#REMOVE_END\n"
        )
