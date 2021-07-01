from tempfile import NamedTemporaryFile
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestNeighbors(TestCase):
    """

    Used dataset: nc_spm_full_v2alphav2

    Test cases:
    test_sequential: Test output with sequential filter type
    test_parallel: Test output with parallel filter type
    test_sequential_null: Test output with sequential filter type
        with null mode enabled
    test_parallel_null: Test output with parallel filter type
        with null mode enabled
    """

    test_results = {
        "test_sequential": {
            "n": 2025000,
            "null_cells": 0,
            "cells": 2025000,
            "min": 55.5787925720215,
            "max": 156.124557495117,
            "range": 100.545764923096,
            "mean": 110.374947784588,
            "mean_of_abs": 110.374947784588,
            "stddev": 20.2637480154117,
            "variance": 410.6194836321,
            "coeff_var": 18.3590102846157,
            "sum": 223509269.26379,
        },
        "test_parallel": {
            "n": 2025000,
            "null_cells": 0,
            "cells": 2025000,
            "min": 55.5787925720215,
            "max": 156.223892211914,
            "range": 100.645099639893,
            "mean": 110.375174079437,
            "mean_of_abs": 110.375174079437,
            "stddev": 20.2824544942723,
            "variance": 411.377960312227,
            "coeff_var": 18.3759207298509,
            "sum": 223509727.51086,
        },
        "test_sequential_null": {
            False: {
                "n": 2019304,
                "null_cells": 5696,
                "cells": 2025000,
                "min": 4.33287368650781e-06,
                "max": 26.9061088562012,
                "range": 26.9061045233275,
                "mean": 3.86448891335477,
                "mean_of_abs": 3.86448891335477,
                "stddev": 2.25477835586112,
                "variance": 5.08402543405979,
                "coeff_var": 58.3460945655489,
                "sum": 7803577.92069293,
            },
            True: {
                "n": 2019304,
                "null_cells": 5696,
                "cells": 2025000,
                "min": 0,
                "max": 38.6893920898438,
                "range": 38.6893920898438,
                "mean": 3.86452240667335,
                "mean_of_abs": 3.86452240667335,
                "stddev": 3.00791412221812,
                "variance": 9.04754736663921,
                "coeff_var": 77.8340453408676,
                "sum": 7803645.55388512,
            },
        },
        "test_parallel_null": {
            False: {
                "n": 2019304,
                "null_cells": 5696,
                "cells": 2025000,
                "min": 0,
                "max": 28.3634567260742,
                "range": 28.3634567260742,
                "mean": 3.86451066432895,
                "mean_of_abs": 3.86451066432895,
                "stddev": 2.35362361203859,
                "variance": 5.53954410714559,
                "coeff_var": 60.9035351814014,
                "sum": 7803621.84252211,
            },
            True: {
                "n": 2019304,
                "null_cells": 5696,
                "cells": 2025000,
                "min": 0,
                "max": 38.6893920898438,
                "range": 38.6893920898438,
                "mean": 3.86452240667335,
                "mean_of_abs": 3.86452240667335,
                "stddev": 3.00791412221812,
                "variance": 9.04754736663921,
                "coeff_var": 77.8340453408676,
                "sum": 7803645.55388512,
            },
        },
    }

    filter_options = {
        "uniform": {
            "sequential": b"""MATRIX 5
                              1 1 1 1 1
                              1 1 1 1 1
                              1 1 1 1 1
                              1 1 1 1 1
                              1 1 1 1 1
                              DIVISOR 0
                              TYPE    S""",
            "parallel": b"""MATRIX 5
                            1 1 1 1 1
                            1 1 1 1 1
                            1 1 1 1 1
                            1 1 1 1 1
                            1 1 1 1 1
                            DIVISOR 0
                            TYPE    P""",
        },
    }

    # TODO: replace by unified handing of maps
    to_remove = []

    def create_filter(self, options):
        """Create a temporary filter file with the given name and options."""
        f = NamedTemporaryFile()
        f.write(options)
        f.flush()
        return f

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove", flags="f", type="raster", name=",".join(cls.to_remove)
            )

    def test_sequential(self):
        """Test output with sequential filter type."""
        test_case = "test_sequential"
        output = "{}_raster".format(test_case)
        self.to_remove.append(output)

        filter = self.create_filter(self.filter_options["uniform"]["sequential"])
        self.assertModule(
            "r.mfilter",
            input="elevation",
            output=output,
            filter=filter.name,
        )
        filter.close()
        self.assertRasterFitsUnivar(
            raster=output,
            reference=self.test_results[test_case],
            precision=1e-5,
        )

    def test_parallel(self):
        """Test output with parallel filter type."""
        test_case = "test_parallel"
        output = "{}_raster".format(test_case)
        self.to_remove.append(output)

        filter = self.create_filter(self.filter_options["uniform"]["parallel"])
        self.assertModule(
            "r.mfilter",
            input="elevation",
            output=output,
            filter=filter.name,
        )
        filter.close()
        self.assertRasterFitsUnivar(
            raster=output,
            reference=self.test_results[test_case],
            precision=1e-5,
        )

    def test_sequential_null(self):
        """Test output with sequential filter type with null mode enabled."""
        test_case = "test_sequential_null"
        output = "{}_raster".format(test_case)
        output_z = "{}_z_raster".format(test_case)
        self.to_remove.extend([output, output_z])

        filter = self.create_filter(self.filter_options["uniform"]["sequential"])
        self.assertModule(
            "r.mfilter",
            input="slope",
            output=output,
            filter=filter.name,
        )
        self.assertModule(
            "r.mfilter",
            input="slope",
            output=output_z,
            filter=filter.name,
            flags="z",
        )
        filter.close()
        self.assertRasterFitsUnivar(
            raster=output,
            reference=self.test_results[test_case][False],
            precision=1e-5,
        )
        self.assertRasterFitsUnivar(
            raster=output_z,
            reference=self.test_results[test_case][True],
            precision=1e-5,
        )

    def test_parallel_null(self):
        """Test output with parallel filter type with null mode enabled."""
        test_case = "test_parallel_null"
        output = "{}_raster".format(test_case)
        output_z = "{}_z_raster".format(test_case)
        self.to_remove.extend([output, output_z])

        filter = self.create_filter(self.filter_options["uniform"]["parallel"])
        self.assertModule(
            "r.mfilter",
            input="slope",
            output=output,
            filter=filter.name,
        )
        self.assertModule(
            "r.mfilter",
            input="slope",
            output=output_z,
            filter=filter.name,
            flags="z",
        )
        filter.close()
        self.assertRasterFitsUnivar(
            raster=output,
            reference=self.test_results[test_case][False],
            precision=1e-5,
        )
        self.assertRasterFitsUnivar(
            raster=output_z,
            reference=self.test_results[test_case][True],
            precision=1e-5,
        )


if __name__ == "__main__":
    test()
