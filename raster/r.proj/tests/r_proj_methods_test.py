"""Reference-value test for r.proj across interpolation methods.

For every interpolation method, r.proj is run at OMP_NUM_THREADS=1 on the
generated input_mid raster reprojected into EPSG:3857, and its univariate
statistics are compared against reference values. This verifies that changes
such as the OpenMP parallelization do not change the serial output.

Reference provenance: values captured from r.proj.serial (md5
cde07de7d7b2e2798fb203fc01b27714), the frozen main-branch serial binary, run
at OMP_NUM_THREADS=1 on the session_3857 / input_mid fixture data with the
output region set from r.proj -g. The single-thread run matches the
single-thread reference. All seven references are pairwise distinct, and every
pairwise method swap is catchable at the primary rel=1e-7 tolerance (the
input_mid curvature term is chosen so bilinear_f and bicubic_f diverge past
1e-7).

The rel=1e-6 relaxation pre-registered below, if ever invoked on a CI
floating-point flutter, sacrifices the bilinear_f/bicubic_f swap detection
(their means differ by only ~2.6e-7 relative, under 1e-6) -- so relaxing is a
visible trade, not a silent one.
"""

import pytest

import grass.script as gs

SRC_PROJECT = "src4326"
INPUT_MID = "input_mid"

REFERENCE = {
    "nearest": {
        "n": 2500,
        "null_cells": 0,
        "min": 103,
        "max": 5058,
        "mean": 2581.0344,
        "stddev": 1443.17823750798,
    },
    "bilinear": {
        "n": 2352,
        "null_cells": 148,
        "min": 202.023132324219,
        "max": 5054.64990234375,
        "mean": 2624.75047491683,
        "stddev": 1414.50216671975,
    },
    "bicubic": {
        "n": 2162,
        "null_cells": 338,
        "min": 306.120361328125,
        "max": 4958.90966796875,
        "mean": 2624.59341703092,
        "stddev": 1356.74263472616,
    },
    "lanczos": {
        "n": 2116,
        "null_cells": 384,
        "min": 305.625366210938,
        "max": 4851.7939453125,
        "mean": 2572.88704589094,
        "stddev": 1327.61055457531,
    },
    "bilinear_f": {
        "n": 2500,
        "null_cells": 0,
        "min": 103,
        "max": 5058,
        "mean": 2575.07724680176,
        "stddev": 1443.17041934389,
    },
    "bicubic_f": {
        "n": 2500,
        "null_cells": 0,
        "min": 103,
        "max": 5058,
        "mean": 2575.0779140625,
        "stddev": 1443.17057576919,
    },
    "lanczos_f": {
        "n": 2500,
        "null_cells": 0,
        "min": 103,
        "max": 5058,
        "mean": 2573.76246621094,
        "stddev": 1443.17228983607,
    },
}

METHODS = list(REFERENCE)


def _set_region_from_source(env):
    """Set the output region to r.proj's suggested bounds (method-independent)."""
    text = gs.read_command(
        "r.proj",
        project=SRC_PROJECT,
        mapset="PERMANENT",
        input=INPUT_MID,
        method="nearest",
        flags="g",
        env=env,
    )
    region = dict(token.split("=") for token in text.split())
    gs.run_command(
        "g.region",
        n=region["n"],
        s=region["s"],
        e=region["e"],
        w=region["w"],
        rows=region["rows"],
        cols=region["cols"],
        env=env,
    )


@pytest.mark.parametrize("method", METHODS)
def test_method_matches_serial_reference(session_3857, method):
    """r.proj serial output stats must match the captured serial reference."""
    env = dict(session_3857.env)
    env["OMP_NUM_THREADS"] = "1"
    _set_region_from_source(env)

    output = f"ref_{method}"
    gs.run_command(
        "r.proj",
        project=SRC_PROJECT,
        mapset="PERMANENT",
        input=INPUT_MID,
        output=output,
        method=method,
        overwrite=True,
        quiet=True,
        env=env,
    )
    stats = gs.parse_command("r.univar", map=output, flags="g", env=env)
    reference = REFERENCE[method]

    assert int(stats["n"]) == reference["n"]
    assert int(stats["null_cells"]) == reference["null_cells"]
    # Primary tolerance rel=1e-7. The only permitted relaxation, on an actual
    # CI floating-point flutter (naming the platform that showed it), is Anna's
    # r.param.scale tolerance rel=1e-6, abs=5e-8; see the module docstring for
    # the bilinear_f/bicubic_f detection it trades away.
    for field in ("min", "max", "mean", "stddev"):
        assert float(stats[field]) == pytest.approx(reference[field], rel=1e-7)
