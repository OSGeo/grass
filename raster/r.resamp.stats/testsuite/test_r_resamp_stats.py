"""Test r.resamp.stats serial and parallel correctness

Verifies that r.resamp.stats output matches known reference values (from
r.univar) for several aggregation methods, both unweighted and weighted,
and that parallel (nprocs=4) output matches serial (nprocs=1) output.

Adapted from r.resamp.filter test pattern using assertRasterFitsUnivar.

Uses the nc_spm_08_grass7 dataset (elevation map at res=100).

@author Vinay Chopra
"""

import math

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.main import test


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
        "average_w": {
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
        "quantile_w": {
            "n": 20250,
            "null_cells": 0,
            "cells": 20250,
            "min": 57.2978134155273,
            "max": 156.273727416992,
            "range": 98.9759140014648,
            "mean": 113.079299972157,
            "mean_of_abs": 113.079299972157,
            "stddev": 20.2227222247457,
            "variance": 408.958494179225,
            "coeff_var": 17.883664145184,
            "sum": 2289855.82443619,
        },
    }

    to_remove = []

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # Coarsen by ~10x from the native 10m resolution to 100m
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

    def _run_and_check(self, method, key, weighted=False, quantile=None):
        """Run r.resamp.stats serially and in parallel, check both outputs."""
        serial_out = f"test_resamp_stats_serial_{key}"
        parallel_out = f"test_resamp_stats_parallel_{key}"
        self.to_remove.extend([serial_out, parallel_out])

        flags = "w" if weighted else ""
        kwargs = {"quantile": quantile} if quantile is not None else {}

        # Serial run (nprocs=1)
        self.assertModule(
            "r.resamp.stats",
            input=self.input_map,
            output=serial_out,
            method=method,
            flags=flags,
            nprocs=1,
            overwrite=True,
            **kwargs,
        )

        # Parallel run (nprocs=4)
        self.assertModule(
            "r.resamp.stats",
            input=self.input_map,
            output=parallel_out,
            method=method,
            flags=flags,
            nprocs=4,
            overwrite=True,
            **kwargs,
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

    def test_average_weighted(self):
        """Test weighted average: serial and parallel match reference."""
        self._run_and_check("average", "average_w", weighted=True)

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

    def test_quantile_weighted(self):
        """Test weighted quantile=0.95: serial and parallel match reference."""
        self._run_and_check("quantile", "quantile_w", weighted=True, quantile=0.95)


class TestResampStatsNullPropagation(TestCase):
    """Test that the -n flag correctly propagates NULLs.

    Used dataset: nc_spm_08_grass7
    """

    input_map = "elevation"
    null_input = "test_resamp_null_input"
    to_remove = []

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # Use native resolution for null input creation (sparse NULLs)
        cls.runModule("g.region", raster=cls.input_map)
        # Created elevation with scattered NULLs (every 3rd cell in a
        # checkerboard pattern). This ensures that when resampled to
        # res=100, some destination cells have a MIX of NULL/non-NULL
        # sources, so -n flag produces more NULLs than default.
        cls.runModule(
            "r.mapcalc",
            expression=(
                f"{cls.null_input} = if((row() + col()) % 3 == 0, null(), elevation)"
            ),
            overwrite=True,
        )
        cls.to_remove.append(cls.null_input)
        # Now setting the output region to coarse for resampling tests
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
        """Test that -n flag produces NULLs where input has NULLs."""
        output_with_n = "test_resamp_null_propagate"
        output_without_n = "test_resamp_null_ignore"
        self.to_remove.extend([output_with_n, output_without_n])

        # With -n: NULL cells should propagate
        self.assertModule(
            "r.resamp.stats",
            input=self.null_input,
            output=output_with_n,
            method="average",
            flags="n",
            nprocs=1,
            overwrite=True,
        )
        # Without -n: NULL cells should be ignored
        self.assertModule(
            "r.resamp.stats",
            input=self.null_input,
            output=output_without_n,
            method="average",
            nprocs=1,
            overwrite=True,
        )

        # With -n should have more null_cells than without
        univar_with_n = SimpleModule("r.univar", map=output_with_n, flags="g")
        self.runModule(univar_with_n)
        info_with_n = dict(
            line.split("=", 1)
            for line in univar_with_n.outputs.stdout.strip().splitlines()
        )

        univar_without_n = SimpleModule("r.univar", map=output_without_n, flags="g")
        self.runModule(univar_without_n)
        info_without_n = dict(
            line.split("=", 1)
            for line in univar_without_n.outputs.stdout.strip().splitlines()
        )

        self.assertGreater(
            int(info_with_n["null_cells"]),
            int(info_without_n["null_cells"]),
            msg="With -n flag, null_cells should be greater",
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

        # Check serial and parallel produce same stats
        univar_serial = SimpleModule("r.univar", map=serial_out, flags="g")
        self.runModule(univar_serial)
        serial_info = dict(
            line.split("=", 1)
            for line in univar_serial.outputs.stdout.strip().splitlines()
        )

        univar_parallel = SimpleModule("r.univar", map=parallel_out, flags="g")
        self.runModule(univar_parallel)
        parallel_info = dict(
            line.split("=", 1)
            for line in univar_parallel.outputs.stdout.strip().splitlines()
        )

        for key in ("n", "null_cells", "cells"):
            self.assertEqual(
                serial_info[key],
                parallel_info[key],
                msg=f"Mismatch for '{key}' between serial and parallel with -n flag",
            )
        # For numeric stats, handle potential NaN values
        for key in ("min", "max", "mean", "sum"):
            s_val = float(serial_info.get(key, "0"))
            p_val = float(parallel_info.get(key, "0"))
            if math.isnan(s_val) and math.isnan(p_val):
                continue  # both NaN — OK
            self.assertAlmostEqual(
                s_val,
                p_val,
                places=6,
                msg=f"Mismatch for '{key}' between serial and parallel with -n flag",
            )


if __name__ == "__main__":
    test()
