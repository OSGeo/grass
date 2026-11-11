# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests of using grass.lib in a session set up by grass.script.setup.init

This file is meant to be executed also standalone (pytest with this file as
the only argument). The tests run their code in a subprocess without the
dynamic library search path variable, so finding the libraries depends only
on the variables init sets in the environment of the running process and on
the search path recorded in the libraries at build time.
"""

import sys
from textwrap import dedent

import pytest

WINDOWS = sys.platform.startswith("win")


@pytest.mark.usefixtures("mock_no_session")
def test_grass_lib_importable_after_init(
    tmp_path, run_code_in_environment, clean_subprocess_environment
):
    """Check that grass.lib modules can be imported after init"""
    project = tmp_path / "test"
    code = f"""
        import json
        import grass.script as gs

        gs.create_project(r"{project}")
        with gs.setup.init(r"{project}"):
            import grass.lib.gis
            import grass.lib.raster
        print(json.dumps({{"imported": True}}))
    """
    assert run_code_in_environment(code, env=clean_subprocess_environment)["imported"]


@pytest.mark.xfail(
    WINDOWS,
    strict=True,
    reason="On Windows, the C libraries (built with a different C runtime than"
    " Python) do not see the environment variables set by init in the running"
    " process, so they do not see the session",
)
@pytest.mark.usefixtures("mock_no_session")
def test_grass_lib_usable_after_init(
    tmp_path, run_code_in_environment, clean_subprocess_environment
):
    """Check that grass.lib sees the session and works with rasters after init"""
    project = tmp_path / "test"
    code = f"""
        import json
        import grass.script as gs

        gs.create_project(r"{project}")
        with gs.setup.init(r"{project}"):
            import grass.lib.gis as libgis
            import grass.lib.raster as libraster

            libgis.G_gisinit(b"test")
            gs.run_command("g.region", rows=2, cols=2)
            gs.mapcalc("ones = 1")
            fd = libraster.Rast_open_old(b"ones", b"")
            libraster.Rast_close(fd)
        print(json.dumps({{"raster_opened": True}}))
    """
    assert run_code_in_environment(code, env=clean_subprocess_environment)[
        "raster_opened"
    ]


@pytest.mark.usefixtures("mock_no_session")
def test_grass_lib_usable_in_subprocess_of_init(
    tmp_path, run_code_in_environment, clean_subprocess_environment
):
    """Check that grass.lib works in a process started from a session of init

    The session exists in the environment of the started process from its
    start, so the C libraries see it on all platforms, including Windows,
    where they do not see a session set up in their own process.
    """
    project = tmp_path / "test"
    child_file = tmp_path / "child.py"
    child_file.write_text(
        dedent(
            """
            import json

            import grass.lib.gis as libgis
            import grass.lib.raster as libraster

            libgis.G_gisinit(b"test")
            fd = libraster.Rast_open_old(b"ones", b"")
            libraster.Rast_close(fd)
            print(json.dumps({"raster_opened": True}))
            """
        )
    )
    code = f"""
        import subprocess
        import sys

        import grass.script as gs

        gs.create_project(r"{project}")
        with gs.setup.init(r"{project}"):
            gs.run_command("g.region", rows=2, cols=2)
            gs.mapcalc("ones = 1")
            result = subprocess.run(
                [sys.executable, r"{child_file}"],
                capture_output=True,
                text=True,
                check=True,
            )
        print(result.stdout)
    """
    assert run_code_in_environment(code, env=clean_subprocess_environment)[
        "raster_opened"
    ]


@pytest.mark.skipif(sys.platform != "linux", reason="Reads /proc/self/maps")
@pytest.mark.usefixtures("mock_no_session")
def test_session_does_not_load_libraries(
    tmp_path, run_code_in_environment, clean_subprocess_environment
):
    """Check that a session does not load the C libraries into the process

    The check reads /proc/self/maps, hence it runs only on Linux.
    """
    project = tmp_path / "test"
    code = f"""
        import json
        import grass.script as gs

        gs.create_project(r"{project}")
        with gs.setup.init(r"{project}"):
            pass
        with open("/proc/self/maps") as maps:
            mapped = [line for line in maps if "libgrass_" in line]
        print(json.dumps({{"mapped": mapped}}))
    """
    assert (
        run_code_in_environment(code, env=clean_subprocess_environment)["mapped"] == []
    )
