import json

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRRegressionLine(TestCase):
    """Unit tests for r.regression.line module."""

    @classmethod
    def setUpClass(cls):
        """Set up a temporary region."""
        cls.use_temp_region()
        cls.runModule("g.region", raster="elev_srtm_30m")

    @classmethod
    def tearDownClass(cls):
        """Clean up the temporary region."""
        cls.del_temp_region()

    def test_default_format(self):
        """Test default and plain output formats."""
        module = SimpleModule(
            "r.regression.line", mapx="elev_ned_30m", mapy="elev_srtm_30m"
        )
        self.assertModule(module)

        expected = [
            "y = a + b*x",
            "   a (Offset): -1.659279",
            "   b (Gain): 1.043968",
            "   R (sumXY - sumX*sumY/N): 0.894038",
            "   N (Number of elements): 225000",
            "   F (F-test significance): 896093.366283",
            "   meanX (Mean of map1): 110.307571",
            "   sdX (Standard deviation of map1): 20.311998",
            "   meanY (Mean of map2): 113.498292",
            "   sdY (Standard deviation of map2): 23.718307",
        ]
        self.assertEqual(module.outputs.stdout.splitlines(), expected)

        # Test explicit plain format
        module = SimpleModule(
            "r.regression.line",
            mapx="elev_ned_30m",
            mapy="elev_srtm_30m",
            format="plain",
        )
        self.assertModule(module)
        self.assertEqual(module.outputs.stdout.splitlines(), expected)

    def test_shell_format(self):
        """Test shell and -g flag output formats."""
        module = SimpleModule(
            "r.regression.line", mapx="elev_ned_30m", mapy="elev_srtm_30m", flags="g"
        )
        self.assertModule(module)

        expected = [
            "a=-1.659279",
            "b=1.043968",
            "R=0.894038",
            "N=225000",
            "F=896093.366283",
            "meanX=110.307571",
            "sdX=20.311998",
            "meanY=113.498292",
            "sdY=23.718307",
        ]
        self.assertEqual(module.outputs.stdout.splitlines(), expected)

        # Using format=shell
        module = SimpleModule(
            "r.regression.line",
            mapx="elev_ned_30m",
            mapy="elev_srtm_30m",
            format="shell",
        )
        self.assertModule(module)
        self.assertEqual(module.outputs.stdout.splitlines(), expected)

    def test_json_format(self):
        """Test JSON output format."""
        module = SimpleModule(
            "r.regression.line",
            mapx="elev_ned_30m",
            mapy="elev_srtm_30m",
            format="json",
        )
        self.assertModule(module)

        expected = {
            "a": -1.6592786233805945,
            "b": 1.0439679629649166,
            "R": 0.8940383063008781,
            "N": 225000,
            "F": 896093.366283,
            "x_mean": 110.30757108713786,
            "x_stddev": 20.311997672696272,
            "y_mean": 113.49829166406644,
            "y_stddev": 23.718306793642626,
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
