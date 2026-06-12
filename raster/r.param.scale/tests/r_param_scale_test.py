"""Reference-value test for r.param.scale.

For every method, window size and the -c (constrained) flag, the module is
run on a generated deterministic DEM and its univariate statistics are
compared against reference values captured from the current (main) state.
This verifies that changes such as the OpenMP parallelization do not change
the output.

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
        "n": 9604,
        "null_cells": 396,
        "min": 101.80008,
        "max": 198.80299000000002,
        "mean": 146.74988000000002,
        "stddev": 22.905529009686152,
        "sum": 1409385.84752,
    },
    "elev_s3_c": {
        "n": 9604,
        "null_cells": 396,
        "min": 101.80008,
        "max": 198.80299000000002,
        "mean": 146.74988000000002,
        "stddev": 22.905529009686152,
        "sum": 1409385.84752,
    },
    "elev_s9": {
        "n": 8464,
        "null_cells": 1536,
        "min": 104.50125,
        "max": 195.24736000000001,
        "mean": 146.74845499999998,
        "stddev": 21.640094414951772,
        "sum": 1242078.92312,
    },
    "elev_s9_c": {
        "n": 8464,
        "null_cells": 1536,
        "min": 104.50125,
        "max": 195.24736000000001,
        "mean": 146.74845499999998,
        "stddev": 21.640094414951772,
        "sum": 1242078.92312,
    },
    "slope_s3": {
        "n": 9604,
        "null_cells": 396,
        "min": 26.11279735339223,
        "max": 47.19549760541889,
        "mean": 38.077540811229795,
        "stddev": 4.257376390450918,
        "sum": 365696.70195105096,
    },
    "slope_s3_c": {
        "n": 9604,
        "null_cells": 396,
        "min": 26.11279735339223,
        "max": 47.19549760541889,
        "mean": 38.077540811229795,
        "stddev": 4.257376390450918,
        "sum": 365696.70195105096,
    },
    "slope_s9": {
        "n": 8464,
        "null_cells": 1536,
        "min": 27.994624771098945,
        "max": 46.39341708634131,
        "mean": 38.30545911342715,
        "stddev": 3.7383495956974326,
        "sum": 324217.4059360474,
    },
    "slope_s9_c": {
        "n": 8464,
        "null_cells": 1536,
        "min": 27.994624771098945,
        "max": 46.39341708634131,
        "mean": 38.30545911342715,
        "stddev": 3.7383495956974326,
        "sum": 324217.4059360474,
    },
    "aspect_s3": {
        "n": 9604,
        "null_cells": 396,
        "min": 34.26483920319282,
        "max": 83.9943056740639,
        "mean": 69.89921627090044,
        "stddev": 9.80054233689577,
        "sum": 671312.0730657278,
    },
    "aspect_s3_c": {
        "n": 9604,
        "null_cells": 396,
        "min": 34.26483920319282,
        "max": 83.9943056740639,
        "mean": 69.89921627090044,
        "stddev": 9.80054233689577,
        "sum": 671312.0730657278,
    },
    "aspect_s9": {
        "n": 8464,
        "null_cells": 1536,
        "min": 39.71495221659997,
        "max": 83.9110288746332,
        "mean": 71.32034704005915,
        "stddev": 8.656904375789493,
        "sum": 603655.4173470606,
    },
    "aspect_s9_c": {
        "n": 8464,
        "null_cells": 1536,
        "min": 39.71495221659997,
        "max": 83.9110288746332,
        "mean": 71.32034704005915,
        "stddev": 8.656904375789493,
        "sum": 603655.4173470606,
    },
    "profc_s3": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.010414957049241315,
        "max": 0.008138805255095718,
        "mean": -0.0004937251815518331,
        "stddev": 0.004140620299469786,
        "sum": -4.741736643623805,
    },
    "profc_s3_c": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.010414957049242677,
        "max": 0.008138805255103635,
        "mean": -0.0004937251815518332,
        "stddev": 0.004140620299469785,
        "sum": -4.741736643623806,
    },
    "profc_s9": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.009295406153998707,
        "max": 0.007503159731525539,
        "mean": -0.0004539438224726161,
        "stddev": 0.003838687462185371,
        "sum": -3.8421805134082225,
    },
    "profc_s9_c": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.009295406153998159,
        "max": 0.007503159731526574,
        "mean": -0.00045394382247261156,
        "stddev": 0.003838687462185365,
        "sum": -3.8421805134081843,
    },
    "planc_s3": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.024973893602114817,
        "max": 0.020000370561371668,
        "mean": -0.0016986388986200568,
        "stddev": 0.010304609544013556,
        "sum": -16.313727982347025,
    },
    "planc_s3_c": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.02497389360213334,
        "max": 0.020000370561371668,
        "mean": -0.0016986388986200568,
        "stddev": 0.010304609544013554,
        "sum": -16.313727982347025,
    },
    "planc_s9": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.023172116994388176,
        "max": 0.018705300916878005,
        "mean": -0.0015645116474066307,
        "stddev": 0.009899714748066968,
        "sum": -13.242026583649722,
    },
    "planc_s9_c": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.023172116994393328,
        "max": 0.01870530091687663,
        "mean": -0.0015645116474066426,
        "stddev": 0.009899714748066966,
        "sum": -13.242026583649823,
    },
    "longc_s3": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.01721728145598164,
        "max": 0.015066253883933475,
        "mean": -0.0009657769775810888,
        "stddev": 0.007465476241214705,
        "sum": -9.275322092688777,
    },
    "longc_s3_c": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.017217281455981635,
        "max": 0.015066253883918317,
        "mean": -0.0009657769775810888,
        "stddev": 0.007465476241214704,
        "sum": -9.275322092688777,
    },
    "longc_s9": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.015952890777756957,
        "max": 0.013745579123878547,
        "mean": -0.0009010148592352038,
        "stddev": 0.0070625245896366465,
        "sum": -7.626189768566765,
    },
    "longc_s9_c": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.01595289077775651,
        "max": 0.013745579123881262,
        "mean": -0.000901014859235197,
        "stddev": 0.00706252458963664,
        "sum": -7.626189768566707,
    },
    "crosc_s3": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.014822718543956488,
        "max": 0.016913746116061393,
        "mean": 0.0009357769775811592,
        "stddev": 0.008204945342702974,
        "sum": 8.987202092689452,
    },
    "crosc_s3_c": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.014822718543956484,
        "max": 0.016913746116046232,
        "mean": 0.0009357769775811592,
        "stddev": 0.008204945342702972,
        "sum": 8.987202092689452,
    },
    "crosc_s9": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.014107109222242262,
        "max": 0.016254420876121067,
        "mean": 0.0008710148592351738,
        "stddev": 0.007933921345768109,
        "sum": 7.372269768566511,
    },
    "crosc_s9_c": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.014107109222241813,
        "max": 0.016254420876123787,
        "mean": 0.0008710148592351805,
        "stddev": 0.007933921345768109,
        "sum": 7.372269768566568,
    },
    "maxic_s3": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.01381219566081021,
        "max": 0.017968408451255333,
        "mean": 0.005776959740874191,
        "stddev": 0.007496497052087006,
        "sum": 55.48192135135573,
    },
    "maxic_s3_c": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.013812195660810208,
        "max": 0.017968408451240172,
        "mean": 0.005776959740874191,
        "stddev": 0.007496497052087005,
        "sum": 55.48192135135573,
    },
    "maxic_s9": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.01288736143971942,
        "max": 0.01692093727122889,
        "mean": 0.00545960617406201,
        "stddev": 0.007026364910497034,
        "sum": 46.21010665726085,
    },
    "maxic_s9_c": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.012887361439718968,
        "max": 0.01692093727123161,
        "mean": 0.005459606174062016,
        "stddev": 0.007026364910497032,
        "sum": 46.21010665726091,
    },
    "minic_s3": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.018227804339127917,
        "max": 0.014011591548739535,
        "mean": -0.00580695974087412,
        "stddev": 0.007549720966249834,
        "sum": -55.770041351355054,
    },
    "minic_s3_c": {
        "n": 9604,
        "null_cells": 396,
        "min": -0.01822780433912791,
        "max": 0.014011591548724377,
        "mean": -0.00580695974087412,
        "stddev": 0.007549720966249832,
        "sum": -55.770041351355054,
    },
    "minic_s9": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.0171726385602798,
        "max": 0.013079062728770722,
        "mean": -0.005489606174062039,
        "stddev": 0.007079189263071018,
        "sum": -46.4640266572611,
    },
    "minic_s9_c": {
        "n": 8464,
        "null_cells": 1536,
        "min": -0.01717263856027935,
        "max": 0.013079062728773438,
        "mean": -0.005489606174062032,
        "stddev": 0.007079189263071013,
        "sum": -46.464026657261044,
    },
    "feature_s3": {
        "n": 9604,
        "null_cells": 396,
        "min": 1.0,
        "max": 5.0,
        "mean": 4.023115368596418,
        "stddev": 1.0211664727322596,
        "sum": 38638.0,
    },
    "feature_s3_c": {
        "n": 9604,
        "null_cells": 396,
        "min": 1.0,
        "max": 5.0,
        "mean": 4.023115368596418,
        "stddev": 1.0211664727322596,
        "sum": 38638.0,
    },
    "feature_s9": {
        "n": 8464,
        "null_cells": 1536,
        "min": 1.0,
        "max": 5.0,
        "mean": 4.016776937618148,
        "stddev": 1.0218326549968164,
        "sum": 33998.0,
    },
    "feature_s9_c": {
        "n": 8464,
        "null_cells": 1536,
        "min": 1.0,
        "max": 5.0,
        "mean": 4.016776937618148,
        "stddev": 1.0218326549968164,
        "sum": 33998.0,
    },
}

METHODS = [
    "elev",
    "slope",
    "aspect",
    "profc",
    "planc",
    "longc",
    "crosc",
    "maxic",
    "minic",
    "feature",
]
SIZES = [3, 9]


def _key(method, size, constrained):
    return f"{method}_s{size}{'_c' if constrained else ''}"


def _combos():
    for method in METHODS:
        for size in SIZES:
            for constrained in (False, True):
                yield method, size, constrained


@pytest.mark.parametrize(("method", "size", "constrained"), list(_combos()))
def test_param_scale_matches_reference(param_scale_session, method, size, constrained):
    """r.param.scale stats must match the main reference values."""
    key = _key(method, size, constrained)
    tools = Tools(session=param_scale_session)
    output = f"out_{key}"

    kwargs = {
        "input": "dem",
        "output": output,
        "method": method,
        "size": size,
        "overwrite": True,
    }
    if constrained:
        kwargs["flags"] = "c"
    tools.r_param_scale(**kwargs)

    stats = tools.r_univar(map=output, format="json")
    if isinstance(stats, list):
        stats = stats[0]
    reference = REFERENCE[key]

    # Cell counts and null pattern: exact integer equality.
    assert stats["n"] == reference["n"]
    assert stats["null_cells"] == reference["null_cells"]

    # Value statistics: relative tolerance, with an absolute floor that
    # covers near-zero values and a zero standard deviation.
    for field in ("min", "max", "mean", "stddev", "sum"):
        assert stats[field] == pytest.approx(reference[field], rel=1e-6, abs=1e-9)
