# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests of making the GRASS C libraries (grass.lib) work without a session

The loading done by the grass.lib package on import is tested here through
its private functions because grass.lib is generated and has no tests
directory of its own. Like the other tests, these run in a subprocess:
importing grass.lib loads the GRASS C libraries, which would otherwise stay
loaded in the test process.

This file is meant to be executed also standalone (pytest with this file as
the only argument), including against an installation moved away from the
prefix it was configured with: the tests provide any needed environment
themselves instead of relying on the environment of the test process.
"""

import os
import sys
from pathlib import Path

from grass.app.runtime import RuntimePaths
from grass.script.setup import get_install_path

IMPORT_CODE = """
    import json

    import grass.lib.gis
    import grass.lib.raster

    print(json.dumps({"imported": True}))
"""


def test_grass_lib_load_failure_raises_import_error(
    tmp_path, run_code_in_environment, clean_subprocess_environment
):
    """Check that a failure to load the core library is an import error"""
    lib_path = tmp_path / "lib"
    lib_path.mkdir()
    (lib_path / "libgrass_gis.so").write_text("This is not a shared library.")
    code = f"""
        import json

        import grass.lib

        try:
            grass.lib._load_libraries_if_needed(r"{lib_path}")
        except ImportError as error:
            print(json.dumps({{"message": str(error)}}))
        else:
            print(json.dumps({{"message": None}}))
    """
    message = run_code_in_environment(code, env=clean_subprocess_environment)["message"]
    assert message
    assert "libgrass_gis" in message


def test_grass_lib_loading_without_core_library_is_noop(
    tmp_path, run_code_in_environment, clean_subprocess_environment
):
    """Check that nothing is loaded without the core library"""
    lib_path = tmp_path / "lib"
    lib_path.mkdir()
    (lib_path / "libgrass_notalibrary.so").write_text("This is not a shared library.")
    code = f"""
        import json

        import grass.lib

        calls = []


        def record_call(lib_path):
            calls.append(lib_path)
            # The replaced function returns the list of load failures.
            return []


        grass.lib._preload_libraries = record_call
        grass.lib._load_libraries_if_needed(r"{lib_path}")
        print(json.dumps({{"calls": len(calls)}}))
    """
    assert run_code_in_environment(code, env=clean_subprocess_environment)["calls"] == 0


def test_preload_reports_libraries_which_cannot_be_loaded(
    tmp_path, run_code_in_environment, clean_subprocess_environment
):
    """Check that libraries which fail to load are reported, not raised"""
    lib_path = tmp_path / "lib"
    lib_path.mkdir()
    broken = lib_path / "libgrass_notalibrary.so"
    broken.write_text("This is not a shared library.")
    code = f"""
        import json
        import os

        import grass.lib

        failures = grass.lib._preload_libraries(r"{lib_path}")
        # Paths are compared by name because the function resolves them and
        # the temporary directory may be behind a symbolic link.
        print(json.dumps([os.path.basename(path) for path, _error in failures]))
    """
    assert run_code_in_environment(code, env=clean_subprocess_environment) == [
        broken.name
    ]


def test_grass_lib_importable_without_session(
    run_code_in_environment, clean_subprocess_environment
):
    """Check the import with neither a session nor the search path variable"""
    assert run_code_in_environment(IMPORT_CODE, env=clean_subprocess_environment)[
        "imported"
    ]


def test_grass_lib_importable_with_manual_search_path(
    run_code_in_environment, clean_subprocess_environment
):
    """Check that the search path variable set at process start is enough"""
    lib_path = os.fspath(Path(get_install_path()) / "lib")
    env = clean_subprocess_environment
    variables = [RuntimePaths().ld_library_path_var]
    if sys.platform == "darwin":
        # The variable GRASS knows on macOS (LD_RUN_PATH) is read by the
        # ctypes loader but ignored by dyld, which reads DYLD_LIBRARY_PATH.
        variables.append("DYLD_LIBRARY_PATH")
    for variable in variables:
        if env.get(variable):
            env[variable] = lib_path + os.pathsep + env[variable]
        else:
            env[variable] = lib_path
    assert run_code_in_environment(IMPORT_CODE, env=env)["imported"]
