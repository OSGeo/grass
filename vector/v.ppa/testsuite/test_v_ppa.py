"""
Name:       v.ppa test
Purpose:    Tests v.ppa functions.
Author:     Corey T. White
Copyright:  (C) 2024 by Corey White and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase


class TestPointPatternAnalysis(TestCase):
    input = "crime"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule(
            "v.import",
            input="data/raleigh_crime_2022_2024.fgb",
            output=cls.input,
            overwrite=True,
        )
        cls.runModule("g.region", vector=cls.input, res=30)

    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.remove", type="vector", flags="f", name=cls.input)
        cls.del_temp_region()

    def test_g_function_csv(self):
        """Testing g function csv output"""
        self.assertModule(
            "v.ppa",
            input=self.input,
            output=f"outputs/g_{self.input}.csv",
            method="g",
            format="plain",
            seed=1,
            overwrite=True,
        )

    def test_g_function_json(self):
        """Testing g function json output"""
        self.assertModule(
            "v.ppa",
            input=self.input,
            output=f"outputs/g_{self.input}.json",
            method="g",
            format="json",
            seed=1,
            overwrite=True,
        )

    def test_f_function_plain(self):
        """Testing f function csv output"""
        self.assertModule(
            "v.ppa",
            input=self.input,
            output=f"outputs/f_{self.input}.csv",
            method="f",
            format="plain",
            seed=1,
            overwrite=True,
        )

    def test_f_function_json(self):
        """Testing f function json output"""
        self.assertModule(
            "v.ppa",
            input=self.input,
            output=f"outputs/f_{self.input}.json",
            method="f",
            format="json",
            seed=1,
            overwrite=True,
        )

    def test_f_function_num_distances(self):
        """Testing f function num_distance parameter"""
        self.assertModule(
            "v.ppa",
            input=self.input,
            output=f"outputs/g_{self.input}.json",
            method="f",
            format="json",
            num_distances=400,
            seed=1,
            overwrite=True,
        )

    def test_f_function_random_points(self):
        """Testing f function random points parameter"""
        self.assertModule(
            "v.ppa",
            input=self.input,
            output=f"outputs/g_{self.input}.json",
            method="f",
            format="json",
            random_points=2000,
            seed=1,
            overwrite=True,
        )

    def test_k_function_plain(self):
        """Testing k function csv output"""
        self.assertModule(
            "v.ppa",
            input=self.input,
            output=f"outputs/k_{self.input}.csv",
            method="k",
            format="plain",
            seed=1,
            overwrite=True,
        )

    def test_k_function_json(self):
        """Testing k function json output"""
        self.assertModule(
            "v.ppa",
            input=self.input,
            # output=f"outputs/k_{self.input}.json",
            method="k",
            format="json",
            seed=1,
            overwrite=True,
        )

    def test_k_function_save_json_file(self):
        """Testing k function json output"""
        self.assertModule(
            "v.ppa",
            input=self.input,
            output=f"outputs/k_{self.input}.json",
            method="k",
            format="json",
            seed=1,
            overwrite=True,
        )

    def test_l_function_plain(self):
        """Testing l function csv output"""
        self.assertModule(
            "v.ppa",
            input=self.input,
            output=f"outputs/l_{self.input}.csv",
            method="l",
            random_points=100,
            format="plain",
            seed=1,
            overwrite=True,
        )

    def test_l_function_json(self):
        """Testing l function json output"""
        self.assertModule(
            "v.ppa",
            input=self.input,
            output=f"outputs/l_{self.input}.json",
            method="l",
            random_points=100,
            format="json",
            seed=1,
            overwrite=True,
        )

    def test_simulation(self):
        pass

    def test_monte_carlo_envelope(self):
        pass

    def test_generate_random_points(self):
        pass

    def test_euclidean_distance(self):
        pass

    def test_max_distance(self):
        pass


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
