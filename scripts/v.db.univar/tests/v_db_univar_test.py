"""Tests of v.db.univar output statistics"""

import json
import statistics

import pytest

try:
    import numpy as np
except ImportError:
    np = None

import grass.script as gs


def approx(x):
    """Get an approximate value to test against"""
    # Using absolute tolerance, but generally using relative too as pytest
    # does by default would be an option here as well.
    return pytest.approx(x, abs=1e-10)


def numpy_percentiles(values, percentiles, method="lower"):
    """Get list of percentiles using NumPy"""
    # Lazy-importing to make the whole NumPy is optional for this test.
    # Available since 1.9.0.
    from numpy.lib import NumpyVersion  # pylint: disable=import-outside-toplevel

    if NumpyVersion(np.__version__) < "1.22.0":
        # Deprecated since 1.22.0.
        kwargs = {"interpolation": method}
    else:
        kwargs = {"method": method}
    return np.percentile(values, percentiles, **kwargs)


def test_basic_stats(simple_dataset):
    """Test basic statistics against the Python statistics package"""
    data = json.loads(
        gs.read_command(
            "v.db.univar",
            map=simple_dataset.vector_name,
            column=simple_dataset.column_name,
            format="json",
            env=simple_dataset.session.env,
        )
    )
    assert "statistics" in data
    stats = data["statistics"]
    assert stats["n"] == len(simple_dataset.values)
    ref_min = min(simple_dataset.values)
    ref_max = max(simple_dataset.values)
    # Using approx for all computed values, but it is really needed only for
    # stddev and variance.
    assert stats["min"] == approx(ref_min)
    assert stats["max"] == approx(ref_max)
    assert stats["range"] == approx(ref_max - ref_min)
    assert stats["sum"] == approx(sum(simple_dataset.values))
    assert stats["mean"] == approx(statistics.mean(simple_dataset.values))
    assert stats["mean_abs"] == approx(
        statistics.mean(abs(i) for i in simple_dataset.values)
    )
    assert stats["stddev"] == approx(statistics.pstdev(simple_dataset.values))
    assert stats["variance"] == approx(statistics.pvariance(simple_dataset.values))


def test_extra_stats(simple_dataset):
    """Test extended statistics against the Python statistics package"""
    data = json.loads(
        gs.read_command(
            "v.db.univar",
            map=simple_dataset.vector_name,
            column=simple_dataset.column_name,
            flags="e",
            format="json",
            env=simple_dataset.session.env,
        )
    )
    stats = data["statistics"]
    # Test some of the basic stats.
    assert stats["n"] == len(simple_dataset.values)
    assert stats["min"] == min(simple_dataset.values)
    assert stats["max"] == max(simple_dataset.values)
    # Test extra stats.
    assert stats["median"] == statistics.median(simple_dataset.values)


@pytest.mark.skipif(np is None, reason="NumPy package not available")
def test_quartiles_default_percentile(simple_dataset):
    """Test extended statistics including quantiles against NumPy"""
    data = json.loads(
        gs.read_command(
            "v.db.univar",
            map=simple_dataset.vector_name,
            column=simple_dataset.column_name,
            flags="e",
            format="json",
            env=simple_dataset.session.env,
        )
    )
    assert "statistics" in data
    stats = data["statistics"]
    # Test some of the basic stats.
    assert stats["n"] == len(simple_dataset.values)
    assert stats["min"] == min(simple_dataset.values)
    assert stats["max"] == max(simple_dataset.values)
    assert stats["max"] == max(simple_dataset.values)
    # Test percentiles.
    # These work only for n=10. For other n, all methods are usually
    # off by one at least for one of the values.
    assert stats["median"] == numpy_percentiles(
        simple_dataset.values, percentiles=50, method="midpoint"
    )
    assert stats["first_quartile"] == numpy_percentiles(
        simple_dataset.values, percentiles=20, method="lower"
    )
    assert stats["third_quartile"] == numpy_percentiles(
        simple_dataset.values, percentiles=75, method="higher"
    )
    assert len(stats["percentiles"]) == 1
    assert len(stats["percentile_values"]) == 1
    precentile_90 = stats["percentile_values"][0]
    assert precentile_90 == numpy_percentiles(
        simple_dataset.values, percentiles=90, method="lower"
    )


@pytest.mark.skipif(np is None, reason="NumPy package not available")
def test_percentiles(simple_dataset):
    """Test custom percentiles against NumPy"""
    percentiles = list(range(10, 100, 10))
    data = json.loads(
        gs.read_command(
            "v.db.univar",
            map=simple_dataset.vector_name,
            column=simple_dataset.column_name,
            flags="e",
            percentile=percentiles,
            format="json",
            env=simple_dataset.session.env,
        )
    )
    stats = data["statistics"]
    # Test some of the basic stats.
    assert stats["n"] == len(simple_dataset.values)
    assert stats["min"] == min(simple_dataset.values)
    assert stats["max"] == max(simple_dataset.values)
    # Test percentiles.
    assert percentiles == stats["percentiles"]
    ref_percentiles = numpy_percentiles(simple_dataset.values, percentiles)
    assert len(percentiles) == len(ref_percentiles), "Error in the test itself"
    # Same limitation as above: Works only for numbers with n=10.
    assert list(ref_percentiles) == stats["percentile_values"]


def test_fixed_values(simple_dataset):
    """Test against hardcoded values"""
    percentiles = list(range(10, 100, 20))
    data = json.loads(
        gs.read_command(
            "v.db.univar",
            map=simple_dataset.vector_name,
            column=simple_dataset.column_name,
            flags="e",
            percentile=percentiles,
            format="json",
            env=simple_dataset.session.env,
        )
    )
    assert "statistics" in data
    stats = data["statistics"]
    assert stats["n"] == 10
    assert stats["min"] == 100.11
    assert stats["max"] == 109.11
    assert stats["range"] == 9
    assert stats["sum"] == approx(1046.1)
    assert stats["mean"] == approx(104.61)
    assert stats["mean_abs"] == approx(104.61)
    assert stats["stddev"] == approx(2.87228132326952)
    assert stats["variance"] == approx(8.25000000000291)
    assert stats["coeff_var"] == approx(0.0274570435261402)
    # Test percentiles.
    assert stats["percentiles"] == percentiles
    ref_percentiles = [100.11, 102.11, 104.11, 106.11, 108.11]
    assert len(stats["percentiles"]) == len(ref_percentiles), "Error in the test itself"
    assert stats["percentile_values"] == ref_percentiles
