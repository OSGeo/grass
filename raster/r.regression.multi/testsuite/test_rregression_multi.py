import json

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRRegressionMulti(TestCase):
    """Unit tests for r.regression.multi module."""

    @classmethod
    def setUpClass(cls):
        """Set up a temporary region."""
        cls.use_temp_region()
        cls.runModule("g.region", raster="soils_Kfactor")

    @classmethod
    def tearDownClass(cls):
        """Clean up the temporary region."""
        cls.del_temp_region()

    def test_shell_format(self):
        """Test shell, -g flag and default output formats."""
        expected = [
            "n=525000",
            "Rsq=0.115424",
            "Rsqadj=0.115419",
            "RMSE=0.026785",
            "MAE=0.020049",
            "F=22834.749577",
            "b0=0.050201",
            "AIC=-3800909.191798",
            "AICc=-3800909.191798",
            "BIC=-3800864.507184",
            "",
            "predictor1=elevation",
            "b1=0.001523",
            "F1=64152.655957",
            "AIC1=-3740385.046757",
            "AICc1=-3740385.046757",
            "BIC1=-3740364.704450",
            "",
            "predictor2=aspect",
            "b2=0.000022",
            "F2=4922.480908",
            "AIC2=-3796011.607461",
            "AICc2=-3796011.607461",
            "BIC2=-3795991.265154",
            "",
            "predictor3=slope",
            "b3=0.002853",
            "F3=11927.479106",
            "AIC3=-3789117.096287",
            "AICc3=-3789117.096287",
            "BIC3=-3789096.753980",
        ]

        # Using -g flag
        module = SimpleModule(
            "r.regression.multi",
            mapx=["elevation", "aspect", "slope"],
            mapy="soils_Kfactor",
            flags="g",
        )
        self.assertModule(module)
        self.assertEqual(module.outputs.stdout.splitlines(), expected)

        # With default output
        module = SimpleModule(
            "r.regression.multi",
            mapx=["elevation", "aspect", "slope"],
            mapy="soils_Kfactor",
        )
        self.assertModule(module)
        self.assertEqual(module.outputs.stdout.splitlines(), expected)

        # Using format=shell
        module = SimpleModule(
            "r.regression.multi",
            mapx=["elevation", "aspect", "slope"],
            mapy="soils_Kfactor",
            format="shell",
        )
        self.assertModule(module)
        self.assertEqual(module.outputs.stdout.splitlines(), expected)

    def test_json_format(self):
        """Test JSON output format."""
        module = SimpleModule(
            "r.regression.multi",
            mapx=["elevation", "aspect", "slope"],
            mapy="soils_Kfactor",
            format="json",
        )
        self.assertModule(module)

        expected = {
            "n": 525000,
            "Rsq": 0.11542412808794023,
            "Rsqadj": 0.11541907333015966,
            "RMSE": 0.026784788790483356,
            "MAE": 0.02004910663010291,
            "F": 22834.74957652947,
            "b0": 0.05020075834611028,
            "AIC": -3800909.191798161,
            "AICc": -3800909.191798161,
            "BIC": -3800864.5071839946,
            "predictor1": "elevation",
            "b1": 0.001523404412774435,
            "F1": 64152.65595722587,
            "AIC1": -3740385.0467569917,
            "AICc1": -3740385.0467569917,
            "BIC1": -3740364.7044499083,
            "predictor2": "aspect",
            "b2": 2.226906672106333e-05,
            "F2": 4922.48090838096,
            "AIC2": -3796011.6074610753,
            "AICc2": -3796011.6074610753,
            "BIC2": -3795991.265153992,
            "predictor3": "slope",
            "b3": 0.0028529315695204355,
            "F3": 11927.479105518147,
            "AIC3": -3789117.0962874037,
            "AICc3": -3789117.0962874037,
            "BIC3": -3789096.753980321,
        }
        output_json = json.loads(module.outputs.stdout)

        self.assertCountEqual(list(expected.keys()), list(output_json.keys()))
        for key, value in expected.items():
            if isinstance(value, float):
                self.assertAlmostEqual(value, output_json[key], places=6)
            else:
                self.assertEqual(value, output_json[key])


if __name__ == "__main__":
    test()
