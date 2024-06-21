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
from types import SimpleNamespace

from grass.benchmark import (
    benchmark_resolutions,
    benchmark_nprocs,
    benchmark_single,
    join_results,
    load_results,
    load_results_from_file,
    num_cells_plot,
    save_results,
    save_results_to_file,
)
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

    def test_single(self):
        """Test that single benchmark function runs"""
        label = "Standard output"
        repeat = 4
        benchmarks = [
            dict(
                module=Module("r.univar", map="elevation", stdout_=DEVNULL, run_=False),
                label=label,
            )
        ]
        results = []
        for benchmark in benchmarks:
            results.append(benchmark_single(**benchmark, repeat=repeat))
        self.assertEqual(len(results), len(benchmarks))
        for result in results:
            self.assertTrue(hasattr(result, "all_times"))
            self.assertTrue(hasattr(result, "time"))
            self.assertTrue(hasattr(result, "label"))
            self.assertEqual(len(result.all_times), repeat)
        self.assertEqual(results[0].label, label)

    def test_nprocs(self):
        """Test that benchmark function runs for nprocs"""
        label = "Standard output"
        repeat = 4
        benchmarks = [
            dict(
                module=Module("r.univar", map="elevation", stdout_=DEVNULL, run_=False),
                label=label,
                max_nprocs=4,
            )
        ]
        results = []
        for benchmark in benchmarks:
            results.append(benchmark_nprocs(**benchmark, repeat=repeat, shuffle=True))
        self.assertEqual(len(results), len(benchmarks))
        for result in results:
            self.assertTrue(hasattr(result, "times"))
            self.assertTrue(hasattr(result, "all_times"))
            self.assertTrue(hasattr(result, "speedup"))
            self.assertTrue(hasattr(result, "efficiency"))
            self.assertTrue(hasattr(result, "label"))
            self.assertEqual(len(result.all_times), repeat)
        self.assertEqual(results[0].label, label)


class TestBenchmarkResults(TestCase):
    """Tests that saving results work"""

    def test_save_load(self):
        """Test that results can be saved and loaded"""
        resolutions = [300, 200]
        results = [
            benchmark_resolutions(
                module=Module(
                    "r.univar",
                    map="elevation",
                    stdout_=DEVNULL,
                    stderr_=DEVNULL,
                    run_=False,
                ),
                label="Standard output",
                resolutions=resolutions,
            )
        ]
        results = load_results(save_results(results))
        plot_file = "test_res_plot.png"
        num_cells_plot(results.results, filename=plot_file)
        self.assertTrue(Path(plot_file).is_file())

    def test_data_file_roundtrip(self):
        """Test functions can save and load to a file"""
        original = [SimpleNamespace(nprocs=[1, 2, 3], times=[3, 2, 1], label="Test 1")]
        filename = "test_res_file.json"

        save_results_to_file(original, filename)
        self.assertTrue(Path(filename).is_file())

        loaded = load_results_from_file(filename).results
        self.assertEqual(original, loaded)

    def test_join_results_list(self):
        """Test that we can join lists"""
        list_1 = [
            SimpleNamespace(nprocs=[1, 2, 3], times=[3, 2, 1], label="Test 1"),
            SimpleNamespace(nprocs=[1, 2, 3], times=[3, 2, 1], label="Test 2"),
        ]
        list_2 = [SimpleNamespace(nprocs=[1, 2, 3], times=[3, 2, 1], label="Test 3")]
        new_results = join_results([list_1, list_2])
        self.assertEqual(len(new_results), 3)

    def test_join_results_structure(self):
        """Test that we can join a full results structure"""
        list_1 = SimpleNamespace(
            results=[
                SimpleNamespace(nprocs=[1, 2, 3], times=[3, 2, 1], label="Test 1"),
                SimpleNamespace(nprocs=[1, 2, 3], times=[3, 2, 1], label="Test 2"),
            ]
        )
        list_2 = [SimpleNamespace(nprocs=[1, 2, 3], times=[3, 2, 1], label="Test 3")]
        new_results = join_results([list_1, list_2])
        self.assertEqual(len(new_results), 3)


if __name__ == "__main__":
    test()
