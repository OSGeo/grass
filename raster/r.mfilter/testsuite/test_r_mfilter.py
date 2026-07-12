from tempfile import NamedTemporaryFile
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.utils import xfail_windows


class TestNeighbors(TestCase):
    """

    Used dataset: nc_spm_full_v2beta1

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
                "n": 1789490,
                "null_cells": 235510,
                "cells": 2025000,
                "min": 34301.08203125,
                "max": 43599.45703125,
                "range": 9298.375,
                "mean": 39040.3073035648,
                "mean_of_abs": 39040.3073035648,
                "stddev": 338.861109540213,
                "variance": 114826.851424987,
                "coeff_var": 0.867977567147046,
                "sum": 69862239516.6562,
            },
            True: {
                "n": 1789490,
                "null_cells": 235510,
                "cells": 2025000,
                "min": 34300,
                "max": 43600,
                "range": 9300,
                "mean": 39041.4984470043,
                "mean_of_abs": 39041.4984470043,
                "stddev": 348.205753496913,
                "variance": 121247.246631722,
                "coeff_var": 0.891886242454486,
                "sum": 69864371055.9297,
            },
        },
        "test_parallel_null": {
            False: {
                "n": 46381,
                "null_cells": 1978619,
                "cells": 2025000,
                "min": 34300,
                "max": 43600,
                "range": 9300,
                "mean": 38988.3010371973,
                "mean_of_abs": 38988.3010371973,
                "stddev": 792.424493234645,
                "variance": 627936.577478183,
                "coeff_var": 2.03246736111589,
                "sum": 1808316390.40625,
            },
            True: {
                "n": 46381,
                "null_cells": 1978619,
                "cells": 2025000,
                "min": 34300,
                "max": 43600,
                "range": 9300,
                "mean": 38988.2992790859,
                "mean_of_abs": 38988.2992790859,
                "stddev": 798.805978312437,
                "variance": 638090.990987689,
                "coeff_var": 2.04883514562774,
                "sum": 1808316308.86328,
            },
        },
        "test_multiple_filters": {
            "n": 2025000,
            "null_cells": 0,
            "cells": 2025000,
            "min": 55.5787925720215,
            "max": 155.957977294922,
            "range": 100.3791847229,
            "mean": 110.374591878668,
            "mean_of_abs": 110.374591878668,
            "stddev": 20.235686035401,
            "variance": 409.482989323322,
            "coeff_var": 18.3336451722925,
            "sum": 223508548.554302,
        },
        "test_repeated_filters": {
            "n": 2025000,
            "null_cells": 0,
            "cells": 2025000,
            "min": 55.5787925720215,
            "max": 155.465209960938,
            "range": 99.886417388916,
            "mean": 110.373036950725,
            "mean_of_abs": 110.373036950725,
            "stddev": 20.1413729988748,
            "variance": 405.674906279802,
            "coeff_var": 18.2484541110042,
            "sum": 223505399.825218,
        },
    }

    filter_options = {
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
        "mix": b"""MATRIX 5
                   1 1 1 1 1
                   1 1 1 1 1
                   1 1 1 1 1
                   1 1 1 1 1
                   1 1 1 1 1
                   DIVISOR 0
                   TYPE    P

                   MATRIX 5
                   1 1 1 1 1
                   1 1 1 1 1
                   1 1 1 1 1
                   1 1 1 1 1
                   1 1 1 1 1
                   DIVISOR 0
                   TYPE    S

                   MATRIX 3
                   1 1 1
                   1 1 1
                   1 1 1
                   DIVISOR 9
                   TYPE    P""",
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

    @xfail_windows
    def test_sequential(self):
        """Test output with sequential filter type."""
        test_case = "test_sequential"
        output = "{}_raster".format(test_case)
        output_threaded = "{}_threaded_raster".format(test_case)
        self.to_remove.extend([output, output_threaded])

        filter = self.create_filter(self.filter_options["sequential"])
        self.assertModule(
            "r.mfilter",
            input="elevation",
            output=output,
            filter=filter.name,
        )
        self.assertModule(
            "r.mfilter",
            input="elevation",
            output=output_threaded,
            filter=filter.name,
            nprocs=4,
        )
        filter.close()
        self.assertRasterFitsUnivar(
            raster=output,
            reference=self.test_results[test_case],
            precision=1e-5,
        )
        self.assertRasterFitsUnivar(
            raster=output_threaded,
            reference=self.test_results[test_case],
            precision=1e-5,
        )

    @xfail_windows
    def test_parallel(self):
        """Test output with parallel filter type."""
        test_case = "test_parallel"
        output = "{}_raster".format(test_case)
        output_threaded = "{}_threaded_raster".format(test_case)
        self.to_remove.extend([output, output_threaded])

        filter = self.create_filter(self.filter_options["parallel"])
        self.assertModule(
            "r.mfilter",
            input="elevation",
            output=output,
            filter=filter.name,
        )
        self.assertModule(
            "r.mfilter",
            input="elevation",
            output=output_threaded,
            filter=filter.name,
            nprocs=4,
        )
        filter.close()
        self.assertRasterFitsUnivar(
            raster=output,
            reference=self.test_results[test_case],
            precision=1e-5,
        )
        self.assertRasterFitsUnivar(
            raster=output_threaded,
            reference=self.test_results[test_case],
            precision=1e-5,
        )

    @xfail_windows
    def test_sequential_null(self):
        """Test output with sequential filter type with null mode enabled."""
        test_case = "test_sequential_null"
        output = "{}_raster".format(test_case)
        output_z = "{}_z_raster".format(test_case)
        self.to_remove.extend([output, output_z])

        filter = self.create_filter(self.filter_options["sequential"])
        self.assertModule(
            "r.mfilter",
            input="lakes",
            output=output,
            filter=filter.name,
        )
        self.assertModule(
            "r.mfilter",
            input="lakes",
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

    @xfail_windows
    def test_parallel_null(self):
        """Test output with parallel filter type with null mode enabled."""
        test_case = "test_parallel_null"
        output = "{}_raster".format(test_case)
        output_threaded = "{}_threaded_raster".format(test_case)
        output_z = "{}_z_raster".format(test_case)
        output_z_threaded = "{}_z_threaded_raster".format(test_case)
        self.to_remove.extend([output, output_threaded, output_z, output_z_threaded])

        filter = self.create_filter(self.filter_options["parallel"])
        self.assertModule(
            "r.mfilter",
            input="lakes",
            output=output,
            filter=filter.name,
        )
        self.assertModule(
            "r.mfilter",
            input="lakes",
            output=output_threaded,
            filter=filter.name,
            nprocs=4,
        )
        self.assertModule(
            "r.mfilter",
            input="lakes",
            output=output_z,
            filter=filter.name,
            flags="z",
        )
        self.assertModule(
            "r.mfilter",
            input="lakes",
            output=output_z_threaded,
            filter=filter.name,
            flags="z",
            nprocs=4,
        )
        filter.close()
        self.assertRasterFitsUnivar(
            raster=output,
            reference=self.test_results[test_case][False],
            precision=1e-5,
        )
        self.assertRasterFitsUnivar(
            raster=output_threaded,
            reference=self.test_results[test_case][False],
            precision=1e-5,
        )
        self.assertRasterFitsUnivar(
            raster=output_z,
            reference=self.test_results[test_case][True],
            precision=1e-5,
        )
        self.assertRasterFitsUnivar(
            raster=output_z_threaded,
            reference=self.test_results[test_case][True],
            precision=1e-5,
        )

    @xfail_windows
    def test_multiple_filters(self):
        """Test output with multiple filters."""
        test_case = "test_multiple_filters"
        output = "{}_raster".format(test_case)
        output_threaded = "{}_threaded_raster".format(test_case)
        self.to_remove.extend([output, output_threaded])

        filter = self.create_filter(self.filter_options["mix"])
        self.assertModule(
            "r.mfilter",
            input="elevation",
            output=output,
            filter=filter.name,
        )
        self.assertModule(
            "r.mfilter",
            input="elevation",
            output=output_threaded,
            filter=filter.name,
            nprocs=4,
        )
        filter.close()
        self.assertRasterFitsUnivar(
            raster=output,
            reference=self.test_results[test_case],
            precision=1e-5,
        )
        self.assertRasterFitsUnivar(
            raster=output_threaded,
            reference=self.test_results[test_case],
            precision=1e-5,
        )

    @xfail_windows
    def test_repeated_filters(self):
        """Test output with repeated filters."""
        test_case = "test_repeated_filters"
        output = "{}_raster".format(test_case)
        output_threaded = "{}_threaded_raster".format(test_case)
        self.to_remove.extend([output, output_threaded])

        filter = self.create_filter(self.filter_options["mix"])
        self.assertModule(
            "r.mfilter",
            input="elevation",
            output=output,
            filter=filter.name,
            repeat=3,
        )
        self.assertModule(
            "r.mfilter",
            input="elevation",
            output=output_threaded,
            filter=filter.name,
            repeat=3,
            nprocs=4,
        )
        filter.close()
        self.assertRasterFitsUnivar(
            raster=output,
            reference=self.test_results[test_case],
            precision=1e-5,
        )
        self.assertRasterFitsUnivar(
            raster=output_threaded,
            reference=self.test_results[test_case],
            precision=1e-5,
        )


if __name__ == "__main__":
    test()
