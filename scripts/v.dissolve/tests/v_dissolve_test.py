"""Test v.dissolve geometry info and basic attributes"""

import json

import grass.script as gs


def test_dissolve_int(dataset):
    """Dissolving works on integer column"""
    dissolved_vector = "test_int"
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.int_column_name,
        output=dissolved_vector,
        env=dataset.session.env,
    )

    vector_info = gs.vector_info(dissolved_vector, env=dataset.session.env)
    assert vector_info["level"] == 2
    assert vector_info["centroids"] == 3
    assert vector_info["areas"] == 3
    assert vector_info["num_dblinks"] == 0
    # Reference values obtained by examining the result.
    assert vector_info["north"] == 80
    assert vector_info["south"] == 0
    assert vector_info["east"] == 120
    assert vector_info["west"] == 0
    assert vector_info["nodes"] == 14
    assert vector_info["points"] == 0
    assert vector_info["lines"] == 0
    assert vector_info["boundaries"] == 16
    assert vector_info["islands"] == 1
    assert vector_info["primitives"] == 19
    assert vector_info["map3d"] == 0


def test_dissolve_str(dataset):
    """Dissolving works on string column and attributes are present"""
    dissolved_vector = "test_str"
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        env=dataset.session.env,
    )

    vector_info = gs.vector_info(dissolved_vector, env=dataset.session.env)
    assert vector_info["level"] == 2
    assert vector_info["centroids"] == 3
    assert vector_info["areas"] == 3
    assert vector_info["num_dblinks"] == 1
    assert vector_info["attribute_primary_key"] == "cat"
    # Reference values obtained by examining the result.
    assert vector_info["north"] == 80
    assert vector_info["south"] == 0
    assert vector_info["east"] == 120
    assert vector_info["west"] == 0
    assert vector_info["nodes"] == 13
    assert vector_info["points"] == 0
    assert vector_info["lines"] == 0
    assert vector_info["boundaries"] == 15
    assert vector_info["islands"] == 1
    assert vector_info["primitives"] == 18
    assert vector_info["map3d"] == 0

    columns = gs.vector_columns(dissolved_vector, env=dataset.session.env)
    assert len(columns) == 2
    assert sorted(columns.keys()) == sorted(["cat", dataset.str_column_name])
    column_info = columns[dataset.str_column_name]
    assert column_info["type"].lower() == "character"

    records = json.loads(
        gs.read_command(
            "v.db.select", map=dissolved_vector, format="json", env=dataset.session.env
        )
    )["records"]
    ref_unique_values = set(dataset.str_column_values)
    actual_values = [record[dataset.str_column_name] for record in records]
    assert len(actual_values) == len(ref_unique_values)
    assert set(actual_values) == ref_unique_values
