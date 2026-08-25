import json
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestVWhatModule(TestCase):
    """Unit tests for the v.what module."""

    # maps used by tests
    area_vector = "area_vector"
    line_vector = "line_vector"
    point_vector = "point_vector"
    point_vector_3d = "point_vector_3d"
    line_vector_3d = "line_vector_3d"

    @classmethod
    def setUpClass(cls):
        """Setup temporary region and test vector maps."""
        cls.use_temp_region()
        cls.runModule("g.region", s=0, n=5, w=0, e=5, res=1)

        # test grids for area/line/point vector maps
        cls.runModule("v.mkgrid", map=cls.area_vector, grid=[5, 5], type="area")
        cls.runModule("v.mkgrid", map=cls.line_vector, grid=[5, 5], type="line")
        cls.runModule("v.mkgrid", map=cls.point_vector, grid=[5, 5], type="point")

        # point vector to 3D
        cls.runModule(
            "v.to.3d", input=cls.point_vector, output=cls.point_vector_3d, height=50
        )

        # 3D line vector from an existing map
        cls.runModule(
            "v.to.3d",
            input="roadsmajor",
            output=cls.line_vector_3d,
            height=50,
            type="line",
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region and all created test vectors."""
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            flags="f",
            type="vector",
            name=[
                cls.area_vector,
                cls.line_vector,
                cls.point_vector,
                cls.point_vector_3d,
                cls.line_vector_3d,
            ],
        )

    def assert_json_equal(self, expected, actual):
        if isinstance(expected, dict):
            self.assertIsInstance(actual, dict)
            self.assertCountEqual(expected.keys(), actual.keys())
            for key in expected:
                # Skip exact match for keys containing "mapset" or "database" because their values vary
                if key in {"mapset", "database"}:
                    continue
                self.assert_json_equal(expected[key], actual[key])
        elif isinstance(expected, list):
            self.assertIsInstance(actual, list)
            self.assertEqual(len(expected), len(actual))
            for exp_item, act_item in zip(expected, actual, strict=True):
                self.assert_json_equal(exp_item, act_item)
        elif isinstance(expected, float):
            self.assertAlmostEqual(expected, actual, places=6)
        else:
            self.assertEqual(expected, actual)

    def test_line_vector_nodes_and_distance_json(self):
        """v.what should return nodes and distance info for a line vector."""
        module = SimpleModule(
            "v.what",
            map=[self.line_vector],
            coordinates=[2.5, 2.5, 4, 4],
            distance=0.5,
            flags="da",
            format="json",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {
                "coordinate": {"easting": 2.5, "northing": 2.5},
                "map": "line_vector",
                "mapset": "...",
                "id": 48,
                "type": "line",
                "feature_max_distance": 0.5,
                "left": 0,
                "right": 0,
                "length": 1,
                "nodes": [
                    {
                        "id": 16,
                        "number_lines": 4,
                        "coordinate_x": 3,
                        "coordinate_y": 2,
                        "coordinate_z": 0,
                        "lines": [
                            {"id": -47, "angle": -1.5707963705062866},
                            {"id": 14, "angle": 0},
                            {"id": 48, "angle": 1.5707963705062866},
                            {"id": -13, "angle": 3.1415927410125732},
                        ],
                    },
                    {
                        "id": 22,
                        "number_lines": 4,
                        "coordinate_x": 3,
                        "coordinate_y": 3,
                        "coordinate_z": 0,
                        "lines": [
                            {"id": -48, "angle": -1.5707963705062866},
                            {"id": 19, "angle": 0},
                            {"id": 49, "angle": 1.5707963705062866},
                            {"id": -18, "angle": 3.1415927410125732},
                        ],
                    },
                ],
                "data": [],
            },
            {
                "coordinate": {"easting": 4, "northing": 4},
                "map": "line_vector",
                "mapset": "...",
                "id": 24,
                "type": "line",
                "feature_max_distance": 0.5,
                "left": 0,
                "right": 0,
                "length": 1,
                "nodes": [
                    {
                        "id": 28,
                        "number_lines": 4,
                        "coordinate_x": 3,
                        "coordinate_y": 4,
                        "coordinate_z": 0,
                        "lines": [
                            {"id": -49, "angle": -1.5707963705062866},
                            {"id": 24, "angle": 0},
                            {"id": 50, "angle": 1.5707963705062866},
                            {"id": -23, "angle": 3.1415927410125732},
                        ],
                    },
                    {
                        "id": 29,
                        "number_lines": 4,
                        "coordinate_x": 4,
                        "coordinate_y": 4,
                        "coordinate_z": 0,
                        "lines": [
                            {"id": -54, "angle": -1.5707963705062866},
                            {"id": 25, "angle": 0},
                            {"id": 55, "angle": 1.5707963705062866},
                            {"id": -24, "angle": 3.1415927410125732},
                        ],
                    },
                ],
                "data": [],
            },
        ]
        self.assert_json_equal(expected, result)

    def test_line_vector_features_list_json(self):
        """v.what should list multiple features for a line vector."""
        module = SimpleModule(
            "v.what",
            map=[self.line_vector],
            coordinates=[2.5, 2.5, 4, 4],
            distance=0.5,
            flags="ma",
            format="json",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {
                "coordinate": {"easting": 2.5, "northing": 2.5},
                "map": "line_vector",
                "mapset": "...",
                "features": [
                    {"id": 12, "type": "line", "length": 1, "data": []},
                    {"id": 17, "type": "line", "length": 1, "data": []},
                    {"id": 14, "type": "line", "length": 1, "data": []},
                    {"id": 47, "type": "line", "length": 1, "data": []},
                    {"id": 48, "type": "line", "length": 1, "data": []},
                    {"id": 49, "type": "line", "length": 1, "data": []},
                    {"id": 42, "type": "line", "length": 1, "data": []},
                    {"id": 13, "type": "line", "length": 1, "data": []},
                    {"id": 18, "type": "line", "length": 1, "data": []},
                    {"id": 44, "type": "line", "length": 1, "data": []},
                    {"id": 43, "type": "line", "length": 1, "data": []},
                    {"id": 19, "type": "line", "length": 1, "data": []},
                ],
            },
            {
                "coordinate": {"easting": 4, "northing": 4},
                "map": "line_vector",
                "mapset": "...",
                "features": [
                    {"id": 24, "type": "line", "length": 1, "data": []},
                    {"id": 54, "type": "line", "length": 1, "data": []},
                    {"id": 25, "type": "line", "length": 1, "data": []},
                    {"id": 55, "type": "line", "length": 1, "data": []},
                ],
            },
        ]
        self.assert_json_equal(expected, result)

    def test_line_vector_features_with_nodes_json(self):
        """v.what should return full feature/node details for a line vector."""
        module = SimpleModule(
            "v.what",
            map=[self.line_vector],
            coordinates=[4, 4],
            distance=0.5,
            flags="dma",
            format="json",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {
                "coordinate": {"easting": 4, "northing": 4},
                "map": "line_vector",
                "mapset": "...",
                "features": [
                    {
                        "id": 24,
                        "type": "line",
                        "feature_max_distance": 0.5,
                        "left": 0,
                        "right": 0,
                        "length": 1,
                        "nodes": [
                            {
                                "id": 28,
                                "number_lines": 4,
                                "coordinate_x": 3,
                                "coordinate_y": 4,
                                "coordinate_z": 0,
                                "lines": [
                                    {"id": -49, "angle": -1.5707963705062866},
                                    {"id": 24, "angle": 0},
                                    {"id": 50, "angle": 1.5707963705062866},
                                    {"id": -23, "angle": 3.1415927410125732},
                                ],
                            },
                            {
                                "id": 29,
                                "number_lines": 4,
                                "coordinate_x": 4,
                                "coordinate_y": 4,
                                "coordinate_z": 0,
                                "lines": [
                                    {"id": -54, "angle": -1.5707963705062866},
                                    {"id": 25, "angle": 0},
                                    {"id": 55, "angle": 1.5707963705062866},
                                    {"id": -24, "angle": 3.1415927410125732},
                                ],
                            },
                        ],
                        "data": [],
                    },
                    {
                        "id": 54,
                        "type": "line",
                        "feature_max_distance": 0.5,
                        "left": 0,
                        "right": 0,
                        "length": 1,
                        "nodes": [
                            {
                                "id": 23,
                                "number_lines": 4,
                                "coordinate_x": 4,
                                "coordinate_y": 3,
                                "coordinate_z": 0,
                                "lines": [
                                    {"id": -53, "angle": -1.5707963705062866},
                                    {"id": 20, "angle": 0},
                                    {"id": 54, "angle": 1.5707963705062866},
                                    {"id": -19, "angle": 3.1415927410125732},
                                ],
                            },
                            {
                                "id": 29,
                                "number_lines": 4,
                                "coordinate_x": 4,
                                "coordinate_y": 4,
                                "coordinate_z": 0,
                                "lines": [
                                    {"id": -54, "angle": -1.5707963705062866},
                                    {"id": 25, "angle": 0},
                                    {"id": 55, "angle": 1.5707963705062866},
                                    {"id": -24, "angle": 3.1415927410125732},
                                ],
                            },
                        ],
                        "data": [],
                    },
                    {
                        "id": 25,
                        "type": "line",
                        "feature_max_distance": 0.5,
                        "left": 0,
                        "right": 0,
                        "length": 1,
                        "nodes": [
                            {
                                "id": 29,
                                "number_lines": 4,
                                "coordinate_x": 4,
                                "coordinate_y": 4,
                                "coordinate_z": 0,
                                "lines": [
                                    {"id": -54, "angle": -1.5707963705062866},
                                    {"id": 25, "angle": 0},
                                    {"id": 55, "angle": 1.5707963705062866},
                                    {"id": -24, "angle": 3.1415927410125732},
                                ],
                            },
                            {
                                "id": 30,
                                "number_lines": 3,
                                "coordinate_x": 5,
                                "coordinate_y": 4,
                                "coordinate_z": 0,
                                "lines": [
                                    {"id": -59, "angle": -1.5707963705062866},
                                    {"id": 60, "angle": 1.5707963705062866},
                                    {"id": -25, "angle": 3.1415927410125732},
                                ],
                            },
                        ],
                        "data": [],
                    },
                    {
                        "id": 55,
                        "type": "line",
                        "feature_max_distance": 0.5,
                        "left": 0,
                        "right": 0,
                        "length": 1,
                        "nodes": [
                            {
                                "id": 29,
                                "number_lines": 4,
                                "coordinate_x": 4,
                                "coordinate_y": 4,
                                "coordinate_z": 0,
                                "lines": [
                                    {"id": -54, "angle": -1.5707963705062866},
                                    {"id": 25, "angle": 0},
                                    {"id": 55, "angle": 1.5707963705062866},
                                    {"id": -24, "angle": 3.1415927410125732},
                                ],
                            },
                            {
                                "id": 35,
                                "number_lines": 3,
                                "coordinate_x": 4,
                                "coordinate_y": 5,
                                "coordinate_z": 0,
                                "lines": [
                                    {"id": -55, "angle": -1.5707963705062866},
                                    {"id": 30, "angle": 0},
                                    {"id": -29, "angle": 3.1415927410125732},
                                ],
                            },
                        ],
                        "data": [],
                    },
                ],
            }
        ]
        self.assert_json_equal(expected, result)

    def test_point_map_hospitals_attributes_json(self):
        """v.what should include attribute information for the 'hospitals' point vector."""
        module = SimpleModule(
            "v.what",
            map="hospitals",
            coordinates=[542690.4, 204802.7],
            distance=2000000,
            flags="a",
            format="json",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {
                "coordinate": {"easting": 542690.4, "northing": 204802.7},
                "map": "hospitals",
                "mapset": "...",
                "id": 22,
                "type": "point",
                "data": [
                    {
                        "layer": 1,
                        "category": 22,
                        "attributes": {
                            "cat": 22,
                            "OBJECTID": 22,
                            "AREA": 0,
                            "PERIMETER": 0,
                            "HLS_": 22,
                            "HLS_ID": 22,
                            "NAME": "Randolph Hospital",
                            "ADDRESS": "364 White Oak St",
                            "CITY": "Asheboro",
                            "ZIP": "27203",
                            "COUNTY": "Randolph",
                            "PHONE": "(336) 625-5151",
                            "CANCER": "yes",
                            "POLYGONID": 0,
                            "SCALE": 1,
                            "ANGLE": 1,
                        },
                    }
                ],
            }
        ]
        self.assert_json_equal(expected, result)

    def test_area_vector_area_isles_and_categories_json(self):
        """v.what should return area, isles and category details for 'urbanarea' vector."""
        module = SimpleModule(
            "v.what",
            map="urbanarea",
            coordinates=[643554.273559, 227215.046524],
            distance=20000,
            flags="dai",
            format="json",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {
                "coordinate": {"easting": 643554.273559, "northing": 227215.046524},
                "map": "urbanarea",
                "mapset": "...",
                "type": "area",
                "area": 408,
                "number_isles": 1,
                "isles": [89],
                "island": 91,
                "island_area": 0,
                "data": [
                    {
                        "layer": 1,
                        "category": 55,
                        "driver": "sqlite",
                        "database": "...",
                        "table": "urbanarea",
                        "key_column": "cat",
                        "attributes": {
                            "cat": 55,
                            "OBJECTID": 55,
                            "UA": "73261",
                            "NAME": "Raleigh",
                            "UA_TYPE": "UA",
                        },
                    }
                ],
            }
        ]
        self.assert_json_equal(expected, result)

    def test_area_vector_features_list_json(self):
        """v.what should list area features for area vector."""
        module = SimpleModule(
            "v.what",
            map=[self.area_vector],
            coordinates=[2.5, 2.5, 4, 4],
            distance=0.5,
            flags="m",
            format="json",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {
                "coordinate": {"easting": 2.5, "northing": 2.5},
                "map": "area_vector",
                "mapset": "...",
                "features": [
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 7}],
                    },
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 8}],
                    },
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 13}],
                    },
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 18}],
                    },
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 17}],
                    },
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 12}],
                    },
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 19}],
                    },
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 14}],
                    },
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 9}],
                    },
                ],
            },
            {
                "coordinate": {"easting": 4, "northing": 4},
                "map": "area_vector",
                "mapset": "...",
                "features": [
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 24}],
                    },
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 19}],
                    },
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 20}],
                    },
                    {
                        "type": "area",
                        "sq_meters": 1,
                        "hectares": 0.0001,
                        "acres": 0.0002471053814671654,
                        "sq_miles": 3.861021585424459e-07,
                        "data": [{"layer": 1, "category": 25}],
                    },
                ],
            },
        ]
        self.assert_json_equal(expected, result)

    def test_area_vector_features_with_area_json(self):
        """v.what should return area sizes and data for area vector."""
        module = SimpleModule(
            "v.what",
            map=[self.area_vector],
            coordinates=[2.5, 2.5, 4, 4],
            distance=0.5,
            flags="md",
            format="json",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {
                "coordinate": {"easting": 2.5, "northing": 2.5},
                "map": "area_vector",
                "mapset": "...",
                "features": [
                    {
                        "type": "area",
                        "area": 7,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 7}],
                    },
                    {
                        "type": "area",
                        "area": 8,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 8}],
                    },
                    {
                        "type": "area",
                        "area": 13,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 13}],
                    },
                    {
                        "type": "area",
                        "area": 18,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 18}],
                    },
                    {
                        "type": "area",
                        "area": 17,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 17}],
                    },
                    {
                        "type": "area",
                        "area": 12,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 12}],
                    },
                    {
                        "type": "area",
                        "area": 19,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 19}],
                    },
                    {
                        "type": "area",
                        "area": 14,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 14}],
                    },
                    {
                        "type": "area",
                        "area": 9,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 9}],
                    },
                ],
            },
            {
                "coordinate": {"easting": 4, "northing": 4},
                "map": "area_vector",
                "mapset": "...",
                "features": [
                    {
                        "type": "area",
                        "area": 24,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 24}],
                    },
                    {
                        "type": "area",
                        "area": 19,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 19}],
                    },
                    {
                        "type": "area",
                        "area": 20,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 20}],
                    },
                    {
                        "type": "area",
                        "area": 25,
                        "number_isles": 0,
                        "isles": [],
                        "island": 1,
                        "island_area": 0,
                        "data": [{"layer": 1, "category": 25}],
                    },
                ],
            },
        ]
        self.assert_json_equal(expected, result)

    def test_point_vector_3d_height_json(self):
        """v.what should include point height for 3D point vector."""
        module = SimpleModule(
            "v.what",
            map=[self.point_vector_3d],
            coordinates=[2.5, 2.5, 4, 4],
            distance=0.5,
            flags="d",
            format="json",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {
                "coordinate": {"easting": 2.5, "northing": 2.5},
                "map": "point_vector_3d",
                "mapset": "...",
                "id": 13,
                "type": "point",
                "feature_max_distance": 0.5,
                "left": 0,
                "right": 0,
                "nodes": [],
                "point_height": 50,
                "data": [{"layer": 1, "category": 13}],
            },
        ]
        self.assert_json_equal(expected, result)

    def test_line_vector_3d_height_json(self):
        """v.what should include line height for 3D line vector."""
        module = SimpleModule(
            "v.what",
            map=[self.line_vector_3d],
            coordinates=[542690.4, 204802.7],
            distance=2000000,
            format="json",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {
                "coordinate": {"easting": 542690.4, "northing": 204802.7},
                "map": "line_vector_3d",
                "mapset": "...",
                "id": 329,
                "type": "line",
                "length": 2736.394141411282,
                "line_height": 50,
                "data": [{"layer": 1, "category": 329}],
            }
        ]
        self.assert_json_equal(expected, result)


if __name__ == "__main__":
    test()
