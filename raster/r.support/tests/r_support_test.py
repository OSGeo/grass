# SPDX-License-Identifier: GPL-2.0-or-later
import os

import pytest

import grass.script as gs
from grass.tools import Tools
from grass.exceptions import CalledModuleError

SEMANTIC_LABEL = "The_Doors"


@pytest.fixture
def session(tmp_path):
    """A GRASS session with a single-cell synthetic raster."""
    project = tmp_path / "r_support_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=1, s=0, e=1, w=0, res=1)
        tools.r_mapcalc(expression="test = 1")
        yield session


def test_semantic_label_and_removal_are_exclusive(session):
    """Setting and removing a semantic label in one call must fail."""
    tools = Tools(session=session)
    with pytest.raises(CalledModuleError):
        tools.r_support(map="test", semantic_label=SEMANTIC_LABEL, flags="b")


def test_invalid_semantic_label_is_rejected(session):
    """A semantic label longer than the allowed length is rejected."""
    tools = Tools(session=session)
    with pytest.raises(CalledModuleError):
        tools.r_support(map="test", semantic_label="a" * 256)


def test_set_semantic_label(session):
    """r.support assigns the semantic label to the map."""
    tools = Tools(session=session)
    tools.r_support(map="test", semantic_label=SEMANTIC_LABEL)

    info = tools.r_info(map="test", format="json").json
    assert info["semantic_label"] == SEMANTIC_LABEL


def test_remove_semantic_label(session):
    """The -b flag clears a previously assigned semantic label."""
    tools = Tools(session=session)
    tools.r_support(map="test", semantic_label=SEMANTIC_LABEL)
    info = tools.r_info(map="test", format="json").json
    assert info["semantic_label"] == SEMANTIC_LABEL

    tools.r_support(map="test", flags="b")
    info = tools.r_info(map="test", format="json").json
    assert not info.get("semantic_label")


def test_set_metadata_fields(session):
    """title, units, vdatum, source1/source2 and description are all stored."""
    tools = Tools(session=session)
    tools.r_support(
        map="test",
        title="My Title",
        units="meters",
        vdatum="WGS84",
        source1="Source A",
        source2="Source B",
        description="A description",
    )

    info = tools.r_info(map="test", format="json").json
    assert info["title"] == "My Title"
    assert info["units"] == "meters"
    assert info["vdatum"] == "WGS84"
    assert info["source1"] == "Source A"
    assert info["source2"] == "Source B"
    assert info["description"] == "A description"


def test_append_history(session):
    """Each history= call appends a line rather than replacing prior ones."""
    tools = Tools(session=session)
    tools.r_support(map="test", history="First history line")
    tools.r_support(map="test", history="Second history line")

    info = tools.r_info(map="test", format="json").json
    assert info["comments"].endswith("First history line\nSecond history line")


def test_savehistory_and_loadhistory(session, tmp_path):
    """savehistory writes the history to a file that loadhistory can restore
    onto another map."""
    tools = Tools(session=session)
    tools.r_support(map="test", history="Line one")
    tools.r_support(map="test", history="Line two")
    expected = tools.r_info(map="test", format="json").json["comments"]

    history_file = tmp_path / "history.txt"
    tools.r_support(map="test", savehistory=str(history_file))
    assert history_file.read_text().strip() == expected

    tools.r_mapcalc(expression="test2 = 2")
    tools.r_support(map="test2", loadhistory=str(history_file))
    info = tools.r_info(map="test2", format="json").json
    assert info["comments"] == expected


def test_reset_null_file_clears_null_cells(session):
    """The -n flag (re)creates the null file, here clearing the null status
    of a cell that matches the CELL type's null sentinel on disk."""
    tools = Tools(session=session)
    tools.g_region(n=1, s=0, e=3, w=0, res=1)
    tools.r_mapcalc(expression="with_null = if(col() == 2, null(), col())")
    before = tools.r_univar(map="with_null", format="json").json
    assert before["null_cells"] == 1

    tools.r_support(map="with_null", flags="n")
    after = tools.r_univar(map="with_null", format="json").json
    assert after["null_cells"] == 0


def test_delete_null_file_restores_null_cells(session):
    """The -d flag removes the null file, falling back to the implicit
    null sentinel stored in the raw cell values."""
    tools = Tools(session=session)
    tools.g_region(n=1, s=0, e=3, w=0, res=1)
    tools.r_mapcalc(expression="with_null = if(col() == 2, null(), col())")
    tools.r_support(map="with_null", flags="n")
    assert tools.r_univar(map="with_null", format="json").json["null_cells"] == 0

    tools.r_support(map="with_null", flags="d")
    after = tools.r_univar(map="with_null", format="json").json
    assert after["null_cells"] == 1
