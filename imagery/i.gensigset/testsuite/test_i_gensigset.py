import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from pathlib import Path


class TestIGensigset(TestCase):
    """Regression testsuite for i.gensigset module."""

    group_name = "test_gensigset_group"
    subgroup_name = "test_gensigset_subgroup"
    input_maps = ["gensig_band1", "gensig_band2", "gensig_band3"]
    training_map = "gensig_training"
    signature_file = "gensig_signatures"
    temp_rasters = []
    temp_signatures = []

    @classmethod
    def setUpClass(cls):
        """Set up the test environment and create input raster maps."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, rows=200, cols=200)

        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_maps[0]} = 50 + 30 * sin(row() / 20.0) + 20 * cos(col() / 15.0)",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_maps[1]} = 40 + 25 * exp(-(((row() - 100)^2 + (col() - 100)^2) / 2000))",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_maps[2]} = 30 + (row() + col()) / 10.0",
            overwrite=True,
        )
        cls.temp_rasters.extend(cls.input_maps)

        cls.runModule(
            "r.mapcalc",
            expression=(
                f"{cls.training_map} = "
                "if(row() < 60 && col() < 60, 1, "
                "if(row() > 60 && row() < 140 && col() > 60 && col() < 140, 2, "
                "if(row() > 140 && col() > 140, 3, 0)))"
            ),
            overwrite=True,
        )
        cls.temp_rasters.append(cls.training_map)

        cls.runModule(
            "i.group",
            group=cls.group_name,
            subgroup=cls.subgroup_name,
            input=",".join(cls.input_maps),
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up the test environment by removing created maps and signatures."""
        cls.runModule("g.remove", flags="f", type="group", name=cls.group_name)
        cls.runModule("g.remove", flags="f", type="raster", name=cls.temp_rasters)
        cls.runModule("i.signatures", remove=cls.temp_signatures, type="sigset")
        cls.del_temp_region()

    def _generate_signatures(self, sig_name, **kwargs):
        """Helper method to generate signatures using i.gensigset."""
        self.assertModule(
            "i.gensigset",
            trainingmap=self.training_map,
            group=self.group_name,
            subgroup=self.subgroup_name,
            signaturefile=sig_name,
            overwrite=True,
            **kwargs,
        )
        self.temp_signatures.append(sig_name)

    def _get_sigfile_path(self, sigfile_name):
        """Helper method to get the path of the generated signature file."""
        env = gs.gisenv()
        return (
            Path(env["GISDBASE"])
            / env["LOCATION_NAME"]
            / env["MAPSET"]
            / "signatures"
            / "sigset"
            / sigfile_name
            / "sig"
        )

    def _parse_sigfile(self, path):
        """Helper method to parse the signature file and extract statistics."""
        class_count = 0
        subclass_count = 0
        means = []

        with open(path) as f:
            for line in f:
                if line.strip().startswith("class:"):
                    class_count += 1
                elif line.strip().startswith("subclass:"):
                    subclass_count += 1
                elif line.strip().startswith("means:"):
                    mean_vals = list(map(float, line.strip().split(":")[1].split()))
                    means.append(mean_vals)

        return {
            "classes": class_count,
            "total_subsigs": subclass_count,
            "mean_vectors": means,
        }

    def test_basic_signature_generation(self):
        """Validate that the generated signature file contains expected classes and subsignatures."""
        sig_file = f"{self.signature_file}_basic"
        self._generate_signatures(sig_file)
        sig_path = self._get_sigfile_path(sig_file)

        self.assertTrue(
            sig_path.exists(), f"Expected signature file does not exist: {sig_path}"
        )
        stats = self._parse_sigfile(sig_path)

        self.assertEqual(
            stats["classes"], 3, "Expected 3 classes based on training map"
        )
        self.assertGreaterEqual(
            stats["total_subsigs"], 3, "Expected at least 3 subsignatures"
        )
        for vec in stats["mean_vectors"]:
            self.assertEqual(len(vec), 3, "Each mean vector should have 3 dimensions.")
            for val in vec:
                self.assertGreaterEqual(val, 0.0)
                self.assertLessEqual(val, 300.0)

    def test_maxsig_parameter(self):
        """Verify that increasing maxsig yields same or more classes and subsignatures."""
        sig_file_max3 = f"{self.signature_file}_max3"
        sig_file_max10 = f"{self.signature_file}_max10"

        self._generate_signatures(sig_file_max3, maxsig=3)
        self._generate_signatures(sig_file_max10, maxsig=10)

        path3 = self._get_sigfile_path(sig_file_max3)
        path10 = self._get_sigfile_path(sig_file_max10)

        stats3 = self._parse_sigfile(path3)
        stats10 = self._parse_sigfile(path10)

        self.assertGreaterEqual(
            stats10["classes"],
            stats3["classes"],
            "Expected maxsig=10 to result in equal or more classes than maxsig=3",
        )

        self.assertGreaterEqual(
            stats10["total_subsigs"],
            stats3["total_subsigs"],
            "Expected maxsig=10 to result in equal or more subsignatures than maxsig=3",
        )

    def test_minimal_maxsig(self):
        """Test that setting maxsig=1 produces minimal valid signatures."""
        sig_file = f"{self.signature_file}_min"
        self._generate_signatures(sig_file, maxsig=1)
        path = self._get_sigfile_path(sig_file)
        stats = self._parse_sigfile(path)

        self.assertLessEqual(
            stats["classes"],
            4,
            "Should not have more than 4 classes due to map structure",
        )
        self.assertGreaterEqual(
            stats["total_subsigs"],
            3,
            "Should produce at least one subsignature per valid class",
        )
        self.assertLessEqual(
            stats["total_subsigs"],
            4,
            "Unexpected number of subsignatures with maxsig=1",
        )


if __name__ == "__main__":
    test()
