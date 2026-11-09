# SPDX-License-Identifier: GPL-2.0-or-later
import os

import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError
from grass.tools import Tools

# A fresh project's default mapset, used by every test through the session
# fixture below.
MAPSET = "PERMANENT"


@pytest.fixture
def session(tmp_path):
    """An isolated GRASS session with a small region for fast map creation."""
    project = tmp_path / "g_remove_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session


def test_remove_procedure(session):
    """Without -f, matching maps are listed but not removed. With -f, they
    are removed and each removal is reported on stderr."""
    tools = Tools(session=session, consistent_return_value=True)
    tools.g_region(s=0, n=5, w=0, e=5, res=1)
    for i in range(10):
        tools.r_mapcalc(expression=f"test_map_{i} = 100")
    tools.r_mapcalc(expression="test_two = 2")

    listed = tools.g_remove(type="raster", pattern="test_map_*,*two")
    expected_listing = (
        "".join(f"raster/test_map_{i}@{MAPSET}\n" for i in range(10))
        + f"raster/test_two@{MAPSET}\n"
    )
    assert listed.stdout == expected_listing

    removed = tools.g_remove(type="raster", pattern="test_map_*,*two", flags="f")
    assert removed.stdout == ""
    expected_log = "".join(f"Removing raster <test_map_{i}>\n" for i in range(10))
    expected_log += "Removing raster <test_two>\n"
    assert removed.stderr == expected_log


def test_remove_procedure_exclude(session):
    """exclude= removes matches of its own pattern from the listing, without
    excluding them from a later removal that does not repeat exclude=."""
    tools = Tools(session=session, consistent_return_value=True)
    tools.g_region(s=0, n=5, w=0, e=5, res=1)
    tools.r_mapcalc(expression="test_apples = 100")
    tools.r_mapcalc(expression="test_oranges = 200")
    tools.r_mapcalc(expression="test_apples_big = 300")
    tools.r_mapcalc(expression="test_apples_small = 300")

    listed = tools.g_remove(
        type="raster", pattern="test_{apples,oranges}*", exclude="*_small"
    )
    assert listed.stdout == (
        f"raster/test_apples@{MAPSET}\n"
        f"raster/test_apples_big@{MAPSET}\n"
        f"raster/test_oranges@{MAPSET}\n"
    )

    removed = tools.g_remove(
        type="raster", pattern="test_{apples,oranges}{_small,_big,*}", flags="f"
    )
    assert removed.stdout == ""
    assert len(removed.stderr.strip().splitlines()) == 4


def test_re_flags(session):
    """-r and -e are mutually exclusive; check the specific error rather
    than just that both flag names appear (they are always listed together
    in the usage text printed on any parser error)."""
    tools = Tools(session=session)
    with pytest.raises(CalledModuleError, match=r"-r.*-e.*mutually exclusive"):
        tools.g_remove(flags="re", type="raster", pattern="xxxyyyzzz")
