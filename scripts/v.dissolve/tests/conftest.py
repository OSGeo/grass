"""Fixtures for v.dissolve tests"""

from types import SimpleNamespace

import pytest

import grass.script as gs
import grass.script.setup as grass_setup


def updates_as_transaction(table, cat_column, column, column_quote, cats, values):
    """Create SQL statement for categories and values for a given column"""
    sql = ["BEGIN TRANSACTION"]
    if column_quote:
        quote = "'"
    else:
        quote = ""
    for cat, value in zip(cats, values):
        sql.append(
            f"UPDATE {table} SET {column} = {quote}{value}{quote} "
            f"WHERE {cat_column} = {cat};"
        )
    sql.append("END TRANSACTION")
    return "\n".join(sql)


def value_update_by_category(map_name, layer, column_name, cats, values):
    """Update column value for multiple rows based on category"""
    db_info = gs.vector_db(map_name)[layer]
    table = db_info["table"]
    database = db_info["database"]
    driver = db_info["driver"]
    cat_column = "cat"
    column_type = gs.vector_columns(map_name, layer)[column_name]
    column_quote = bool(column_type["type"] in ("CHARACTER", "TEXT"))
    sql = updates_as_transaction(
        table=table,
        cat_column=cat_column,
        column=column_name,
        column_quote=column_quote,
        cats=cats,
        values=values,
    )
    gs.write_command(
        "db.execute", input="-", database=database, driver=driver, stdin=sql
    )


@pytest.fixture(scope="module")
def dataset(tmp_path_factory):
    """Creates a session with a mapset which has vector with a float column"""
    tmp_path = tmp_path_factory.mktemp("dataset")
    location = "test"
    point_map_name = "points"
    map_name = "areas"
    dissolve_column_name = "int_value"
    value_column_name = "double_value"
    str_column_name = "str_value"

    cats = [1, 2, 3, 4, 5, 6]
    dissolve_values = [10, 10, 24, 5, 5, 5]
    values = [100.78, 102.78, 109.78, 104.78, 103.78, 105.78]
    str_values = ["apples", "oranges", "oranges", "plumbs", "oranges", "plumbs"]
    num_points = len(cats)

    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with grass_setup.init(tmp_path / location):
        gs.run_command("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        gs.run_command("v.random", output=point_map_name, npoints=num_points, seed=42)
        gs.run_command("v.voronoi", input=point_map_name, output=map_name)
        gs.run_command(
            "v.db.addtable",
            map=map_name,
            columns=[
                f"{dissolve_column_name} integer",
                f"{value_column_name} double precision",
                f"{str_column_name} text",
            ],
        )
        value_update_by_category(
            map_name=map_name,
            layer=1,
            column_name=dissolve_column_name,
            cats=cats,
            values=dissolve_values,
        )
        value_update_by_category(
            map_name=map_name,
            layer=1,
            column_name=value_column_name,
            cats=cats,
            values=values,
        )
        value_update_by_category(
            map_name=map_name,
            layer=1,
            column_name=str_column_name,
            cats=cats,
            values=str_values,
        )
        yield SimpleNamespace(
            vector_name=map_name,
            dissolve_column_name=dissolve_column_name,
            dissolve_values=dissolve_values,
            value_column_name=value_column_name,
            values=values,
            str_column_name=str_column_name,
            str_column_values=str_values,
        )
