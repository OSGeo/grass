"""Test v.dissolve attribute aggregations"""

import json

import pytest

import grass.script as gs


@pytest.mark.parametrize(
    "aggregate_methods",
    [
        ["n"],
        ["sum"],
        ["range"],
        ["min", "max", "mean", "variance"],
        ["mean_abs", "stddev", "coeff_var"],
    ],
)
def test_aggregate_methods(dataset, aggregate_methods):
    """All aggregate methods are accepted and their columns generated"""
    dissolved_vector = f"test_methods_{'_'.join(aggregate_methods)}"
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        aggregate_column=dataset.float_column_name,
        aggregate_method=aggregate_methods,
    )
    columns = gs.vector_columns(dissolved_vector)
    stats_columns = [
        f"{dataset.float_column_name}_{method}" for method in aggregate_methods
    ]
    assert sorted(columns.keys()) == sorted(
        ["cat", dataset.str_column_name] + stats_columns
    )


def test_aggregate_two_columns(dataset):
    """Aggregate stats for two columns are generated"""
    dissolved_vector = "test_two_columns"
    aggregate_methods = ["mean", "stddev"]
    aggregate_columns = [dataset.float_column_name, dataset.int_column_name]
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        aggregate_column=aggregate_columns,
        aggregate_method=aggregate_methods,
    )
    stats_columns = [
        f"{column}_{method}"
        for method in aggregate_methods
        for column in aggregate_columns
    ]
    columns = gs.vector_columns(dissolved_vector)
    assert sorted(columns.keys()) == sorted(
        ["cat", dataset.str_column_name] + stats_columns
    )


def test_aggregate_column_result(dataset):
    """Check resulting types and values"""
    dissolved_vector = "test_results"
    stats = ["sum", "n"]
    stats_columns = ["value_sum", "value_n"]
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        aggregate_column=dataset.float_column_name,
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
    assert sorted(columns.keys()) == sorted(
        ["cat", dataset.str_column_name] + stats_columns
    )
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
    assert sorted(aggregate_sum) == [
        dataset.float_values[0],
        pytest.approx(dataset.float_values[3] + dataset.float_values[5]),
        pytest.approx(
            dataset.float_values[1] + dataset.float_values[2] + dataset.float_values[4]
        ),
    ]


def test_int_fails(dataset):
    """An integer column fails with aggregates"""
    dissolved_vector = "test_int"
    stats = ["sum", "n"]
    stats_columns = ["value_sum", "value_n"]
    assert (
        gs.run_command(
            "v.dissolve",
            input=dataset.vector_name,
            column=dataset.int_column_name,
            output=dissolved_vector,
            aggregate_column=dataset.float_column_name,
            aggregate_method=stats,
            stats_column=stats_columns,
            errors="status",
        )
        != 0
    )
