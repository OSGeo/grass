from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import grass.script as gs


class TestICCA(TestCase):
    """Regression and invariance tests for the i.cca GRASS GIS module."""

    group_name = "cca_group"
    subgroup_name = "cca_subgroup"
    input_maps = ["band1", "band2", "band3"]
    training_map = "cca_training"
    signature_file = "cca_sig"
    output_prefix = "cca_output"
    temp_rasters = []

    @classmethod
    def setUpClass(cls):
        """Set up the test environment and create input raster maps."""
        cls.use_temp_region()
        cls.runModule("g.region", n=20, s=0, e=20, w=0, rows=200, cols=200)

        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_maps[0]} = 50 + 30 * sin(row() / 20.0) + 20 * cos(col() / 15.0)",
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_maps[1]} = 40 + 25 * exp(-(((row() - 100)^2 + (col() - 100)^2) / 2000.0))",
        )
        cls.runModule(
            "r.mapcalc", expression=f"{cls.input_maps[2]} = 30 + (row() + col()) / 10.0"
        )
        cls.temp_rasters.extend(cls.input_maps)

        cls.runModule(
            "r.mapcalc",
            expression=(
                f"{cls.training_map} = if(row() < 60 && col() < 60, 1, "
                f"if(row() >= 60 && row() < 140 && col() >= 60 && col() < 140, 2, "
                f"if(row() >= 140 && col() >= 140, 3, null())))"
            ),
        )
        cls.temp_rasters.append(cls.training_map)

        cls.runModule(
            "i.group",
            group=cls.group_name,
            subgroup=cls.subgroup_name,
            input=",".join(cls.input_maps),
        )
        cls.runModule(
            "i.gensig",
            trainingmap=cls.training_map,
            group=cls.group_name,
            subgroup=cls.subgroup_name,
            signaturefile=cls.signature_file,
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up the test environment by removing created maps and signatures."""
        cls.runModule("g.remove", flags="f", type="raster", name=cls.temp_rasters)
        cls.runModule("g.remove", flags="f", type="group", name=cls.group_name)
        cls.runModule("i.signatures", type="sig", remove=cls.signature_file)

    def setUp(self):
        """Run i.cca before each test and register outputs."""
        self.assertModule(
            "i.cca",
            group=self.group_name,
            subgroup=self.subgroup_name,
            signature=self.signature_file,
            output=self.output_prefix,
        )

        for i in range(1, 4):
            name = f"{self.output_prefix}.{i}"
            self.assertRasterExists(name)
            if name not in self.temp_rasters:
                self.temp_rasters.append(name)

    def test_valid_outputs(self):
        """Validate that i.cca output statistics match expected values."""
        expected_stats = {
            "cca_output.1": {
                "min": 14.519806,
                "max": 36.065579,
                "mean": 25.303323,
                "stddev": 4.515555,
            },
            "cca_output.2": {
                "min": 5.936035,
                "max": 29.925073,
                "mean": 10.712231,
                "stddev": 5.263790,
            },
            "cca_output.3": {
                "min": 64.618755,
                "max": 70.855162,
                "mean": 68.056793,
                "stddev": 1.238260,
            },
        }

        for mapname, refstats in expected_stats.items():
            self.assertRasterFitsUnivar(
                mapname,
                reference=refstats,
                precision=1e-6,
                msg=f"{mapname} stats diverge from expected output",
            )

    def test_orthogonality_of_components(self):
        """Check that canonical components are approximately uncorrelated."""
        lines = gs.read_command(
            "r.covar", map="cca_output.1,cca_output.2,cca_output.3"
        ).splitlines()[1:]
        cov_matrix = [list(map(float, l.split())) for l in lines]

        for i in range(3):
            for j in range(3):
                if i != j:
                    self.assertAlmostEqual(
                        cov_matrix[i][j],
                        0.0,
                        delta=0.5,
                        msg=f"Cov[{i + 1},{j + 1}] = {cov_matrix[i][j]:.6f} exceeds orthogonality tolerance",
                    )

    def test_covariance_matrix_symmetry(self):
        """Validate that the output covariance matrix is symmetric."""
        lines = gs.read_command(
            "r.covar", map="cca_output.1,cca_output.2,cca_output.3"
        ).splitlines()[1:]
        mat = [list(map(float, l.split())) for l in lines]
        for i in range(3):
            for j in range(3):
                self.assertAlmostEqual(
                    mat[i][j],
                    mat[j][i],
                    delta=1e-10,
                    msg=f"Covariance matrix asymmetry at ({i},{j})",
                )

    def test_components_lack_linear_dependency(self):
        """Use r.regression.line to confirm no strong linear relationships between components."""
        for i in range(1, 4):
            for j in range(1, 4):
                if i == j:
                    continue
                mapx = f"{self.output_prefix}.{i}"
                mapy = f"{self.output_prefix}.{j}"
                out = gs.parse_command(
                    "r.regression.line", mapx=mapx, mapy=mapy, flags="g"
                )
                R = float(out["R"])
                slope = float(out["b"])
                self.assertAlmostEqual(
                    R,
                    0.0,
                    delta=0.1,
                    msg=f"R({mapx},{mapy}) = {R:.4f} indicates correlation",
                )
                self.assertLess(
                    abs(slope),
                    0.5,
                    msg=f"Slope({mapx}->{mapy}) = {slope:.4f} suggests dependence",
                )


if __name__ == "__main__":
    test()
