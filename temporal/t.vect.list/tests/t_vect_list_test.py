"""Test t.vect.list output formats"""

import json

import pytest

import grass.script as gs

yaml = pytest.importorskip("yaml", reason="PyYAML package not available")


@pytest.mark.needs_solo_run
def test_defaults(space_time_vector_dataset):
    """Check that the module runs with default parameters"""
    gs.run_command(
        "t.vect.list",
        input=space_time_vector_dataset.name,
        env=space_time_vector_dataset.session.env,
    )


@pytest.mark.needs_solo_run
def test_json(space_time_vector_dataset):
    """Check JSON can be parsed and contains the right values"""
    result = json.loads(
        gs.read_command(
            "t.vect.list",
            input=space_time_vector_dataset.name,
            format="json",
            env=space_time_vector_dataset.session.env,
        )
    )
    assert "data" in result
    assert "metadata" in result
    for item in result["data"]:
        for name in result["metadata"]["column_names"]:
            if name != "layer":  # layer can be None
                assert item[name], "All values should be set with the default columns"
    names = [item["name"] for item in result["data"]]
    assert names == space_time_vector_dataset.vector_names
