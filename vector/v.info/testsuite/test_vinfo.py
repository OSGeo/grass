import json

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.gunittest.gmodules import SimpleModule


class TestVInfo(TestCase):
    """Test the shell output of v.info that is not location/mapset or user dependent"""

    test_vinfo_no_db = "test_vinfo_no_db"
    test_vinfo_with_db = "test_vinfo_with_db"
    test_vinfo_with_db_3d = "test_vinfo_with_db_3d"
    test_vinfo_with_hist = "test_vinfo_with_hist"

    # All maps should be tested against these references
    reference = {
        "format": "native",
        "level": 2,
        "nodes": 0,
        "points": 5,
        "lines": 0,
        "boundaries": 0,
        "centroids": 0,
        "areas": 0,
        "islands": 0,
        "primitives": 5,
        "scale": "1:1",
    }

    @classmethod
    def setUpClass(cls):
        """Generate some vector layer with attribute table, z coordinates
        and timestamp
        """
        cls.runModule(
            "v.random", output=cls.test_vinfo_no_db, npoints=5, zmin=0, zmax=100
        )

        cls.runModule(
            "v.random",
            output=cls.test_vinfo_with_db,
            npoints=5,
            zmin=0,
            zmax=100,
            column="elevation",
        )

        cls.runModule(
            "v.random",
            output=cls.test_vinfo_with_db_3d,
            npoints=5,
            zmin=0,
            zmax=100,
            column="elevation",
            flags="z",
        )

        cls.runModule(
            "v.random", output=cls.test_vinfo_with_hist, npoints=5, zmin=0, zmax=100
        )

        # For testing vector history file with multiple commands
        cls.runModule(
            "v.support",
            map=cls.test_vinfo_with_hist,
            cmdhist='v.mkgrid map="test_vinfo_with_hist" grid=10,10 type="point"',
        )

        cls.runModule("v.timestamp", map=cls.test_vinfo_with_db_3d, date="15 jan 1994")

    @classmethod
    def tearDownClass(cls):
        """Remove created maps"""
        cls.runModule(
            "g.remove",
            flags="f",
            type="vector",
            name=[
                cls.test_vinfo_no_db,
                cls.test_vinfo_with_db,
                cls.test_vinfo_with_db_3d,
                cls.test_vinfo_with_hist,
            ],
        )

    def test_smoke(self):
        """Simply test running the module with different parameters"""
        self.assertModule("v.info", map=self.test_vinfo_with_db_3d)
        self.assertModule("v.info", map=self.test_vinfo_with_db_3d, flags="e")
        self.assertModule("v.info", map=self.test_vinfo_with_db_3d, flags="et")
        self.assertModule("v.info", map=self.test_vinfo_with_db_3d, flags="tg")
        self.assertModule("v.info", map=self.test_vinfo_with_db_3d, flags="t")
        self.assertModule("v.info", map=self.test_vinfo_with_db_3d, flags="c")
        self.assertModule("v.info", map=self.test_vinfo_with_db_3d, flags="h")

    def test_common_references(self):
        """Test all maps against the common references"""
        self.assertModuleKeyValue(
            "v.info",
            map=self.test_vinfo_no_db,
            flags="etg",
            sep="=",
            precision=0.1,
            reference=self.reference,
        )
        self.assertModuleKeyValue(
            "v.info",
            map=self.test_vinfo_with_db,
            flags="etg",
            sep="=",
            precision=0.1,
            reference=self.reference,
        )
        self.assertModuleKeyValue(
            "v.info",
            map=self.test_vinfo_with_db_3d,
            flags="etg",
            sep="=",
            precision=0.1,
            reference=self.reference,
        )

    def test_info_no_db(self):
        """Test the simple vector map"""
        self.assertModuleKeyValue(
            "v.info",
            map=self.test_vinfo_no_db,
            flags="etg",
            sep="=",
            precision=0.1,
            reference={
                "name": self.test_vinfo_no_db,
                "map3d": 0,
                "num_dblinks": 0,
                "bottom": 0.0,
                "top": 0.0,
            },
        )

    def test_info_with_db(self):
        """Test the vector map with database"""
        self.assertModuleKeyValue(
            "v.info",
            map=self.test_vinfo_with_db,
            flags="etg",
            sep="=",
            precision=0.1,
            layer="1",
            reference={
                "name": self.test_vinfo_with_db,
                "num_dblinks": 1,
                "attribute_layer_name": self.test_vinfo_with_db,
                "attribute_layer_number": 1,
                "attribute_database_driver": "sqlite",
                "attribute_table": self.test_vinfo_with_db,
                "attribute_primary_key": "cat",
                "timestamp": "none",
                "map3d": 0,
                "bottom": 0.0,
                "top": 0.0,
            },
        )

    def test_info_with_db_wrong_layer(self):
        """Test the vector map with database and set the wrong layer, the output should not contain attribute data"""
        self.assertModuleKeyValue(
            "v.info",
            map=self.test_vinfo_with_db,
            flags="etg",
            sep="=",
            precision=0.1,
            layer="2",
            reference={
                "name": self.test_vinfo_with_db,
                "num_dblinks": 1,
                "timestamp": "none",
                "map3d": 0,
                "bottom": 0.0,
                "top": 0.0,
            },
        )

    def test_info_with_db_3d(self):
        """Test the vector map with database, z coordinates and timestamp"""
        self.assertModuleKeyValue(
            "v.info",
            map=self.test_vinfo_with_db_3d,
            flags="etg",
            sep="=",
            precision=0.1,
            layer="1",
            reference={
                "name": self.test_vinfo_with_db_3d,
                "num_dblinks": 1,
                "attribute_layer_name": self.test_vinfo_with_db_3d,
                "attribute_layer_number": 1,
                "attribute_database_driver": "sqlite",
                "attribute_table": self.test_vinfo_with_db_3d,
                "attribute_primary_key": "cat",
                "map3d": 1,
                "timestamp": "15 Jan 1994",
            },
        )

    def test_json(self):
        module = SimpleModule("v.info", map=self.test_vinfo_with_db_3d, format="json")
        self.runModule(module)

        expected = {
            "name": "test_vinfo_with_db_3d",
            "title": "",
            "scale": "1:1",
            "organization": "",
            "timestamp": "15 Jan 1994",
            "format": "native",
            "level": 2,
            "num_dblinks": 1,
            "attribute_layer_number": 1,
            "attribute_layer_name": "test_vinfo_with_db_3d",
            "attribute_database_driver": "sqlite",
            "attribute_table": "test_vinfo_with_db_3d",
            "attribute_primary_key": "cat",
            "projection": "Lambert Conformal Conic",
            "digitization_threshold": 0,
            "comment": "",
            "nodes": 0,
            "points": 5,
            "lines": 0,
            "boundaries": 0,
            "centroids": 0,
            "areas": 0,
            "islands": 0,
            "faces": 0,
            "kernels": 0,
            "volumes": 0,
            "holes": 0,
            "primitives": 5,
            "map3d": 1,
        }
        result = json.loads(module.outputs.stdout)

        # the following fields vary with the Grass sample data's path
        # therefore only check for their presence in the JSON output
        # and not exact values
        remove_fields = [
            "project",
            "database",
            "source_date",
            "attribute_database",
            "top",
            "bottom",
            "east",
            "west",
            "north",
            "south",
            "creator",
            "mapset",
        ]
        for field in remove_fields:
            self.assertIn(field, result)
            result.pop(field)
        self.assertDictEqual(expected, result)

    def test_json_column(self):
        module = SimpleModule(
            "v.info", map=self.test_vinfo_with_db_3d, format="json", flags="c"
        )
        self.runModule(module)

        expected_json = {
            "columns": [
                {"is_number": True, "name": "cat", "sql_type": "INTEGER"},
                {
                    "is_number": True,
                    "name": "elevation",
                    "sql_type": "DOUBLE PRECISION",
                },
            ]
        }

        result = json.loads(module.outputs.stdout)

        self.assertDictEqual(expected_json, result)

    def test_json_histroy(self):
        """Test the JSON output format of v.info with the history flag, using a history file containing multiple commands."""
        module = SimpleModule(
            "v.info", map=self.test_vinfo_with_hist, format="json", flags="h"
        )
        self.runModule(module)
        result = json.loads(module.outputs.stdout)

        expected_json = {
            "records": [
                {
                    "history_number": 1,
                    "command": 'v.random output="test_vinfo_with_hist" npoints=5 layer="-1" zmin=0 zmax=100 column_type="double precision"',
                },
                {
                    "history_number": 2,
                    "command": 'v.mkgrid map="test_vinfo_with_hist" grid=10,10 type="point"',
                },
            ]
        }

        # The following fields vary depending on the test data's path,
        # date, and user. Therefore, only check for their presence in
        # the JSON output and not for their exact values.
        remove_fields = ["mapset_path", "date", "user"]
        for record in result["records"]:
            for field in remove_fields:
                self.assertIn(field, record)
                record.pop(field)

        self.assertDictEqual(expected_json, result)

    def test_database_table(self):
        """Test the database table column and type of the two vector maps with attribute data"""
        self.assertModuleKeyValue(
            "v.info",
            map=self.test_vinfo_with_db,
            flags="c",
            sep="|",
            precision=0.1,
            reference={"INTEGER": "cat", "DOUBLE PRECISION": "elevation"},
        )

        self.assertModuleKeyValue(
            "v.info",
            map=self.test_vinfo_with_db,
            flags="c",
            sep="|",
            precision=0.1,
            reference={"INTEGER": "cat", "DOUBLE PRECISION": "elevation"},
        )


if __name__ == "__main__":
    test()
