"""
All of the components of ctypegencore require an argument called "options".
In command-line usage, this would be an optparser.Values object. However, if
ctypesgen is used as a standard Python module, constructing this object
would be a pain. So this module exists to provide a "default" options object
for convenience.
"""

import optparse
import copy

default_values = {
    "other_headers": [],
    "modules": [],
    "include_search_paths": [],
    "compile_libdirs": [],
    "runtime_libdirs": [],
    "cpp": "gcc -E",
    "allow_gnu_c": False,
    "cpp_defines": [],
    "cpp_undefines": [],
    "save_preprocessed_headers": None,
    "all_headers": False,
    "builtin_symbols": False,
    "include_symbols": [],
    "exclude_symbols": [],
    "show_all_errors": False,
    "show_long_errors": False,
    "show_macro_warnings": True,
    "header_template": None,
    "inserted_files": [],
    "other_known_names": [],
    "include_macros": True,
    "include_undefs": True,
    "libraries": [],
    "strip_build_path": None,
    "output_language": "py",
    "no_stddef_types": False,
    "no_gnu_types": False,
    "no_python_types": False,
    "debug_level": 0,
    "strip_prefixes": [],
    "embed_preamble": True,
}


def get_default_options():
    return optparse.Values(copy.deepcopy(default_values))
