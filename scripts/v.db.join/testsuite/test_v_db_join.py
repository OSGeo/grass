"""
TEST:      test_v_db_join.py

AUTHOR(S): Stefan Blumentrath

PURPOSE:   Test for v.db.join

COPYRIGHT: (C) 2024 Stefan Blumentrath, and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestVDbJoin(TestCase):
    """Test v.db.join script"""

    @classmethod
    def setUpClass(cls):
        """Copy vector."""
        firestation_sql = """CREATE TABLE firestation_test_table (
        CITY text,
        some_number int,
        some_text text,
        some_double double precision,
        some_float real
        );
        INSERT INTO firestation_test VALUES
        ('Cary', 1, 'short', 1.1233445366756784345,),
        ('Apex', 2, 'longer', -111.1220390953406936354,),
        ('Garner', 3, 'short', 4.20529509802443234245,),
        ('Relaigh', 4, 'even longer than before', 32.913873948295837592,);
        """
        firestation_existing_sql = """CREATE TABLE firestation_test_table_update (
        CITY text,
        others int
        );
        INSERT INTO firestation_test_table_update VALUES
        ('Cary', 1),
        ('Apex', 2),
        ('Garner', 3),
        ('Relaigh', 4);
        """
        cls.runModule("g.copy", vector=["firestations", "test_firestations"])
        cls.runModule("db.execute", sql=firestation_sql)
        cls.runModule("db.execute", sql=firestation_existing_sql)

    @classmethod
    def tearDownClass(cls):
        """Remove copied vector data and created tables"""
        cls.runModule("g.remove", type="vector", name="test_firestations", flags="f")
        cls.runModule("db.execute", sql="DROP TABLE firestation_test_table;")
        cls.runModule("db.execute", sql="DROP TABLE firestation_test_table_update;")

    def test_join_firestations_table(self):
        """Join firestations table with new different columns"""
        module = SimpleModule(
            "v.db.join",
            map="test_firestations",
            column="CITY",
            other_table="firestation_test_table",
            other_column="CITY",
        )
        self.assertModule(module)

    def test_join_firestations_table_existing(self):
        """Join firestations table with only existing columns"""
        module = SimpleModule(
            "v.db.join",
            map="test_firestations",
            column="CITY",
            other_table="firestation_test_table_update",
            other_column="CITY",
        )
        self.assertModule(module)


if __name__ == "__main__":
    test()
