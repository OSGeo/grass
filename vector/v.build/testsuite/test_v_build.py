import json
import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestVBuild(TestCase):
    """Test v.build output against expected output stored in vbuild_output"""

    @classmethod
    def setUpClass(cls):
        """Set up a temporary region and create vector map with test features."""
        cls.use_temp_region()
        input_data = "P 1\n1 1\nL 2\n0.5 0.5\n2.5 2.5"
        gs.write_command(
            "v.in.ascii",
            input="-",
            format="standard",
            stdin=input_data,
            output="test_3x3_map",
            flags="n",
            overwrite=True,
        )

        # Read the expected output.
        cls.vbuild_output = """---------- TOPOLOGY DUMP ----------
Topology format: native
-----------------------------------
N,S,E,W,T,B: 2.500000, 0.500000, 2.500000, 0.500000, 0.000000, 0.000000
-----------------------------------
Nodes (2 nodes, alive + dead):
node = 1, n_lines = 1, xyz = 0.500000, 0.500000, 0.000000
  line =   2, type = 2, angle = 0.785398 (45.0000)
node = 2, n_lines = 1, xyz = 2.500000, 2.500000, 0.000000
  line =  -2, type = 2, angle = -2.356194 (225.0000)
-----------------------------------
Lines (2 lines, alive + dead):
line = 1, type = 1, offset = 18
line = 2, type = 2, offset = 35, n1 = 1, n2 = 2
-----------------------------------
Areas (0 areas, alive + dead):
-----------------------------------
Islands (0 islands, alive + dead):
---------- SPATIAL INDEX DUMP ----------
Nodes
Node level=0  count=2
  Branch 0  id = 1  0.500000 0.500000 0.000000 0.500000 0.500000 0.000000
  Branch 1  id = 2  2.500000 2.500000 0.000000 2.500000 2.500000 0.000000
Lines
Node level=0  count=2
  Branch 0  id = 1  1.000000 1.000000 0.000000 1.000000 1.000000 0.000000
  Branch 1  id = 2  0.500000 0.500000 0.000000 2.500000 2.500000 0.000000
Areas
Node level=0  count=0
Isles
Node level=0  count=0
---------- CATEGORY INDEX DUMP: Number of layers: 1 --------------------------------------
Layer      0  number of unique cats:       1  number of cats:       2  number of types: 2
------------------------------------------------------------------------------------------
            type |     count
               1 |         1
               2 |         1
 category | type | line/area
        0 |    1 |         1
        0 |    2 |         2
------------------------------------------------------------------------------------------"""

    @classmethod
    def tearDownClass(cls):
        """Remove created vector map and temporary files, then delete the temp region."""
        gs.run_command("g.remove", type="vector", flags="f", name="test_3x3_map")
        cls.del_temp_region()

    def test_vbuild_output(self):
        """Compare the v.build output (build_module) to the expected output."""
        # Run v.build (with multiple dump options) and store its output in a class variable.
        build_module = gs.read_command(
            "v.build",
            map="test_3x3_map",
            option="build,dump,sdump,cdump,fdump",
            quiet=True,
        ).strip()

        # Filter out the map meta data from the output.
        output_lines = build_module.split("\n")
        filtered_output = "\n".join(
            [line for line in output_lines if not line.startswith("Map:")]
        )
        self.assertMultiLineEqual(filtered_output, self.vbuild_output)

    def test_vbuild_json_output(self):
        output = gs.read_command(
            "v.build",
            map="test_3x3_map",
            option="sdump,cdump",
            format="json",
            quiet=True,
        )

        data = json.loads(output)

        self.assertIsInstance(data, dict)
        self.assertEqual(data.get("module"), "v.build")

        self.assertIn("nodes", data)
        self.assertIsInstance(data["nodes"], list)
        self.assertEqual(len(data["nodes"]), 2)

        for node in data["nodes"]:
            self.assertIsInstance(node.get("node_id"), int)
            self.assertIsInstance(node.get("n_lines"), int)
            self.assertGreater(node["node_id"], 0)

        self.assertIn("categories", data)
        self.assertIsInstance(data["categories"], list)
        self.assertEqual(len(data["categories"]), 1)

        cat_info = data["categories"][0]
        self.assertEqual(cat_info.get("layer"), 0)
        self.assertEqual(cat_info.get("n_cats"), 2)
        self.assertIsInstance(cat_info.get("n_cats"), int)

        self.assertEqual(data.get("type"), "spatial_index_dump")
        self.assertEqual(data.get("action_c"), "category_index_dump")


if __name__ == "__main__":
    test()
