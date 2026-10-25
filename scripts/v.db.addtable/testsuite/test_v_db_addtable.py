"""
Created on Sun Jun 09 12:01:34 2018

@author: Sanjeet Bhatti
"""

import os

import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command
from grass.script.utils import decode


class TestVDbAddTable(TestCase):
    """Test v.db.addtable script"""

    @classmethod
    def setUpClass(cls):
        """Copy vector."""
        run_command("g.copy", vector="roadsmajor,myroads")

    @classmethod
    def tearDownClass(cls):
        """Remove copied vector"""
        run_command("g.remove", type="vector", name="myroads", flags="f")

    def test_add_single_columned_table_check(self):
        """Add a new attribute table with single column to layer 2"""
        module = SimpleModule(
            "v.db.addtable", map="myroads", columns="slope double precision", layer=2
        )
        self.assertModule(module)

        run_command("v.db.connect", flags="p", map="myroads")

        m = SimpleModule("v.info", map="myroads", flags="c")
        self.assertModule(m)
        self.assertNotRegex(decode(m.outputs.stdout), "slope")

        m = SimpleModule("v.info", map="myroads", flags="c", layer=2)
        self.assertModule(m)
        self.assertRegex(decode(m.outputs.stdout), "slope")


class TestVDbAddTablePerMapDatabase(TestCase):
    """Test v.db.addtable with a database file named after the map"""

    @classmethod
    def setUpClass(cls):
        """Use a per-map SQLite file and create a vector without a table."""
        cls.connection = gs.db_connection(force=True)
        run_command(
            "db.connect",
            driver="sqlite",
            database="$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/$MAP.db",
        )
        run_command("v.random", output="points", npoints=5, seed=1)

    @classmethod
    def tearDownClass(cls):
        """Remove the vector and restore the database connection"""
        run_command("g.remove", type="vector", name="points", flags="f")
        run_command(
            "db.connect",
            driver=cls.connection["driver"],
            database=cls.connection["database"],
        )

    def test_map_variable_in_file_name(self):
        """$MAP is expanded when it is part of the file name"""
        module = SimpleModule("v.db.addtable", map="points", columns="value integer")
        self.assertModule(module)

        m = SimpleModule("v.info", map="points", flags="c")
        self.assertModule(m)
        self.assertRegex(decode(m.outputs.stdout), "value")

        env = gs.gisenv()
        database = os.path.join(
            env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"], "sqlite", "points.db"
        )
        self.assertFileExists(database)


if __name__ == "__main__":
    test()
