# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests of loading the C libraries into the process with init(load_libs=True)

This file is meant to be executed also standalone (pytest with this file as
the only argument) against an installation which was moved away from the
prefix it was configured with. Only then the GRASS libraries cannot find each
other through their RUNPATH and the tests actually fail without the loading.
"""

import json
import os
import subprocess
import sys
from textwrap import dedent

import pytest

import grass.app.runtime
import grass.script as gs


def run_in_clean_environment(code, tmp_path):
    """Run code in a subprocess without the dynamic library search path variable

    The variable is what a parent GRASS session or a manual setup uses to make
    the GRASS libraries available, so removing it leaves the loading done by
    init as the only mechanism which can make grass.lib work.
    """
    source_file = tmp_path / "code.py"
    source_file.write_text(dedent(code))
    env = os.environ.copy()
    env.pop("LD_LIBRARY_PATH", None)
    env.pop("DYLD_LIBRARY_PATH", None)
    result = subprocess.run(
        [sys.executable, os.fspath(source_file)],
        capture_output=True,
        text=True,
        check=False,
        env=env,
    )
    assert result.returncode == 0, result.stderr
    return json.loads(result.stdout)


@pytest.mark.usefixtures("mock_no_session")
def test_grass_lib_usable_with_load_libs(tmp_path):
    """Check that grass.lib is usable after init with load_libs"""
    project = tmp_path / "test"
    code = f"""
        import json
        import grass.script as gs

        gs.create_project(r"{project}")
        with gs.setup.init(r"{project}", load_libs=True):
            import grass.lib.gis as libgis
            import grass.lib.raster as libraster

            libgis.G_gisinit(b"test")
            gs.run_command("g.region", rows=2, cols=2)
            gs.mapcalc("ones = 1")
            fd = libraster.Rast_open_old(b"ones", b"")
            libraster.Rast_close(fd)
        print(json.dumps({{"raster_opened": True}}))
    """
    assert run_in_clean_environment(code, tmp_path=tmp_path)["raster_opened"]


@pytest.mark.usefixtures("mock_no_session")
def test_grass_lib_usable_with_load_libs_and_custom_env(tmp_path):
    """Check that grass.lib is usable with load_libs and a custom environment

    The grass.lib loader reads GISBASE from the global environment, so a
    session which keeps its variables in its own environment needs the path
    to the libraries passed to the loader directly.
    """
    project = tmp_path / "test"
    code = f"""
        import json
        import os
        import grass.script as gs

        gs.create_project(r"{project}")
        env = os.environ.copy()
        with gs.setup.init(r"{project}", env=env, load_libs=True):
            import grass.lib.gis as libgis

            libgis.G_gisinit(b"test")
        print(
            json.dumps(
                {{
                    "gis_init_worked": True,
                    "gisbase_in_global_env": "GISBASE" in os.environ,
                }}
            )
        )
    """
    result = run_in_clean_environment(code, tmp_path=tmp_path)
    assert result["gis_init_worked"]
    # The session did not fall back to the global environment for the lookup.
    assert not result["gisbase_in_global_env"]


@pytest.mark.usefixtures("mock_no_session")
def test_libraries_not_loaded_by_default(tmp_path, monkeypatch):
    """Check that a session does not load the C libraries unless asked to"""
    calls = []

    def record_call(install_path):
        calls.append(install_path)

    monkeypatch.setattr(grass.app.runtime, "preload_dynamic_libraries", record_call)
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()):
        pass
    assert not calls
    with gs.setup.init(project, env=os.environ.copy(), load_libs=True):
        pass
    assert calls


def test_preload_reports_libraries_which_cannot_be_loaded(tmp_path):
    """Check that libraries which fail to load are reported, not raised"""
    lib_path = tmp_path / "lib"
    lib_path.mkdir()
    broken = lib_path / "libgrass_notalibrary.so"
    broken.write_text("This is not a shared library.")
    failures = grass.app.runtime.preload_dynamic_libraries(install_path=tmp_path)
    if sys.platform.startswith("win"):
        assert failures == []
    else:
        # Paths are compared by name because the function resolves them and
        # the temporary directory may be behind a symbolic link.
        assert [path.name for path, _error in failures] == [broken.name]
