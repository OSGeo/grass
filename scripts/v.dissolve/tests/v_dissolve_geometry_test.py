"""Test v.dissolve with more advanced geometry"""

import json

import grass.script as gs


def test_dissolve_discontinuous_str(discontinuous_dataset):
    """Dissolving of discontinuous areas results in a single attribute record

    Even when the areas are discontinuous, there should be only one row
    in the attribute table.
    This behavior is assumed by the attribute aggregation functionality.
    """
    dataset = discontinuous_dataset
    dissolved_vector = "test_discontinuous_str"
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        env=discontinuous_dataset.session.env,
    )

    vector_info = gs.vector_info(
        dissolved_vector, env=discontinuous_dataset.session.env
    )
    assert vector_info["level"] == 2
    assert vector_info["centroids"] == 5
    assert vector_info["areas"] == 5
    assert vector_info["num_dblinks"] == 1
    assert vector_info["attribute_primary_key"] == "cat"
    # Reference values obtained by examining the result.
    assert vector_info["north"] == 80
    assert vector_info["south"] == 0
    assert vector_info["east"] == 120
    assert vector_info["west"] == 0
    assert vector_info["nodes"] == 14
    assert vector_info["points"] == 0
    assert vector_info["lines"] == 0
    assert vector_info["boundaries"] == 18
    assert vector_info["islands"] == 1
    assert vector_info["primitives"] == 23
    assert vector_info["map3d"] == 0

    columns = gs.vector_columns(dissolved_vector, env=discontinuous_dataset.session.env)
    assert len(columns) == 2
    assert sorted(columns.keys()) == sorted(["cat", dataset.str_column_name])
    column_info = columns[dataset.str_column_name]
    assert column_info["type"].lower() == "character"

    records = json.loads(
        gs.read_command(
            "v.db.select",
            map=dissolved_vector,
            format="json",
            env=dataset.session.env,
        )
    )["records"]
    ref_unique_values = set(dataset.str_column_values)
    actual_values = [record[dataset.str_column_name] for record in records]
    assert len(actual_values) == len(ref_unique_values)
    assert set(actual_values) == ref_unique_values
