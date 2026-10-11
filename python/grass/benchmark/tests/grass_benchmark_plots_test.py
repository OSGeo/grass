"""Tests of the nprocs plot and its ranges in grass.benchmark"""

from types import SimpleNamespace

import pytest

from grass.benchmark import nprocs_plot
from grass.benchmark.plots import nprocs_ranges


def make_result(nprocs, all_times, label="test"):
    times = [sum(t) / len(t) for t in all_times]
    return SimpleNamespace(
        nprocs=nprocs,
        all_times=all_times,
        times=times,
        speedup=[times[0] / t for t in times],
        efficiency=[times[0] / (n * t) for n, t in zip(nprocs, times, strict=True)],
        label=label,
    )


def test_time_range_is_min_max_of_runs():
    result = make_result([1, 2], [[10.0, 12.0, 11.0], [6.0, 5.0, 7.0]])
    assert nprocs_ranges(result, "time") == ([10.0, 5.0], [12.0, 7.0])


def test_speedup_range_spans_ratios_of_serial_to_parallel_runs():
    result = make_result([1, 2], [[10.0, 12.0], [5.0, 6.0]])
    lows, highs = nprocs_ranges(result, "speedup")
    assert lows == [1, 10.0 / 6.0]
    assert highs == [1, 12.0 / 5.0]


def test_efficiency_range_is_speedup_range_per_thread():
    result = make_result([1, 4], [[8.0, 12.0], [2.0, 4.0]])
    lows, highs = nprocs_ranges(result, "efficiency")
    assert lows == [1, 8.0 / 4.0 / 4]
    assert highs == [1, 12.0 / 2.0 / 4]


def test_serial_is_smallest_nprocs():
    result = make_result([2, 4], [[6.0, 6.0], [3.0, 4.0]])
    lows, highs = nprocs_ranges(result, "speedup")
    assert lows == [1, 6.0 / 4.0]
    assert highs == [1, 6.0 / 3.0]


def test_no_range_without_all_times():
    result = SimpleNamespace(nprocs=[1, 2], times=[2.0, 1.0], label="test")
    assert nprocs_ranges(result, "time") is None


@pytest.mark.parametrize("metric", ["time", "speedup", "efficiency"])
def test_plot_file_has_requested_size(tmp_path, metric):
    pytest.importorskip("matplotlib")
    results = [make_result([1, 2, 4], [[4.0, 4.2], [2.1, 2.3], [1.2, 1.4]])]
    filename = tmp_path / f"{metric}.png"
    nprocs_plot(
        results, filename=str(filename), metric=metric, figsize=(6, 4.5), dpi=50
    )
    with open(filename, "rb") as f:
        header = f.read(24)
    # PNG width and height are big-endian integers at bytes 16 to 24.
    assert int.from_bytes(header[16:20], "big") == 300
    assert int.from_bytes(header[20:24], "big") == 225


def test_plot_without_all_times(tmp_path):
    pytest.importorskip("matplotlib")
    results = [SimpleNamespace(nprocs=[1, 2], times=[2.0, 1.0], label="test")]
    filename = tmp_path / "time.png"
    nprocs_plot(results, filename=str(filename))
    assert filename.is_file()
