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

"""Basic tests of grass.benchmark"""

from pathlib import Path
from subprocess import DEVNULL

from grass.benchmark import benchmark_resolutions, num_cells_plot
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.pygrass.modules import Module


class TestBenchmarksRun(TestCase):
    """Tests that functions for benchmarking can run"""

    def test_resolutions(self):
        """Test that resolution tests runs without nprocs and plots to file"""
        benchmarks = [
            dict(
                module=Module("r.univar", map="elevation", stdout_=DEVNULL, run_=False),
                label="Standard output",
            ),
            dict(
                module=Module(
                    "r.univar", map="elevation", flags="g", stdout_=DEVNULL, run_=False
                ),
                label="Standard output",
            ),
        ]
        resolutions = [300, 200, 100]
        results = []
        for benchmark in benchmarks:
            results.append(
                benchmark_resolutions(
                    **benchmark,
                    resolutions=resolutions,
                )
            )
        plot_file = "test_res_plot.png"
        num_cells_plot(results, filename=plot_file)
        self.assertTrue(Path(plot_file).is_file())


if __name__ == "__main__":
    test()
