# SPDX-License-Identifier: GPL-2.0-or-later
import io
import os

import pytest

import grass.script as gs
from grass.tools import Tools

# Loaded from stdin. Same data the original gunittest test used: every cell is
# 1..15, so no cell matches the declared null= value (no NULL cells result).
INPUT_FROM_STDIN = """north: 4299000.00
south: 4247000.00
east: 528000.00
west: 500000.00
rows: 10
cols: 15
null: -9999

1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15"""

# Same content as the original testsuite data file, including its trailing
# newline. A bare * marks a NULL cell (three of them).
INPUT_FROM_FILE = """north: 220542
south: 220528
east: 638492
west: 638478
rows: 7
cols: 7
3 2 5 3 5 3 4
1 4 5 5 5 * 4
2 1 * 3 5 5 2
4 2 4 4 4 5 4
4 4 2 * 5 2 4
1 2 1 1 2 2 2
5 4 1 2 3 4 2
"""


@pytest.fixture
def session(tmp_path):
    """An isolated GRASS session with no maps."""
    project = tmp_path / "r_in_ascii_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session


def test_read_ascii_from_stdin(session):
    """Load an ASCII grid from stdin; the declared null= value matches no cell."""
    tools = Tools(session=session)
    tools.g_region(
        n=4299000.00, s=4247000.00, e=528000.00, w=500000.00, rows=10, cols=15
    )

    tools.r_in_ascii(input=io.StringIO(INPUT_FROM_STDIN), output="ascii", type="CELL")

    info = tools.r_info(map="ascii", format="json").json
    assert info["min"] == 1
    assert info["max"] == 15

    univar = tools.r_univar(map="ascii", format="json").json
    assert univar["null_cells"] == 0


def test_read_ascii_with_null_character(session):
    """A bare * marks a cell as NULL (three cells here)."""
    tools = Tools(session=session)
    tools.g_region(n=220542, s=220528, e=638492, w=638478, rows=7, cols=7)

    tools.r_in_ascii(input=io.StringIO(INPUT_FROM_FILE), output="ascii", type="CELL")

    info = tools.r_info(map="ascii", format="json").json
    assert info["min"] == 1
    assert info["max"] == 5

    univar = tools.r_univar(map="ascii", format="json").json
    assert univar["null_cells"] == 3
