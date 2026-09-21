"""Test r.resamp.stats serial and parallel correctness

Verifies that r.resamp.stats output matches known reference values (from
r.univar) for several aggregation methods, unweighted at res=100 and
area-weighted at res=25, that the -n flag propagates NULLs from partially
NULL source blocks, and that parallel (nprocs=4) output matches serial
(nprocs=1) output.

Adapted from r.resamp.filter test pattern using assertRasterFitsUnivar.

Uses the nc_spm_08_grass7 dataset (elevation map, native resolution 10 m).

@author Vinay Kumar Chopra
"""

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.main import test


def univar_stats(case, raster):
    """Return the r.univar -g output of a raster as a dict of raw strings."""
    module = SimpleModule("r.univar", map=raster, flags="g")
    case.runModule(module)
    return dict(
        line.split("=", 1) for line in module.outputs.stdout.strip().splitlines()
    )


class TestResampStatsReference(TestCase):
    """Test r.resamp.stats against known reference values.

    Validates both serial and threaded output match expected univar
    statistics, following the r.resamp.filter test pattern.

    Used dataset: nc_spm_08_grass7
    """

    input_map = "elevation"

    # Reference values computed with res=100 on nc_spm_08_grass7
    test_options = {
        "average": {
            "n": 20250,
            "null_cells": 0,
            "cells": 20250,
            "min": 57.1003440475464,
            "max": 155.728766784668,
            "range": 98.6284227371216,
            "mean": 110.375440275606,
            "mean_of_abs": 110.375440275606,
            "stddev": 20.2166675908506,
            "variance": 408.713648478948,
            "coeff_var": 18.3162735662661,
            "sum": 2235102.66558102,
        },
        "median": {
            "n": 20250,
            "null_cells": 0,
            "cells": 20250,
            "min": 57.0738582611084,
            "max": 155.859214782715,
            "range": 98.7853565216064,
            "mean": 110.388871867427,
            "mean_of_abs": 110.388871867427,
            "stddev": 20.2798663204998,
            "variance": 411.272977977342,
            "coeff_var": 18.3712959263278,
            "sum": 2235374.6553154,
        },
        "sum": {
            "n": 20250,
            "null_cells": 0,
            "cells": 20250,
            "min": 5710.03440475464,
            "max": 15572.8766784668,
            "range": 9862.84227371216,
            "mean": 11037.5440275606,
            "mean_of_abs": 11037.5440275606,
            "stddev": 2021.66675908506,
            "variance": 4087136.4847895,
            "coeff_var": 18.3162735662661,
            "sum": 223510266.558102,
        },
        "minimum": {
            "n": 20250,
            "null_cells": 0,
            "cells": 20250,
            "min": 55.5787925720215,
            "max": 154.124160766602,
            "range": 98.5453681945801,
            "mean": 106.847962996683,
            "mean_of_abs": 106.847962996683,
            "stddev": 20.1033220854381,
            "variance": 404.143558870865,
            "coeff_var": 18.8148856764468,
            "sum": 2163671.25068283,
        },
        "maximum": {
            "n": 20250,
            "null_cells": 0,
            "cells": 20250,
            "min": 57.4681549072266,
            "max": 156.329864501953,
            "range": 98.8617095947266,
            "mean": 113.746081302031,
            "mean_of_abs": 113.746081302031,
            "stddev": 20.1794473171495,
            "variance": 407.210094025612,
            "coeff_var": 17.7407846372895,
            "sum": 2303358.14636612,
        },
    }

    to_remove = []

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # Coarsen by 10x from the native 10m resolution to 100m
        cls.runModule("g.region", raster=cls.input_map, res=100)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove",
                flags="f",
                type="raster",
                name=",".join(cls.to_remove),
            )

    def _run_and_check(self, method, key):
        """Run r.resamp.stats serially and in parallel, check both outputs."""
        serial_out = f"test_resamp_stats_serial_{key}"
        parallel_out = f"test_resamp_stats_parallel_{key}"
        self.to_remove.extend([serial_out, parallel_out])

        # Serial run (nprocs=1)
        self.assertModule(
            "r.resamp.stats",
            input=self.input_map,
            output=serial_out,
            method=method,
            nprocs=1,
            overwrite=True,
        )

        # Parallel run (nprocs=4)
        self.assertModule(
            "r.resamp.stats",
            input=self.input_map,
            output=parallel_out,
            method=method,
            nprocs=4,
            overwrite=True,
        )

        # Check both outputs against known reference values
        self.assertRasterFitsUnivar(
            raster=serial_out,
            reference=self.test_options[key],
            precision=1e-5,
        )
        self.assertRasterFitsUnivar(
            raster=parallel_out,
            reference=self.test_options[key],
            precision=1e-5,
        )

    def test_average_unweighted(self):
        """Test unweighted average: serial and parallel match reference."""
        self._run_and_check("average", "average")

    def test_median_unweighted(self):
        """Test unweighted median: serial and parallel match reference."""
        self._run_and_check("median", "median")

    def test_sum_unweighted(self):
        """Test unweighted sum: serial and parallel match reference."""
        self._run_and_check("sum", "sum")

    def test_minimum_unweighted(self):
        """Test unweighted minimum: serial and parallel match reference."""
        self._run_and_check("minimum", "minimum")

    def test_maximum_unweighted(self):
        """Test unweighted maximum: serial and parallel match reference."""
        self._run_and_check("maximum", "maximum")


