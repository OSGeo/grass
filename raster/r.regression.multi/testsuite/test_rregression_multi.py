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
        """Test -g flag and default output formats."""
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


if __name__ == "__main__":
    test()
