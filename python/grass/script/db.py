"""
Database related functions to be used in Python scripts.

Usage:

::

    from grass.script import db as grass

    grass.db_describe(table)
    ...

(C) 2008-2015 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Glynn Clements
.. sectionauthor:: Martin Landa <landa.martin gmail.com>
"""

from __future__ import annotations

import os

from ctypes import byref

from .core import (
    gisenv,
    run_command,
    parse_command,
    read_command,
    tempfile,
    fatal,
    list_strings,
)
from .utils import try_remove
from grass.exceptions import CalledModuleError


def db_describe(table, env=None, **args):
    """Return the list of columns for a database table
    (interface to `db.describe -c`). Example:

    >>> run_command("g.copy", vector="firestations,myfirestations")
    0
    >>> db_describe("myfirestations")  # doctest: +ELLIPSIS
    {'nrows': 71, 'cols': [['cat', 'INTEGER', '20'], ... 'ncols': 22}
    >>> run_command("g.remove", flags="f", type="vector", name="myfirestations")
    0

    :param str table: table name
    :param list args:
    :param env: environment

    :return: parsed module output
    """
    if "database" in args and args["database"] == "":
        args.pop("database")
    if "driver" in args and args["driver"] == "":
        args.pop("driver")
    s = read_command("db.describe", flags="c", table=table, env=env, **args)
    if not s:
        fatal(_("Unable to describe table <%s>") % table, env=env)

    cols = []
    result = {}
    for line in s.splitlines():
        f = line.split(":")
        key = f[0]
        f[1] = f[1].lstrip(" ")
        if key.startswith("Column "):
            n = int(key.split(" ")[1])
            cols.insert(n, f[1:])
        elif key in {"ncols", "nrows"}:
            result[key] = int(f[1])
        else:
            result[key] = f[1:]
    result["cols"] = cols

    return result


def db_table_exist(table, env=None, **args):
    """Check if table exists.

    If no driver or database are given, then default settings is used
    (check db_connection()).

    >>> run_command("g.copy", vector="firestations,myfirestations")
    0
    >>> db_table_exist("myfirestations")
    True
    >>> run_command("g.remove", flags="f", type="vector", name="myfirestations")
    0

    :param str table: table name
    :param args:
    :param env: environment

    :return: True for success, False otherwise
    """
    nuldev = open(os.devnull, "w+")
    ok = True
    try:
        run_command(
            "db.describe",
            flags="c",
            table=table,
            stdout=nuldev,
            stderr=nuldev,
            env=env,
            **args,
        )
    except CalledModuleError:
        ok = False
    finally:
        nuldev.close()

    return ok


def db_connection(force=False, env=None):
    """Return the current database connection parameters
    (interface to `db.connect -g`). Example:

    >>> db_connection()
    {'group': '', 'schema': '', 'driver': 'sqlite', 'database': '$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'}

    :param force True to set up default DB connection if not defined
    :param env: environment

    :return: parsed output of db.connect
    """  # noqa: E501
    try:
        nuldev = open(os.devnull, "w")
        conn = parse_command("db.connect", flags="g", stderr=nuldev, env=env)
        nuldev.close()
    except CalledModuleError:
        conn = None

    if not conn and force:
        run_command("db.connect", flags="c", env=env)
        conn = parse_command("db.connect", flags="g", env=env)

    if conn and conn.get("driver") == "sqlite":
        gis_env = gisenv()
        conn["database"] = (
            conn["database"]
            .replace("$GISDBASE", gis_env["GISDBASE"])
            .replace("$LOCATION_NAME", gis_env["LOCATION_NAME"])
            .replace("$MAPSET", gis_env["MAPSET"])
        )

    return conn


def db_select(sql=None, filename=None, table=None, env=None, **args):
    """Perform SQL select statement

    Note: one of <em>sql</em>, <em>filename</em>, or <em>table</em>
    arguments must be provided.

    Examples:

    >>> run_command("g.copy", vector="firestations,myfirestations")
    0
    >>> db_select(sql="SELECT cat,CITY FROM myfirestations WHERE cat < 4")
    (('1', 'Morrisville'), ('2', 'Morrisville'), ('3', 'Apex'))

    Simplyfied usage (it performs <tt>SELECT * FROM myfirestations</tt>.)

    >>> db_select(table="myfirestations")  # doctest: +ELLIPSIS
    (('1', '24', 'Morrisville #3', ... 'HS2A', '1.37'))
    >>> run_command("g.remove", flags="f", type="vector", name="myfirestations")
    0

    :param str sql: SQL statement to perform (or None)
    :param str filename: name of file with SQL statements (or None)
    :param str table: name of table to query (or None)
    :param str args: see *db.select* arguments
    :param env: environment
    """
    fname = tempfile(create=False, env=env)
    if sql:
        args["sql"] = sql
    elif filename:
        args["input"] = filename
    elif table:
        args["table"] = table
    else:
        fatal(
            _(
                "Programmer error: '%(sql)s', '%(filename)s', or '%(table)s' must be \
                    provided"
            )
            % {"sql": "sql", "filename": "filename", "table": "table"},
            env=env,
        )

    if "sep" not in args:
        args["sep"] = "|"

    try:
        run_command("db.select", quiet=True, flags="c", output=fname, env=env, **args)
    except CalledModuleError:
        fatal(_("Fetching data failed"), env=env)

    ofile = open(fname)
    result = [tuple(x.rstrip(os.linesep).split(args["sep"])) for x in ofile]
    ofile.close()
    try_remove(fname)

    return tuple(result)


