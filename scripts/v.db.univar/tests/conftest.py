"""Fixtures for v.db.univar tests"""

import os

from types import SimpleNamespace

import pytest

import grass.script as gs


def updates_as_transaction(table, cat_column, column, cats, values):
    """Create SQL statement for categories and values for a given column"""
    sql = ["BEGIN TRANSACTION"]
    for cat, value in zip(cats, values):
        sql.append(f"UPDATE {table} SET {column} = {value} WHERE {cat_column} = {cat};")
    sql.append("END TRANSACTION")
    return "\n".join(sql)


def value_update_by_category(map_name, layer, column_name, cats, values, env):
    """Update column value for multiple rows based on category"""
    db_info = gs.vector_db(map_name, env=env)[layer]
    table = db_info["table"]
    database = db_info["database"]
    driver = db_info["driver"]
    cat_column = "cat"
    sql = updates_as_transaction(
        table=table,
        cat_column=cat_column,
        column=column_name,
        cats=cats,
        values=values,
    )
    gs.write_command(
        "db.execute", input="-", database=database, driver=driver, stdin=sql, env=env
    )


@pytest.fixture(scope="module")
def simple_dataset(tmp_path_factory):
    """Creates a session with a mapset which has vector with a float column"""
    tmp_path = tmp_path_factory.mktemp("simple_dataset")
    location = "test"
    map_name = "points"
    column_name = "double_value"
    num_points = 10
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with gs.setup.init(tmp_path / location, env=os.environ.copy()) as session:
        gs.run_command(
            "g.region",
            s=0,
            n=80,
            w=0,
            e=120,
            b=0,
            t=50,
            res=10,
            res3=10,
            env=session.env,
        )
        gs.run_command(
            "v.random", output=map_name, npoints=num_points, seed=42, env=session.env
        )
        gs.run_command(
            "v.db.addtable",
            map=map_name,
            columns=f"{column_name} double precision",
            env=session.env,
        )
        cats = list(range(1, 1 + num_points))
        values = [float(i) + 0.11 for i in range(100, 100 + num_points)]
        value_update_by_category(
            map_name=map_name,
            layer=1,
            column_name=column_name,
            cats=cats,
            values=values,
            env=session.env,
        )
        yield SimpleNamespace(
            session=session,
            vector_name=map_name,
            column_name=column_name,
            values=values,
        )
