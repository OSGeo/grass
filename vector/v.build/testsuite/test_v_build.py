import os
import ast
import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestVBuild(TestCase):
    """Test v.build output against expected output stored in vbuild_output.txt"""

    @classmethod
    def setUpClass(cls):
        """Set up a temporary region and create vector map with test features."""

        cls.use_temp_region()
        gs.run_command("v.edit", map="test_3x3_map", tool="create", overwrite=True)
        gs.run_command("g.region", n=3, s=0, e=3, w=0, res=1)
        input_data = "VERTI:\nP 1\n1 1\nL 2\n0.5 0.5\n2.5 2.5"

        with open("input_data.txt", "w") as file:
            file.write(input_data)

        # Import features into the vector map using the input file.
        gs.run_command(
            "v.edit",
            map="test_3x3_map",
            tool="add",
            type="point,line",
            input="input_data.txt",
        )

        # Run v.build (with multiple dump options) and store its output in a class variable.
        cls.build_module = gs.parse_command(
            "v.build", map="test_3x3_map", option="build,dump,sdump,cdump,fdump"
        )

        # Read the expected output from the external file.
        with open("vbuild_output.txt") as f:
            expected_str = f.read()
            cls.expected_output = ast.literal_eval(expected_str)

    @classmethod
    def tearDownClass(cls):
        """Remove created vector map and temporary files, then delete the temp region."""
        gs.run_command("g.remove", type="vector", flags="f", name="test_3x3_map")
        if os.path.exists("input_data.txt"):
            os.remove("input_data.txt")
        cls.del_temp_region()

    def test_vbuild_output(self):
        """Compare the v.build output (build_module) to the expected output."""

        # Compare the dictionaries
        if self.build_module == self.expected_output:
            print("Outputs match!")
        else:
            print("Outputs differ!")
            for key1, key2 in zip(
                self.build_module.keys(), self.expected_output.keys()
            ):
                val1 = self.build_module.get(key1)
                val2 = self.expected_output.get(key1)
                if key1 != key2:
                    print(f"Calculated: {key1}\nExpected: {key2}\n")
                if val1 != val2:
                    print(f"Calculated: {val1}\nExpected: {val2}\n")


if __name__ == "__main__":
    test()
