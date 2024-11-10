from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRDistance(TestCase):

    @classmethod
    def setUpClass(cls):
        """Set up a temporary region based on roadsmajor and lakes."""
        cls.use_temp_region()
        cls.runModule("g.region", n=223000, s=220000, w=640000, e=643000, nsres=100)

    @classmethod
    def tearDownClass(cls):
        """Clean up after tests."""
        cls.del_temp_region()

    def test_distance(self):
        module = SimpleModule("r.distance", map=("roadsmajor", "lakes"))
        self.assertModule(module)

        # Capture the output from r.distance
        result = module.outputs.stdout.strip().split("\n")

        # Define the expected results (all lines)
        expected_results = ["1:39000:500:642750:222250:642750:222750"]

        # Compare each line of actual result with expected results
        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_distance_o_flag(self):
        module = SimpleModule("r.distance", map=("lakes", "lakes"), flags="o")
        self.assertModule(module)

        result = module.outputs.stdout.strip().split("\n")

        expected_results = ["39000:39000:0:641750:222950:641750:222950"]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_distance_n_flag(self):
        module = SimpleModule("r.distance", map=("lakes", "roadsmajor"), flags="n")
        self.assertModule(module)

        result = module.outputs.stdout.strip().split("\n")

        expected_results = [
            "*:*:0:640250:222950:640250:222950",
            "*:1:0:642750:222250:642750:222250",
            "39000:*:0:642750:222850:642750:222850",
            "39000:1:500:642750:222750:642750:222250",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )


if __name__ == "__main__":
    test()
