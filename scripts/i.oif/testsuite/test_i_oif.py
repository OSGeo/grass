import os
from pathlib import Path

import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestOIF(TestCase):
    """Regression tests for i.oif GRASS module."""

    band1 = "oif_test_band1"
    band2 = "oif_test_band2"
    band3 = "oif_test_band3"
    band4 = "oif_test_band4"
    const_band = "oif_constant"
    input_rasters = [band1, band2, band3, band4]
    output_file = "oif_test_output.txt"

    @classmethod
    def setUpClass(cls):
        """Set up the test environment and create input raster maps."""
        cls.use_temp_region()
        cls.runModule("g.region", res=10, n=40, s=0, w=0, e=40)

        cls.runModule("r.mapcalc", expression=f"{cls.band1} = row()", overwrite=True)
        cls.runModule("r.mapcalc", expression=f"{cls.band2} = col()", overwrite=True)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.band3} = row() + col()", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", expression=f"{cls.band4} = row() * 2 + col()", overwrite=True
        )
        cls.runModule("r.mapcalc", expression=f"{cls.const_band} = 42", overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Clean up the test environment by removing created maps and files."""
        if Path(cls.output_file).exists():
            os.remove(cls.output_file)

        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=cls.input_rasters + [cls.const_band],
        )
        cls.del_temp_region()

    def test_oif_calculation_and_order(self):
        """
        Check if OIF values are calculated correctly and if the output is sorted in descending order.
        """
        oif_proc = gs.read_command(
            "i.oif",
            input=",".join(self.input_rasters),
        )
        expected_output = (
            f"{self.band1}, {self.band2}, {self.band4}:  3.5301\n"
            f"{self.band1}, {self.band2}, {self.band3}:  2.6992\n"
            f"{self.band2}, {self.band3}, {self.band4}:  2.4723\n"
            f"{self.band1}, {self.band3}, {self.band4}:  2.0387\n"
        )
        self.assertEqual(
            oif_proc,
            expected_output,
            msg="OIF calculation or sorting order has changed.",
        )

    def test_shell_script_style_output(self):
        """
        Test shell script output flag (-g). The output format should
        be different from the default (more compact, comma-separated).
        """

        oif_proc_g = gs.read_command(
            "i.oif",
            flags="g",
            input=",".join(self.input_rasters),
        )

        expected_output_g = (
            f"{self.band1},{self.band2},{self.band4}:3.5301\n"
            f"{self.band1},{self.band2},{self.band3}:2.6992\n"
            f"{self.band2},{self.band3},{self.band4}:2.4723\n"
            f"{self.band1},{self.band3},{self.band4}:2.0387\n"
        )

        self.assertEqual(
            oif_proc_g,
            expected_output_g,
            msg="Shell script style output (-g) is incorrect.",
        )

    def test_serial_vs_parallel_consistency(self):
        """
        Verify that parallelization does not affect numerical results.
        """
        parallel_proc = gs.read_command(
            "i.oif",
            input=",".join(self.input_rasters),
        )

        serial_proc = gs.read_command(
            "i.oif",
            flags="s",
            input=",".join(self.input_rasters),
        )

        self.assertEqual(
            parallel_proc,
            serial_proc,
            msg="Serial and parallel computations produce different results.",
        )

    def test_output_to_file(self):
        """
        Test that the module correctly writes output to a file when the output parameter is given.
        """
        self.runModule(
            "i.oif",
            input=",".join(self.input_rasters),
            output=self.output_file,
            overwrite=True,
        )

        self.assertTrue(
            Path(self.output_file).exists(), msg="Output file was not created."
        )

        content = Path(self.output_file).read_text()

        expected_content = (
            f"{self.band1}, {self.band2}, {self.band4}:  3.5301\n"
            f"{self.band1}, {self.band2}, {self.band3}:  2.6992\n"
            f"{self.band2}, {self.band3}, {self.band4}:  2.4723\n"
            f"{self.band1}, {self.band3}, {self.band4}:  2.0387\n"
        )
        self.assertEqual(
            content,
            expected_content,
            msg="Content of output file is incorrect.",
        )

    def test_constant_raster_yields_nan(self):
        """
        Test that bands with zero variance (constant rasters) produce NaN in OIF calculation.
        """

        rasters = self.input_rasters + [self.const_band]
        output = gs.read_command(
            "i.oif",
            input=",".join(rasters),
        )

        expected_output = (
            f"{self.band1}, {self.band2}, {self.band4}:  3.5301\n"
            f"{self.band1}, {self.band2}, {self.band3}:  2.6992\n"
            f"{self.band1}, {self.band2}, {self.const_band}:  nan\n"
            f"{self.band1}, {self.band3}, {self.band4}:  2.0387\n"
            f"{self.band1}, {self.band3}, {self.const_band}:  nan\n"
            f"{self.band1}, {self.band4}, {self.const_band}:  nan\n"
            f"{self.band2}, {self.band3}, {self.band4}:  2.4723\n"
            f"{self.band2}, {self.band3}, {self.const_band}:  nan\n"
            f"{self.band2}, {self.band4}, {self.const_band}:  nan\n"
            f"{self.band3}, {self.band4}, {self.const_band}:  nan\n"
        )
        self.assertEqual(
            output,
            expected_output,
            msg="Output doesn't match the expected output.",
        )


if __name__ == "__main__":
    test()
