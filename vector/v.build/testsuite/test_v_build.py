import ast
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
        # Run v.build (with multiple dump options) and store its output in a class variable.
        cls.build_module = gs.parse_command(
            "v.build", map="test_3x3_map", option="build,dump,sdump,cdump,fdump"
        )
        # Read the expected output.
        vbuild_output = """{'---------- TOPOLOGY DUMP ----------': None, 'Map:             test_3x3_map@PERMANENT': None, 'Topology format: native': None, '-----------------------------------': None, 'N,S,E,W,T,B: 2.500000, 0.500000, 2.500000, 0.500000, 0.000000, 0.000000': None, 'Nodes (2 nodes, alive + dead):': None, 'node': '2, n_lines = 1, xyz = 2.500000, 2.500000, 0.000000', 'line': '2, type = 2, offset = 35, n1 = 1, n2 = 2', 'Lines (2 lines, alive + dead):': None, 'Areas (0 areas, alive + dead):': None, 'Islands (0 islands, alive + dead):': None, '---------- SPATIAL INDEX DUMP ----------': None, 'Nodes': None, 'Node level': '0  count=0', 'Branch 0  id': '1  1.000000 1.000000 0.000000 1.000000 1.000000 0.000000', 'Branch 1  id': '2  0.500000 0.500000 0.000000 2.500000 2.500000 0.000000', 'Lines': None, 'Areas': None, 'Isles': None, '---------- CATEGORY INDEX DUMP: Number of layers: 1 --------------------------------------': None, 'Layer      0  number of unique cats:       1  number of cats:       2  number of types: 2': None, '------------------------------------------------------------------------------------------': None, 'type |     count': None, '1 |         1': None, '2 |         1': None, 'category | type | line/area': None, '0 |    1 |         1': None, '0 |    2 |         2': None}"""
        cls.expected_output = ast.literal_eval(vbuild_output)

    @classmethod
    def tearDownClass(cls):
        """Remove created vector map and temporary files, then delete the temp region."""
        gs.run_command("g.remove", type="vector", flags="f", name="test_3x3_map")
        cls.del_temp_region()

    def test_vbuild_output(self):
        """Compare the v.build output (build_module) to the expected output."""
        self.assertEqual(
            set(self.build_module.keys()),
            set(self.expected_output.keys()),
            "The sets of keys differ between calculated and expected output.",
        )
        # Then, iterate over the keys and assert that each value matches.
        for key in self.build_module:
            self.assertEqual(
                self.build_module.get(key),
                self.expected_output.get(key),
                msg=f"Mismatch for '{key}': Calculated {self.build_module.get(key)} vs Expected {self.expected_output.get(key)}",
            )


if __name__ == "__main__":
    test()
