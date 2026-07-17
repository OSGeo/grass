# SPDX-License-Identifier: GPL-2.0-or-later

import os
from io import StringIO
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools

# One line with three vertices about 1 km apart, so that the default
# smax (10000 m) adds no vertices during reprojection.
LINE_ASCII = """\
L 3 1
 10.00 50.00
 10.01 50.01
 10.02 50.00
 1 1
"""


@pytest.fixture
def reprojection_session(tmp_path):
    """Active session in an EPSG:3857 project, with a lat/lon source
    project next to it containing a three-vertex line vector named "line".
    """
    source = "source_ll"
    gs.create_project(tmp_path, source, epsg="4326")
    with gs.setup.init(tmp_path / source, env=os.environ.copy()) as session:
        Tools(session=session).v_in_ascii(
            input=StringIO(LINE_ASCII), output="line", format="standard", flags="n"
        )
    target = tmp_path / "target_3857"
    gs.create_project(target, epsg="3857")
    with gs.setup.init(target, env=os.environ.copy()) as session:
        yield SimpleNamespace(session=session, source=source)
