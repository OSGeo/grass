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

import pytest

from grass.app.runtime import RuntimePaths
from grass.script.setup import get_install_path

IMPORT_CODE = """
    import json

    import grass.lib.gis
    import grass.lib.raster

    print(json.dumps({"imported": True}))
"""


@pytest.mark.skipif(
    sys.platform.startswith("win"),
    reason="Windows does not load the libraries in advance",
)
def test_grass_lib_load_failure_raises_import_error(
    tmp_path, run_code_in_environment, clean_subprocess_environment
):
    """Check that a failure to load the core library is an import error"""
    lib_path = tmp_path / "lib"
    lib_path.mkdir()
    code = f"""
        import json
        from pathlib import Path

        import grass.lib

        # The file has the name under which the wrapper modules load the library.
        name = grass.lib.load_library.name_formats[0] % grass.lib._CORE_LIBRARY_NAME
        (Path(r"{lib_path}") / name).write_text("This is not a shared library.")
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
