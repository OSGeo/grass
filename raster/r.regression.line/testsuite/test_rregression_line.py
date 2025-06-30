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
        """Test default output format."""
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

    def test_shell_format(self):
        """Test -g flag output format."""
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


if __name__ == "__main__":
    test()
