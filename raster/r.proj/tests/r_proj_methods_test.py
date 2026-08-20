"""These are the reference value tests for r.proj across the interpolation methods.

Each method reprojects input_mid into EPSG:3857. The method's r.univar stats
must also match the references taken from the serial r.proj on the main branch.
"""

import pytest

import grass.script as gs
from grass.tools import Tools

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
    tools = Tools(env=env)
    bounds = tools.r_proj(
        project=SRC_PROJECT,
        mapset="PERMANENT",
        input=INPUT_MID,
        method="nearest",
        flags="p",
        format="json",
    ).json
    tools.g_region(
        n=bounds["north"],
        s=bounds["south"],
        e=bounds["east"],
        w=bounds["west"],
        rows=bounds["rows"],
        cols=bounds["cols"],
    )


@pytest.mark.parametrize("method", METHODS)
def test_method_matches_serial_reference(session_3857, method):
    """r.proj serial output stats must match the captured serial reference."""
    env = dict(session_3857.env)
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
    # Value statistics compared at a relative tolerance of 1e-7.
    for field in ("min", "max", "mean", "stddev"):
        assert float(stats[field]) == pytest.approx(reference[field], rel=1e-7)