def db_table_in_vector(table, mapset=".", env=None):
    """Return the name of vector connected to the table.
    By default it check only in the current mapset, because the same table
    name could be used also in other mapset by other vector.
    It returns None if no vectors are connected to the table.

    >>> run_command("g.copy", vector="firestations,myfirestations")
    0
    >>> db_table_in_vector("myfirestations")
    ['myfirestations@user1']
    >>> db_table_in_vector("mfirestations")
    >>> run_command("g.remove", flags="f", type="vector", name="myfirestations")
    0

    :param str table: name of table to query
    :param env: environment
    """
    from .vector import vector_db

    nuldev = open(os.devnull, "w")
    used = []
    vects = list_strings("vector", mapset=mapset, env=env)
    for vect in vects:
        for f in vector_db(vect, stderr=nuldev, env=env).values():
            if not f:
                continue
            if f["table"] == table:
                used.append(vect)
                break
    if len(used) > 0:
        return used
    return None


class DBHandler:
    """DB handler

    Allow execute SQL command(s) in transaction mode.

    Public methods:

    ::execute
    """

    def __init__(self, driver_name: str, database: str) -> None:
        """Constructor

        :param driver_name: DB driver name
        :param database: database name
        """
        self._driver_name = driver_name
        self._database = database
        self._import_c_interface()

    def _import_c_interface(self):
        """Import C interface"""
        try:
            from grass.lib.dbmi import (
                db_begin_transaction,
                db_close_database_shutdown_driver,
                db_commit_transaction,
                db_execute_immediate,
                db_free_string,
                db_init_string,
                db_set_string,
                db_start_driver_open_database,
                dbString,
                DB_OK,
            )
            from grass.lib.gis import G_gisinit
            from grass.lib.vector import (
                Map_info,
                Vect_subst_var,
            )
        except (ImportError, OSError, TypeError) as e:
            fatal(_("Unable to import C functions: {e}").format(e))

        class CInterface:
            def __init__(self):
                self.db_begin_transaction = db_begin_transaction
                self.db_execute_immediate = db_execute_immediate
                self.db_free_string = db_free_string
                self.db_init_string = db_init_string
                self.db_close_database_shutdown_driver = (
                    db_close_database_shutdown_driver
                )
                self.db_commit_transaction = db_commit_transaction
                self.db_set_string = db_set_string
                self.db_start_driver_open_database = db_start_driver_open_database
                self.dbString = dbString
                self.DB_OK = DB_OK
                self.G_gisinit = G_gisinit
                self.Map_info = Map_info
                self.Vect_subst_var = Vect_subst_var

        self._c_interface = CInterface()

    def _init_driver(self):
        """Init DB driver"""
        map = self._c_interface.Map_info()
        self._pdriver = self._c_interface.db_start_driver_open_database(
            self._driver_name,
            self._c_interface.Vect_subst_var(self._database, byref(map)),
        )
        if not self._pdriver:
            fatal(
                _("Unable to open database <{db}> by driver <{driver}>.").format(
                    db=self._database, driver=self._driver_name
                )
            )

    def _begin_transaction(self):
        """Begin DB transaction."""
        ret = self._c_interface.db_begin_transaction(self._pdriver)
        if ret != self._c_interface.DB_OK:
            self._shutdown_driver()
            fatal(
                _(
                    "Error while starting database <{db}> transaction by"
                    " driver <{driver}>."
                ).format(db=self._database, driver=self._driver_name)
            )

    def _commit_transaction(self):
        """Commit DB transaction."""
        ret = self._c_interface.db_commit_transaction(self._pdriver)
        if ret != self._c_interface.DB_OK:
            self._shutdown_driver()
            fatal(
                _(
                    "Error while commit database <{db}> transaction"
                    " by driver <{driver}>."
                ).format(db=self._database, driver=self._driver_name)
            )

    def _execute(self, sql: str | list | tuple) -> None:
        """Execute SQL

        :param sql: SQL command string or list of SQLs commands
        """
        stmt = self._c_interface.dbString()
        self._c_interface.db_init_string(byref(stmt))
        self._c_interface.db_set_string(byref(stmt), sql)
        if (
            self._c_interface.db_execute_immediate(self._pdriver, byref(stmt))
            != self._c_interface.DB_OK
        ):
            self._c_interface.db_free_string(byref(stmt))
            self._shutdown_driver()
            fatal(_("Error while executing SQL <{}>.").format(sql))
        self._c_interface.db_free_string(byref(stmt))

    def _shutdown_driver(self):
        """Close DB and shutdown driver"""
        self._c_interface.db_close_database_shutdown_driver(self._pdriver)

    def execute(self, sql: str | list | tuple) -> None:
        """Execute SQL

        :param sql: SQL command string or list of SQLs statement
        """
        self._c_interface.G_gisinit("")
        self._init_driver()

        # Begin DB transaction
        self._begin_transaction()
        # Execute SQL string
        if isinstance(sql, (list, tuple)):
            for statement in sql:
                self._execute(sql=statement)
        else:
            self._execute(sql)
        # Commit DB transaction
        self._commit_transaction()

        self._shutdown_driver()
