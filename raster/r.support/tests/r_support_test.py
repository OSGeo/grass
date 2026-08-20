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
