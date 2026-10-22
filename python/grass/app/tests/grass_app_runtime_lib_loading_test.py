# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests of making the GRASS C libraries (grass.lib) work without a session

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
    code = """
        import json

        import grass.lib.gis
        import grass.lib.raster

        print(json.dumps({"imported": True}))
    """
    assert run_code_in_environment(code, env=env)["imported"]
