"""Test v.fill.holes outputs"""

import json

import grass.script as gs


def test_removal(area_dataset):
    """Check that holes are removed"""
    output = "test"
    gs.run_command(
        "v.fill.holes",
        input=area_dataset.name,
        output=output,
        env=area_dataset.session.env,
    )
    original_info = gs.vector_info(area_dataset.name, env=area_dataset.session.env)
    info = gs.vector_info(output, env=area_dataset.session.env)
    removed = 4
    assert info["nodes"] == original_info["nodes"] - removed
    assert info["points"] == original_info["points"]
    assert info["lines"] == original_info["lines"]
    assert info["boundaries"] == original_info["boundaries"] - removed
    assert info["centroids"] == original_info["centroids"]
    assert info["areas"] == original_info["areas"] - removed
    assert info["islands"] == original_info["islands"] - removed
    assert info["primitives"] == original_info["primitives"] - removed


def test_no_change(area_dataset):
    """Check that space in between is not changed including attributes"""
    output = "no_change"
    gs.run_command(
        "v.fill.holes",
        input=area_dataset.areas_with_space_in_between,
        output=output,
        env=area_dataset.session.env,
    )
    original_info = gs.vector_info(
        area_dataset.areas_with_space_in_between, env=area_dataset.session.env
    )
    info = gs.vector_info(output, env=area_dataset.session.env)
    for item in [
        "nodes",
        "points",
        "lines",
        "boundaries",
        "centroids",
        "areas",
        "islands",
        "primitives",
    ]:
        assert info[item] == original_info[item], item

    records = json.loads(
        gs.read_command(
            "v.db.select", map=output, format="json", env=area_dataset.session.env
        )
    )["records"]
    assert len(records) == 2
    assert records[0]["cat"] == 3
    assert records[0]["name"] == "Left plot"
    assert records[1]["cat"] == 4
    assert records[1]["name"] == "Right plot"