class TestResampStatsWeighted(TestCase):
    """Test area-weighted aggregation (-w) with fractional cell overlap.

    The target resolution of 25m is 2.5x the native 10m resolution of the
    input, so the source cells along the edge of a destination cell are only
    half covered and contribute with a weight of 0.5. At an integer
    coarsening ratio every weight would be 1 and -w would reduce to the
    unweighted algorithm, leaving the weighting arithmetic untested.

    Used dataset: nc_spm_08_grass7
    """

    input_map = "elevation"

    # Reference values for method=average with -w at res=25 on nc_spm_08_grass7
    reference = {
        "n": 324000,
        "null_cells": 0,
        "cells": 324000,
        "min": 56.0787606811523,
        "max": 156.254379272461,
        "range": 100.175618591309,
        "mean": 110.375440275606,
        "mean_of_abs": 110.375440275606,
        "stddev": 20.3042465367039,
        "variance": 412.262427423253,
        "coeff_var": 18.3956199730706,
        "sum": 35761642.6492963,
    }

    to_remove = []

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # 25m divides the extent of the elevation map evenly (540x600 cells),
        # so the destination grid is exact while the overlap with the 10m
        # source cells is fractional.
        cls.runModule("g.region", raster=cls.input_map, res=25)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove",
                flags="f",
                type="raster",
                name=",".join(cls.to_remove),
            )

    def test_average_weighted(self):
        """Test weighted average: serial and parallel match reference."""
        serial_out = "test_resamp_stats_weighted_serial"
        parallel_out = "test_resamp_stats_weighted_parallel"
        self.to_remove.extend([serial_out, parallel_out])

        self.assertModule(
            "r.resamp.stats",
            input=self.input_map,
            output=serial_out,
            method="average",
            flags="w",
            nprocs=1,
            overwrite=True,
        )
        self.assertModule(
            "r.resamp.stats",
            input=self.input_map,
            output=parallel_out,
            method="average",
            flags="w",
            nprocs=4,
            overwrite=True,
        )

        self.assertRasterFitsUnivar(
            raster=serial_out,
            reference=self.reference,
            precision=1e-5,
        )
        # Each output row is computed independently, so threading must not
        # change a single cell
        self.assertRastersNoDifference(
            actual=parallel_out,
            reference=serial_out,
            precision=0,
        )

    def test_weighted_differs_from_unweighted(self):
        """Test that -w changes the result when the cell overlap is partial."""
        weighted_out = "test_resamp_stats_weighted_only"
        unweighted_out = "test_resamp_stats_unweighted_only"
        difference = "test_resamp_stats_weight_difference"
        self.to_remove.extend([weighted_out, unweighted_out, difference])

        self.assertModule(
            "r.resamp.stats",
            input=self.input_map,
            output=weighted_out,
            method="average",
            flags="w",
            overwrite=True,
        )
        self.assertModule(
            "r.resamp.stats",
            input=self.input_map,
            output=unweighted_out,
            method="average",
            overwrite=True,
        )

        self.runModule(
            "r.mapcalc",
            expression=f"{difference} = abs({weighted_out} - {unweighted_out})",
            overwrite=True,
        )
        # The largest difference on this dataset is about 2.8 m; requiring
        # more than 1 m keeps the test far above floating point noise while
        # failing outright if -w ever degenerates to the unweighted result.
        self.assertGreater(
            float(univar_stats(self, difference)["max"]),
            1.0,
            msg="Weighted and unweighted average must differ at res=25",
        )


