# -*- coding: utf-8 -*-
"""
SQL
===

It is a collection of strings to avoid to repeat the code. ::

    >>> SELECT.format(cols=', '.join(['cat', 'area']), tname='table')
    'SELECT cat, area FROM table;'
    >>> SELECT_WHERE.format(cols=', '.join(['cat', 'area']),
    ...                     tname='table', condition='area>10000')
    'SELECT cat, area FROM table WHERE area>10000;'


"""

#
# SQL
#
CREATE_TAB = "CREATE TABLE {tname}({coldef})"
DROP_TAB = "DROP TABLE {tname}"
#ALTER TABLE
ADD_COL = "ALTER TABLE {tname} ADD COLUMN {cname} {ctype};"
DROP_COL = "ALTER TABLE {tname} DROP COLUMN {cname};"
DROP_COL_SQLITE = ';\n'.join([
"CREATE TEMPORARY TABLE {tname}_backup({coldef})",
"INSERT INTO {tname}_backup SELECT {colnames} FROM {tname}",
"DROP TABLE {tname}",
"CREATE TABLE {tname}({coldef})",
"INSERT INTO {tname} SELECT {colnames} FROM {tname}_backup",
"CREATE UNIQUE INDEX {tname}_cat ON {tname} ({keycol} )",
"DROP TABLE {tname}_backup",
])
RENAME_COL = "ALTER TABLE {tname} RENAME COLUMN {old_name} TO {new_name};"
CAST_COL = "ALTER TABLE {tname} ALTER COLUMN {col} SET DATA TYPE {ctype};"
RENAME_TAB = "ALTER TABLE {old_name} RENAME TO {new_name};"
INSERT = "INSERT INTO {tname} VALUES ({values})"

#SELECT
SELECT = "SELECT {cols} FROM {tname};"
SELECT_WHERE = "SELECT {cols} FROM {tname} WHERE {condition};"
SELECT_ORDERBY = "SELECT {cols} FROM {tname} ORDER BY {orderby};"

#UPDATE
UPDATE = "UPDATE {tname} SET {new_col} = {old_col};"
UPDATE_WHERE = "UPDATE {tname} SET {values} WHERE {condition};"
UPDATE_COL_WHERE = "UPDATE {tname} SET {new_col} = {old_col} WHERE {condition};"


# GET INFO
PRAGMA = "PRAGMA table_info({tname});"
