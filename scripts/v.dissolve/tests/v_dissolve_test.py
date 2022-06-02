"""Test v.dissolve attribute aggregations"""

import json

import grass.script as gs


def test_aggregate_column(dataset):
    """Check resulting types and values"""
    dissolved_vector = "test1"
    stats = ["sum", "n"]
    stats_columns = ["value_sum", "value_n"]
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        aggregate_column=dataset.value_column_name,
        aggregate_method=stats,
        stats_column=stats_columns,
    )

    vector_info = gs.vector_info(dissolved_vector)
    assert vector_info["level"] == 2
    assert vector_info["centroids"] == 3
    assert vector_info["areas"] == 3
    assert vector_info["num_dblinks"] == 1
    assert vector_info["attribute_primary_key"] == "cat"

    columns = gs.vector_columns(dissolved_vector)
    assert len(columns) == 4
    for stats_column in stats_columns:
        assert stats_column in columns
        column_info = columns[stats_column]
        if stats_column.endswith("_n"):
            correct_type = "integer"
        else:
            correct_type = "double precision"
        assert (
            columns[stats_column]["type"].lower() == correct_type
        ), f"{stats_column} has a wrong type"
    assert dataset.str_column_name in columns
    column_info = columns[dataset.str_column_name]
    assert column_info["type"].lower() == "character"

    records = json.loads(
        gs.read_command(
            "v.db.select",
            map=dissolved_vector,
            format="json",
        )
    )["records"]
    ref_unique_values = set(dataset.str_column_values)
    actual_values = [record[dataset.str_column_name] for record in records]
    assert len(actual_values) == len(ref_unique_values)
    assert set(actual_values) == ref_unique_values

    aggregate_n = [record["value_n"] for record in records]
    assert sum(aggregate_n) == gs.vector_info(dataset.vector_name)["areas"]
    assert sorted(aggregate_n) == [1, 2, 3]
    aggregate_sum = [record["value_sum"] for record in records]
    assert sorted(aggregate_sum) == [100.78, 210.56, 316.34]
