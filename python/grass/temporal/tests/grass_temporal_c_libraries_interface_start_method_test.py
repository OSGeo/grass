# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Test that GRASS server processes start when the default start method is not fork.

Python 3.14 made forkserver the default multiprocessing start method on Linux,
which broke the RPC servers in grass.pygrass and grass.temporal. The servers
now request a fork context explicitly. This test forces forkserver as the
default in a subprocess to reproduce the Python 3.14 situation on any version.
"""

import multiprocessing
import subprocess
import sys

import pytest

SCRIPT = """
import multiprocessing
import sys


def main():
    import grass.script as gs
    import grass.temporal as tgis
    from grass.pygrass.messages import Messenger
    from grass.pygrass.rpc import DataProvider
    from grass.pygrass.rpc.base import RPCServerBase

    project = sys.argv[1]
    gs.create_project(project)
    # Legacy init without env so that os.environ is set for grass.temporal.
    session = gs.setup.init(project)
    gs.run_command("g.region", n=10, s=0, e=10, w=0, res=1)
    gs.run_command("r.mapcalc", expression="a = 1")
    mapset = gs.gisenv()["MAPSET"]

    messenger = Messenger()
    messenger.message("test")
    messenger.stop()

    server = RPCServerBase()
    server.stop()

    provider = DataProvider()
    assert provider.get_raster_image_as_np("a").size > 0
    provider.stop()

    tgis.init()
    ciface = tgis.CLibrariesInterface()
    assert ciface.raster_map_exists("a", mapset)
    ciface.stop()

    session.finish()


if __name__ == "__main__":
    multiprocessing.set_start_method("forkserver")
    main()
"""


@pytest.mark.skipif(
    sys.platform == "darwin" or "fork" not in multiprocessing.get_all_start_methods(),
    reason="Fork is only requested where it is available and not macOS",
)
def test_servers_start_with_forkserver_default(tmp_path):
    """Servers work and exit cleanly when forkserver is the default start method."""
    script = tmp_path / "script.py"
    script.write_text(SCRIPT)
    result = subprocess.run(
        [sys.executable, str(script), str(tmp_path / "project")],
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr
    # A failing server check thread does not change the exit code.
    assert "Traceback" not in result.stderr, result.stderr
    assert "leaked semaphore" not in result.stderr, result.stderr