class TestResampStatsNullPropagation(TestCase):
    """Test that the -n flag correctly propagates NULLs.

    Used dataset: nc_spm_08_grass7
    """

    input_map = "elevation"
    null_input = "test_resamp_null_input"

    # The input NULL pattern below assigns each 10x10 source block that makes
    # up one res=100 destination cell to one of three classes by
    # (block_row + block_col) % 3: class 0 has no NULLs, class 1 has a single
    # NULL source row (partially NULL) and class 2 is entirely NULL. The
    # destination grid is 135x150 = 20250 cells and 135 is a multiple of 3,
    # so each class covers exactly one third of the destination cells.
    cells_per_class = 6750

    to_remove = []

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # row() and col() below are resolved in the current region, so the
        # NULL pattern has to be built at the native resolution of the input
        cls.runModule("g.region", raster=cls.input_map)
        # 0-based indices of the res=100 destination cell a source cell
        # belongs to, given that both regions share the same north and west
        # edge and 100m is exactly 10 source cells
        block_row = "int((row() - 1) / 10)"
        block_col = "int((col() - 1) / 10)"
        block_class = f"(({block_row} + {block_col}) % 3)"
        first_row_of_block = "(row() - 1) % 10 == 0"
        cls.runModule(
            "r.mapcalc",
            expression=(
                f"{cls.null_input} = if({block_class} == 2, null(), "
                f"if({block_class} == 1 && {first_row_of_block}, null(), "
                f"{cls.input_map}))"
            ),
            overwrite=True,
        )
        cls.to_remove.append(cls.null_input)
        # Now set the output region to coarse for the resampling tests
        cls.runModule("g.region", raster=cls.input_map, res=100)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove",
                flags="f",
                type="raster",
                name=",".join(cls.to_remove),
            )

    def test_null_propagation_flag(self):
        """Test that -n turns partially NULL source blocks into NULLs."""
        output_with_n = "test_resamp_null_propagate"
        output_without_n = "test_resamp_null_ignore"
        self.to_remove.extend([output_with_n, output_without_n])

        # With -n: a single NULL source cell makes the destination cell NULL
        self.assertModule(
            "r.resamp.stats",
            input=self.null_input,
            output=output_with_n,
            method="average",
            flags="n",
            nprocs=1,
            overwrite=True,
        )
        # Without -n: NULL source cells are ignored
        self.assertModule(
            "r.resamp.stats",
            input=self.null_input,
            output=output_without_n,
            method="average",
            nprocs=1,
            overwrite=True,
        )

        # Without -n only the entirely NULL class 2 blocks are NULL, with -n
        # the partially NULL class 1 blocks become NULL as well
        info_without_n = univar_stats(self, output_without_n)
        self.assertEqual(
            int(info_without_n["null_cells"]),
            self.cells_per_class,
            msg="Without -n only entirely NULL source blocks give NULL",
        )
        self.assertEqual(
            int(info_without_n["n"]),
            2 * self.cells_per_class,
            msg="Without -n partially NULL source blocks still give a value",
        )

        info_with_n = univar_stats(self, output_with_n)
        self.assertEqual(
            int(info_with_n["null_cells"]),
            2 * self.cells_per_class,
            msg="With -n partially NULL source blocks must give NULL",
        )
        self.assertEqual(
            int(info_with_n["n"]),
            self.cells_per_class,
            msg="With -n source blocks without any NULL must keep their value",
        )

    def test_null_parallel_matches_serial(self):
        """Test that -n flag output matches between serial and parallel."""
        serial_out = "test_resamp_null_serial"
        parallel_out = "test_resamp_null_parallel"
        self.to_remove.extend([serial_out, parallel_out])

        self.assertModule(
            "r.resamp.stats",
            input=self.null_input,
            output=serial_out,
            method="average",
            flags="n",
            nprocs=1,
            overwrite=True,
        )
        self.assertModule(
            "r.resamp.stats",
            input=self.null_input,
            output=parallel_out,
            method="average",
            flags="n",
            nprocs=4,
            overwrite=True,
        )

        # A difference raster is NULL wherever either input is NULL, so the
        # NULL cells have to be compared separately
        serial_info = univar_stats(self, serial_out)
        parallel_info = univar_stats(self, parallel_out)
        for key in ("n", "null_cells", "cells"):
            self.assertEqual(
                serial_info[key],
                parallel_info[key],
                msg=f"Mismatch for '{key}' between serial and parallel with -n flag",
            )
        self.assertRastersNoDifference(
            actual=parallel_out,
            reference=serial_out,
            precision=0,
        )


if __name__ == "__main__":
    test()
