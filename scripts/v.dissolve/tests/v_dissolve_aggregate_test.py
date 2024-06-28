"""Test v.dissolve attribute aggregations"""

import json
import statistics

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
        env=dataset.session.env,
    )
    columns = gs.vector_columns(dissolved_vector, env=dataset.session.env)
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
        env=dataset.session.env,
    )
    stats_columns = [
        f"{column}_{method}"
        for method in aggregate_methods
        for column in aggregate_columns
    ]
    columns = gs.vector_columns(dissolved_vector, env=dataset.session.env)
    assert sorted(columns.keys()) == sorted(
        ["cat", dataset.str_column_name] + stats_columns
    )


@pytest.mark.parametrize("backend", [None, "univar", "sql"])
def test_aggregate_column_result(dataset, backend):
    """Check resulting types and values of basic stats with different backends

    It assumes that the univar-like names are translated to SQLite names.
    """
    dissolved_vector = f"test_results_{backend}"
    stats = ["sum", "n", "min", "max", "mean"]
    stats_columns = [f"value_{method}" for method in stats]
    aggregate_columns = [dataset.float_column_name] * len(stats)
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        aggregate_column=aggregate_columns,
        aggregate_method=stats,
        result_column=stats_columns,
        aggregate_backend=backend,
        env=dataset.session.env,
    )

    vector_info = gs.vector_info(dissolved_vector, env=dataset.session.env)
    assert vector_info["level"] == 2
    assert vector_info["centroids"] == 3
    assert vector_info["areas"] == 3
    assert vector_info["num_dblinks"] == 1
    assert vector_info["attribute_primary_key"] == "cat"

    columns = gs.vector_columns(dissolved_vector, env=dataset.session.env)
    assert len(columns) == len(stats_columns) + 2
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
            "v.db.select", map=dissolved_vector, format="json", env=dataset.session.env
        )
    )["records"]
    ref_unique_values = set(dataset.str_column_values)
    actual_values = [record[dataset.str_column_name] for record in records]
    assert len(actual_values) == len(ref_unique_values)
    assert set(actual_values) == ref_unique_values

    aggregate_n = [record["value_n"] for record in records]
    assert (
        sum(aggregate_n)
        == gs.vector_info(dataset.vector_name, env=dataset.session.env)["areas"]
    )
    assert sorted(aggregate_n) == [1, 2, 3]
    aggregate_sum = [record["value_sum"] for record in records]
    assert sorted(aggregate_sum) == [
        dataset.float_values[0],
        pytest.approx(dataset.float_values[3] + dataset.float_values[5]),
        pytest.approx(
            dataset.float_values[1] + dataset.float_values[2] + dataset.float_values[4]
        ),
    ]
    aggregate_max = [record["value_max"] for record in records]
    assert sorted(aggregate_max) == [
        dataset.float_values[0],
        pytest.approx(max([dataset.float_values[3], dataset.float_values[5]])),
        pytest.approx(
            max(
                [
                    dataset.float_values[1],
                    dataset.float_values[2],
                    dataset.float_values[4],
                ]
            )
        ),
    ]
    aggregate_min = [record["value_min"] for record in records]
    assert sorted(aggregate_min) == [
        dataset.float_values[0],
        pytest.approx(
            min(
                [
                    dataset.float_values[1],
                    dataset.float_values[2],
                    dataset.float_values[4],
                ]
            )
        ),
        pytest.approx(min([dataset.float_values[3], dataset.float_values[5]])),
    ]
    aggregate_mean = [record["value_mean"] for record in records]
    assert sorted(aggregate_mean) == [
        dataset.float_values[0],
        pytest.approx(
            statistics.mean([dataset.float_values[3], dataset.float_values[5]])
        ),
        pytest.approx(
            statistics.mean(
                [
                    dataset.float_values[1],
                    dataset.float_values[2],
                    dataset.float_values[4],
                ]
            )
        ),
    ]


def test_sqlite_agg_accepted(dataset):
    """Numeric SQLite aggregate functions are accepted

    Additionally, it checks:
    1. generated column names
    2. types of columns
    3. aggregate counts
    """
    dissolved_vector = "test_sqlite"
    stats = ["avg", "count", "max", "min", "sum", "total"]
    expected_stats_columns = [
        f"{dataset.float_column_name}_{method}" for method in stats
    ]
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        aggregate_column=dataset.float_column_name,
        aggregate_method=stats,
        aggregate_backend="sql",
        env=dataset.session.env,
    )

    vector_info = gs.vector_info(dissolved_vector, env=dataset.session.env)
    assert vector_info["level"] == 2
    assert vector_info["centroids"] == 3
    assert vector_info["areas"] == 3
    assert vector_info["num_dblinks"] == 1
    assert vector_info["attribute_primary_key"] == "cat"

    columns = gs.vector_columns(dissolved_vector, env=dataset.session.env)
    assert len(columns) == len(expected_stats_columns) + 2
    assert sorted(columns.keys()) == sorted(
        ["cat", dataset.str_column_name] + expected_stats_columns
    ), "Unexpected autogenerated column names"
    for method, stats_column in zip(stats, expected_stats_columns):
        assert stats_column in columns
        column_info = columns[stats_column]
        if method == "count":
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
            "v.db.select", map=dissolved_vector, format="json", env=dataset.session.env
        )
    )["records"]
    ref_unique_values = set(dataset.str_column_values)
    actual_values = [record[dataset.str_column_name] for record in records]
    assert len(actual_values) == len(ref_unique_values)
    assert set(actual_values) == ref_unique_values

    aggregate_n = [record[f"{dataset.float_column_name}_count"] for record in records]
    assert (
        sum(aggregate_n)
        == gs.vector_info(dataset.vector_name, env=dataset.session.env)["areas"]
    )
    assert sorted(aggregate_n) == [1, 2, 3]


