"""Reference-value test for r.param.scale.

For every method and window size, the module is run on a generated
deterministic DEM and its univariate statistics are compared against
reference values captured from the current (main) state. This verifies
that changes such as the OpenMP parallelization do not change the output.

Plain calls only (no nprocs, no memory), so the test runs identically on
main, where those options do not exist, and on the parallel branch.

Reference provenance: the values below were generated with the main-branch
r.param.scale binary at OMP_NUM_THREADS=1 (single threaded, so the internal
LU pivot search is deterministic) on the DEM defined in conftest.py.
"""

import pytest

from grass.tools import Tools

REFERENCE = {
    "elev_s3": {
        "n": 2304,
        "null_cells": 196,
        "min": 101.80008,
        "max": 145.27649,
        "mean": 122.97475500000002,
        "stddev": 10.284051528573706,
        "sum": 283333.83552,
    },
    "elev_s9": {
        "n": 1764,
        "null_cells": 736,
        "min": 104.50125,
        "max": 142.37336,
        "mean": 123.00783000000001,
        "stddev": 9.016737427884317,
        "sum": 216985.81212000002,
    },
    "slope_s3": {
        "n": 2304,
        "null_cells": 196,
        "min": 26.11279735339223,
        "max": 43.40733272035834,
        "mean": 36.38803693895144,
        "stddev": 4.1364008974936635,
        "sum": 83838.0371073441,
    },
    "slope_s9": {
        "n": 1764,
        "null_cells": 736,
        "min": 27.994624771098945,
        "max": 42.541245859220716,
        "mean": 36.47542837888093,
        "stddev": 3.5574709803915248,
        "sum": 64342.655660345954,
    },
    "aspect_s3": {
        "n": 2304,
        "null_cells": 196,
        "min": 42.74594547869662,
        "max": 83.95719694303821,
        "mean": 71.76757612855063,
        "stddev": 9.853640709422669,
        "sum": 165352.49540018066,
    },
    "aspect_s9": {
        "n": 1764,
        "null_cells": 736,
        "min": 48.52233847621093,
        "max": 83.54291301047196,
        "mean": 72.4193735904618,
        "stddev": 8.55015291146811,
        "sum": 127747.77501357462,
    },
    "profc_s3": {
        "n": 2304,
        "null_cells": 196,
        "min": -0.010414957049241315,
        "max": 0.0015296006645766295,
        "mean": -0.0032599386117310356,
        "stddev": 0.0027610770902887294,
        "sum": -7.510898561428306,
    },
    "profc_s9": {
        "n": 1764,
        "null_cells": 736,
        "min": -0.009295406153998707,
        "max": 0.0009198931819685501,
        "mean": -0.0033508540086592074,
        "stddev": 0.0023675176893880913,
        "sum": -5.910906471274842,
    },
    "planc_s3": {
        "n": 2304,
        "null_cells": 196,
        "min": -0.01494920850917067,
        "max": -0.0007867391258994048,
        "mean": -0.009589053773949783,
        "stddev": 0.0041098319545166025,
        "sum": -22.0931798951803,
    },
    "planc_s9": {
        "n": 1764,
        "null_cells": 736,
        "min": -0.014618772606680588,
        "max": -0.002106326029862539,
        "mean": -0.0100430326076053,
        "stddev": 0.0036627207113858933,
        "sum": -17.71590951981575,
    },
    "minic_s3": {
        "n": 2304,
        "null_cells": 196,
        "min": -0.014550896360978123,
        "max": -0.00042706790379223,
        "mean": -0.0075240306544993845,
        "stddev": 0.004095598706703214,
        "sum": -17.335366627966582,
    },
    "minic_s9": {
        "n": 1764,
        "null_cells": 736,
        "min": -0.013649114121733032,
        "max": -0.0013283633817371921,
        "mean": -0.007511322429245234,
        "stddev": 0.0035919637534828856,
        "sum": -13.249972765188593,
    },
    "feature_s3": {
        "n": 2304,
        "null_cells": 196,
        "min": 5.0,
        "max": 5.0,
        "mean": 5.0,
        "stddev": 0.0,
        "sum": 11520.0,
    },
    "feature_s9": {
        "n": 1764,
        "null_cells": 736,
        "min": 5.0,
        "max": 5.0,
        "mean": 5.0,
        "stddev": 0.0,
        "sum": 8820.0,
    },
}

METHODS = [
    "elev",
    "slope",
    "aspect",
    "profc",
    "planc",
    "minic",
    "feature",
]
SIZES = [3, 9]


def _key(method, size):
    return f"{method}_s{size}"


def _combos():
    for method in METHODS:
        for size in SIZES:
            yield method, size


@pytest.mark.parametrize(("method", "size"), list(_combos()))
def test_param_scale_matches_reference(param_scale_session, method, size):
    """r.param.scale stats must match the main reference values."""
    key = _key(method, size)
    tools = Tools(session=param_scale_session)
    output = f"out_{key}"

    kwargs = {
        "input": "dem",
        "output": output,
        "method": method,
        "size": size,
        "overwrite": True,
    }
    tools.r_param_scale(**kwargs)

    stats = tools.r_univar(map=output, format="json")
    reference = REFERENCE[key]

    # Cell counts and null pattern: exact integer equality.
    assert stats["n"] == reference["n"]
    assert stats["null_cells"] == reference["null_cells"]

    # Value statistics: relative tolerance, with an absolute floor that
    # covers near-zero values and a zero standard deviation.
    for field in ("min", "max", "mean", "stddev", "sum"):
        assert stats[field] == pytest.approx(reference[field], rel=1e-6, abs=1e-9)
