# MODULE:    Test of grass.benchmark
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Benchmarking for GRASS GIS modules
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Tests of grass.benchmark CLI"""

from pathlib import Path
from subprocess import DEVNULL

from grass.benchmark import benchmark_resolutions, save_results_to_file
from grass.benchmark.app import main as benchmark_main
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.pygrass.modules import Module


class TestBenchmarkCLI(TestCase):
    """Tests that benchmarkin CLI works"""

    def test_plot_workflow(self):
        """Test that plot workflow runs"""
        label = "Standard output"
        repeat = 4
        json_filename = "plot_test.json"
        png_filename = "plot_test.png"
        png_filename_resolutions = "plot_test_resolutions.png"
        # The benchmark part has only Python API, not CLI.
        result = benchmark_resolutions(
            module=Module("r.univar", map="elevation", stdout_=DEVNULL, run_=False),
            label=label,
            repeat=repeat,
            resolutions=[1000, 500],
        )
        save_results_to_file([result], json_filename)
        benchmark_main(["plot", "cells", json_filename, png_filename])
        self.assertTrue(Path(png_filename).is_file())
        benchmark_main(
            ["plot", "cells", "--resolutions", json_filename, png_filename_resolutions]
        )
        self.assertTrue(Path(png_filename_resolutions).is_file())


if __name__ == "__main__":
    test()
