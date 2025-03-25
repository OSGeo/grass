import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestISmap(TestCase):
    """Regression tests for i.smap GRASS GIS module."""

    group_name = "test_smap_group"
    subgroup_name = "test_smap_subgroup"
    input_maps = ["synth_map1", "synth_map2", "synth_map3"]
    training_map = "training_areas"
    signature_file = "smap_sig"
    output_map = "smap_output"
    goodness_map = "smap_goodness"
    temp_rasters = []

    @classmethod
    def setUpClass(cls):
        """Set up the input data and configure test environment."""
        cls.use_temp_region()
        cls.runModule("g.region", n=50, s=0, e=50, w=0, rows=100, cols=100)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_maps[0]} = 10 * sin(row() / 10.0) + 10 * sin(col() / 10.0) + 10",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_maps[1]} = 10 * cos(row() / 10.0) + 10 * cos(col() / 10.0) + 10",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_maps[2]} = 20 * exp(-((row() - 50)^2 + (col() - 50)^2) / 500)",
            overwrite=True,
        )
        cls.temp_rasters.extend(cls.input_maps)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.training_map} = if(row() < 40 && col() < 40, 1, if(row() > 60 && col() > 60, 3, 2))",
            overwrite=True,
        )
        cls.temp_rasters.append(cls.training_map)
        cls.runModule(
            "r.colors", map=cls.training_map, rules="-", stdin="1 red\n2 green\n3 blue"
        )
        cls.runModule(
            "i.group",
            group=cls.group_name,
            subgroup=cls.subgroup_name,
            input=",".join(cls.input_maps),
        )
        cls.runModule(
            "i.gensigset",
            trainingmap=cls.training_map,
            group=cls.group_name,
            subgroup=cls.subgroup_name,
            signaturefile=cls.signature_file,
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up generated data and reset the region."""
        cls.runModule("g.remove", flags="f", type="group", name=cls.group_name)
        cls.temp_rasters.append(cls.output_map)
        cls.runModule("g.remove", flags="f", type="raster", name=cls.temp_rasters)
        cls.runModule("i.signatures", remove=cls.signature_file, type="sigset")
        cls.del_temp_region()

    def _run_smap(self, output_name, **kwargs):
        """Helper function to execute i.smap with common parameters."""
        self.assertModule(
            "i.smap",
            group=self.group_name,
            subgroup=self.subgroup_name,
            signaturefile=self.signature_file,
            output=output_name,
            overwrite=True,
            **kwargs,
        )
        self.assertRasterExists(output_name)
        return output_name

    def test_basic_classification(self):
        """Verify basic SMAP classification produces valid results."""
        self._run_smap(f"{self.output_map}_basic")
        self.assertRasterExists(f"{self.output_map}_basic")
        self.temp_rasters.append(f"{self.output_map}_basic")

        stats = gs.read_command("r.stats", flags="cn", input=f"{self.output_map}_basic")
        unique_classes = len(
            [line for line in stats.split("\n") if line.strip() and " " in line]
        )
        self.assertEqual(
            unique_classes, 3, f"Expected 3 classes in output, found {unique_classes}"
        )

    def test_with_goodness_map(self):
        """
        Validate goodness of fit map generation and
        verify if map values fall within expected statistical range
        """
        self._run_smap(f"{self.output_map}_goodness", goodness=self.goodness_map)
        self.assertRasterExists(self.goodness_map)
        self.temp_rasters.extend([self.goodness_map, f"{self.output_map}_goodness"])

        reference_stats = {
            "min": -7.328390,
            "max": 4.495414,
        }
        self.assertRasterFitsUnivar(self.goodness_map, reference_stats, precision=1e-6)

    def test_maximum_likelihood_flag(self):
        """Compare SMAP and Maximum Likelihood Estimation (-m flag) approaches"""
        self._run_smap(f"{self.output_map}_smap")
        self.assertRasterExists(f"{self.output_map}_smap")
        self.temp_rasters.append(f"{self.output_map}_smap")

        self._run_smap(f"{self.output_map}_mle", flags="m")
        self.assertRasterExists(f"{self.output_map}_mle")
        self.temp_rasters.append(f"{self.output_map}_mle")

        kappa_result = gs.parse_command(
            "r.kappa",
            classification=f"{self.output_map}_smap",
            reference=f"{self.output_map}_mle",
            format="json",
        )

        overall_accuracy = float(kappa_result["overall_accuracy"])

        self.assertGreater(
            overall_accuracy, 95.0, "Overall accuracy should be reasonably high"
        )
        self.assertLess(
            overall_accuracy, 100.0, "SMAP and ML should not have 100% agreement"
        )

    def test_block_size(self):
        """Ensure block size parameter doesn't affect results"""
        baseline = self._run_smap(f"{self.output_map}_baseline")
        bs1 = self._run_smap(f"{self.output_map}_bs1", blocksize=256)
        bs2 = self._run_smap(f"{self.output_map}_bs2", blocksize=1024)

        self.temp_rasters.extend(
            [
                f"{self.output_map}_baseline",
                f"{self.output_map}_bs1",
                f"{self.output_map}_bs2",
            ]
        )

        self.assertRastersEqual(baseline, bs1)
        self.assertRastersEqual(baseline, bs2)


if __name__ == "__main__":
    test()