def test_sqlite_concat(dataset):
    """SQLite group concat text-returning aggregate function works"""
    dissolved_vector = "test_sqlite_concat"
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        aggregate_column=f"group_concat({dataset.int_column_name})",
        result_column="concat_values text",
        aggregate_backend="sql",
        env=dataset.session.env,
    )
    records = json.loads(
        gs.read_command(
            "v.db.select", map=dissolved_vector, format="json", env=dataset.session.env
        )
    )["records"]
    # Order of records is ignored - they are just sorted.
    # Order within values of group_concat is defined as arbitrary by SQLite.
    expected_integers = sorted(["10", "10,10,24", "5,5"])
    actual_integers = sorted([record["concat_values"] for record in records])
    for expected, actual in zip(expected_integers, actual_integers):
        assert sorted(expected.split(",")) == sorted(actual.split(","))


def test_sqlite_concat_with_two_parameters(dataset):
    """SQLite group concat text-returning two-parameter aggregate function works"""
    dissolved_vector = "test_sqlite_concat_separator"
    separator = "--+--"
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        aggregate_column=f"group_concat({dataset.int_column_name}, '{separator}')",
        result_column="concat_values text",
        aggregate_backend="sql",
        env=dataset.session.env,
    )
    records = json.loads(
        gs.read_command(
            "v.db.select", map=dissolved_vector, format="json", env=dataset.session.env
        )
    )["records"]
    # Order of records is ignored - they are just sorted.
    # Order within values of group_concat is defined as arbitrary by SQLite.
    expected_integers = sorted(["10", "10,10,24", "5,5"])
    actual_integers = sorted([record["concat_values"] for record in records])
    for expected, actual in zip(expected_integers, actual_integers):
        assert sorted(expected.split(",")) == sorted(actual.split(separator))


def test_duplicate_columns_and_methods_accepted(dataset):
    """Duplicate aggregate columns and methods are accepted and deduplicated"""
    dissolved_vector = "test_duplicates"
    stats = ["count", "count", "n", "min", "min", "n", "sum"]
    expected_stats_columns = [
        f"{dataset.float_column_name}_{method}"
        for method in ["count", "n", "min", "sum"]
    ]
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        aggregate_column=[dataset.float_column_name, dataset.float_column_name],
        aggregate_method=stats,
        aggregate_backend="sql",
        env=dataset.session.env,
    )

    vector_info = gs.vector_info(dissolved_vector, env=dataset.session.env)
    assert vector_info["level"] == 2
    assert vector_info["centroids"] == 3
    assert vector_info["areas"] == 3
    assert vector_info["num_dblinks"] == 1
    assert vector_info["attribute_primary_key"] == "cat"

    columns = gs.vector_columns(dissolved_vector, env=dataset.session.env)
    assert sorted(columns.keys()) == sorted(
        ["cat", dataset.str_column_name] + expected_stats_columns
    ), "Unexpected autogenerated column names"


def test_sql_expressions_accepted(dataset):
    """Arbitrary SQL expressions are accepted for columns"""
    dissolved_vector = "test_expressions"
    aggregate_columns = (
        f"sum({dataset.float_column_name}),    "
        f"max({dataset.float_column_name}) -    min({dataset.float_column_name}), "
        f"    count({dataset.float_column_name})    "
    )
    result_columns = (
        "   sum_of_values double,   range_of_values   double, count_of_rows integer"
    )
    expected_stats_columns = ["sum_of_values", "range_of_values", "count_of_rows"]
    gs.run_command(
        "v.dissolve",
        input=dataset.vector_name,
        column=dataset.str_column_name,
        output=dissolved_vector,
        aggregate_column=aggregate_columns,
        result_column=result_columns,
        aggregate_backend="sql",
        env=dataset.session.env,
    )

    vector_info = gs.vector_info(dissolved_vector, env=dataset.session.env)
    assert vector_info["level"] == 2
    assert vector_info["centroids"] == 3
    assert vector_info["areas"] == 3
    assert vector_info["num_dblinks"] == 1
    assert vector_info["attribute_primary_key"] == "cat"

    columns = gs.vector_columns(dissolved_vector, env=dataset.session.env)
    assert sorted(columns.keys()) == sorted(
        ["cat", dataset.str_column_name] + expected_stats_columns
    )


def test_no_methods_with_univar_and_result_columns_fail(dataset):
    """Omitting methods as for sql backend is forbiden for univar"""
    dissolved_vector = "test_no_method_univar_fails"

    aggregate_columns = dataset.float_column_name
    result_columns = (
        "sum_of_values double,range_of_values double, count_of_rows integer"
    )
    assert (
        gs.run_command(
            "v.dissolve",
            input=dataset.vector_name,
            column=dataset.str_column_name,
            output=dissolved_vector,
            aggregate_column=aggregate_columns,
            result_column=result_columns,
            aggregate_backend="univar",
            errors="status",
            env=dataset.session.env,
        )
        != 0
    )


def test_int_fails(dataset):
    """An integer column fails with aggregates"""
    dissolved_vector = "test_int"
    assert (
        gs.run_command(
            "v.dissolve",
            input=dataset.vector_name,
            column=dataset.int_column_name,
            output=dissolved_vector,
            aggregate_column=dataset.float_column_name,
            aggregate_method="n",
            errors="status",
            env=dataset.session.env,
        )
        != 0
    )
