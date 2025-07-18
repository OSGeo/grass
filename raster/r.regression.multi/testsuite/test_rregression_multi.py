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

    def assert_json_equal(self, expected, actual):
        if isinstance(expected, dict):
            self.assertIsInstance(actual, dict)
            self.assertCountEqual(expected.keys(), actual.keys())
            for key in expected:
                self.assert_json_equal(expected[key], actual[key])
        elif isinstance(expected, list):
            self.assertIsInstance(actual, list)
            self.assertEqual(len(expected), len(actual))
            for exp_item, act_item in zip(expected, actual):
                self.assert_json_equal(exp_item, act_item)
        elif isinstance(expected, float):
            self.assertAlmostEqual(expected, actual, places=6)
        else:
            self.assertEqual(expected, actual)

    def test_json_format_multiple_predictors(self):
        """Test JSON output format with multiple predictors."""
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
            "MAE": 0.020049106630102911,
            "F": 22834.74957652947,
            "b0": 0.050200758346110277,
            "AIC": -3800909.1917981608,
            "AICc": -3800909.1917981608,
            "BIC": -3800864.5071839946,
            "predictors": [
                {
                    "name": "elevation",
                    "b": 0.001523404412774435,
                    "F": 64152.65595722587,
                    "AIC": -3740385.0467569917,
                    "AICc": -3740385.0467569917,
                    "BIC": -3740364.7044499083,
                },
                {
                    "name": "aspect",
                    "b": 2.2269066721063329e-05,
                    "F": 4922.4809083809596,
                    "AIC": -3796011.6074610753,
                    "AICc": -3796011.6074610753,
                    "BIC": -3795991.265153992,
                },
                {
                    "name": "slope",
                    "b": 0.0028529315695204355,
                    "F": 11927.479105518147,
                    "AIC": -3789117.0962874037,
                    "AICc": -3789117.0962874037,
                    "BIC": -3789096.7539803209,
                },
            ],
        }
        output_json = json.loads(module.outputs.stdout)
        self.assert_json_equal(expected, output_json)

    def test_json_format_single_predictors(self):
        """Test JSON output format with single predictor."""
        module = SimpleModule(
            "r.regression.multi",
            mapx="elevation",
            mapy="soils_Kfactor",
            format="json",
        )
        self.assertModule(module)

        expected = {
            "n": 525000,
            "Rsq": 0.090046605950474312,
            "Rsqadj": 0.090044872699311274,
            "RMSE": 0.027166285225251077,
            "MAE": 0.020217775695232382,
            "F": 51952.427827544445,
            "b0": 0.091864979493248497,
            "AIC": -3786063.5177048221,
            "AICc": -3786063.5177048221,
            "BIC": -3786041.1753977388,
            "predictors": [
                {
                    "name": "elevation",
                    "b": 0.0012947818375504828,
                    "F": None,
                    "AIC": None,
                    "AICc": None,
                    "BIC": None,
                }
            ],
        }
        output_json = json.loads(module.outputs.stdout)
        self.assert_json_equal(expected, output_json)


if __name__ == "__main__":
    test()
