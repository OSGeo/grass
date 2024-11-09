import os
import sys
import json

from ctypesgen.ctypedescs import CtypesBitfield
from ctypesgen.messages import status_message


# From:
# https://stackoverflow.com/questions/1036409/recursively-convert-python-object-graph-to-dictionary
def todict(obj, classkey="Klass"):
    if isinstance(obj, dict):
        for k in obj.keys():
            obj[k] = todict(obj[k], classkey)
        return obj
    elif isinstance(obj, str) or isinstance(obj, bytes):
        # must handle strings before __iter__ test, since they now have
        # __iter__ in Python3
        return obj
    elif hasattr(obj, "__iter__"):
        return [todict(v, classkey) for v in obj]
    elif hasattr(obj, "__dict__"):
        data = dict(
            [
                (key, todict(value, classkey))
                for key, value in obj.__dict__.items()
                if not callable(value) and not key.startswith("_")
            ]
        )
        if classkey is not None and hasattr(obj, "__class__"):
            data[classkey] = obj.__class__.__name__
        return data
    else:
        return obj


class WrapperPrinter:
    def __init__(self, outpath, options, data):
        status_message("Writing to %s." % (outpath or "stdout"))

        self.file = open(outpath, "w") if outpath else sys.stdout
        self.options = options

        if self.options.strip_build_path and self.options.strip_build_path[-1] != os.path.sep:
            self.options.strip_build_path += os.path.sep

        self.print_group(self.options.libraries, "libraries", self.print_library)

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

        res = []
        for kind, desc in data.output_order:
            if desc.included:
                item = method_table[kind](desc)
                if item:
                    res.append(item)
        self.file.write(json.dumps(res, sort_keys=True, indent=4))
        self.file.write("\n")

    def __del__(self):
        self.file.close()

    def print_group(self, list, name, function):
        if list:
            return [function(obj) for obj in list]

    def print_library(self, library):
        return {"load_library": library}

    def print_constant(self, constant):
        return {"type": "constant", "name": constant.name, "value": constant.value.py_string(False)}

    def print_undef(self, undef):
        return {"type": "undef", "value": undef.macro.py_string(False)}

    def print_typedef(self, typedef):
        return {"type": "typedef", "name": typedef.name, "ctype": todict(typedef.ctype)}

    def print_struct(self, struct):
        res = {"type": struct.variety, "name": struct.tag, "attrib": struct.attrib}
        if not struct.opaque:
            res["fields"] = []
            for name, ctype in struct.members:
                field = {"name": name, "ctype": todict(ctype)}
                if isinstance(ctype, CtypesBitfield):
                    field["bitfield"] = ctype.bitfield.py_string(False)
                res["fields"].append(field)
        return res

    def print_struct_members(self, struct):
        pass

    def print_enum(self, enum):
        res = {"type": "enum", "name": enum.tag}

        if not enum.opaque:
            res["fields"] = []
            for name, ctype in enum.members:
                field = {"name": name, "ctype": todict(ctype)}
                res["fields"].append(field)
        return res

    def print_function(self, function):
        res = {
            "type": "function",
            "name": function.c_name(),
            "variadic": function.variadic,
            "args": todict(function.argtypes),
            "return": todict(function.restype),
            "attrib": function.attrib,
        }
        if function.source_library:
            res["source"] = function.source_library
        return res

    def print_variable(self, variable):
        res = {"type": "variable", "ctype": todict(variable.ctype), "name": variable.c_name()}
        if variable.source_library:
            res["source"] = variable.source_library
        return res

    def print_macro(self, macro):
        if macro.params:
            return {
                "type": "macro_function",
                "name": macro.name,
                "args": macro.params,
                "body": macro.expr.py_string(True),
            }
        else:
            # The macro translator makes heroic efforts but it occasionally fails.
            # Beware the contents of the value!
            return {"type": "macro", "name": macro.name, "value": macro.expr.py_string(True)}
