"""Reference and landform tests for r.geomorphon on a deterministic DEM."""

import json

import pytest

from grass.tools import Tools

SEARCH = 5

REFERENCE = {
    "forms_default": {
        "n": 1444,
        "null_cells": 156,
        "min": 1,
        "max": 10,
        "mean": 2.34349030470914,
        "stddev": 2.31663094892283,
        "sum": 3384,
    },
    "intensity_default": {
        "n": 1444,
        "null_cells": 156,
        "min": -8.88377952575684,
        "max": 8.88377952575684,
        "mean": 0,
        "stddev": 0.974596266625059,
        "sum": 0,
    },
    "range_default": {
        "n": 1444,
        "null_cells": 156,
        "min": 0,
        "max": 13.9962615966797,
        "mean": 2.27708118047741,
        "stddev": 3.85280566774583,
        "sum": 3288.10522460938,
    },
    "forms_extended": {
        "n": 1444,
        "null_cells": 156,
        "min": 1,
        "max": 10,
        "mean": 2.50277008310249,
        "stddev": 2.39415680072826,
        "sum": 3614,
    },
    "forms_meters": {
        "n": 1444,
        "null_cells": 156,
        "min": 1,
        "max": 10,
        "mean": 2.34349030470914,
        "stddev": 2.31663094892283,
        "sum": 3384,
    },
    "forms_skip1": {
        "n": 1296,
        "null_cells": 304,
        "min": 1,
        "max": 10,
        "mean": 2.49691358024691,
        "stddev": 2.44381110076768,
        "sum": 3236,
    },
    "forms_anglev2": {
        "n": 1444,
        "null_cells": 156,
        "min": 1,
        "max": 10,
        "mean": 2.34349030470914,
        "stddev": 2.31663094892283,
        "sum": 3384,
    },
    "forms_anglev2_distance": {
        "n": 1444,
        "null_cells": 156,
        "min": 1,
        "max": 10,
        "mean": 2.34349030470914,
        "stddev": 2.31663094892283,
        "sum": 3384,
    },
}

# Each entry maps a reference key to its output option and any extra options.
COMBOS = [
    ("forms_default", "forms", {}),
    ("intensity_default", "intensity", {}),
    ("range_default", "range", {}),
    ("forms_extended", "forms", {"flags": "e", "search": 12}),
    ("forms_meters", "forms", {"flags": "m"}),
    ("forms_skip1", "forms", {"skip": 1}),
    ("forms_anglev2", "forms", {"comparison": "anglev2"}),
    ("forms_anglev2_distance", "forms", {"comparison": "anglev2_distance"}),
]

# Landform categories from flat (1) to pit (10).
ALL_CLASSES = set(range(1, 11))

# Expected final_results of the one-off profile at easting 13, northing 16.
PROFILE_FINAL = {
    "azimuth": 90,
    "elongation": 3,
    "width_m": 2,
    "intensity_m": 1.1735076904296875,
    "exposition_m": 5.1710052490234375,
    "range_m": 9.4890289306640625,
    "variance": 13.911697387695312,
    "extends": 0.11313708498984762,
    "octagon_perimeter_m": 13.152982445134985,
    "octagon_area_m2": 8,
    "mesh_perimeter_m": 26.577144026719676,
    "mesh_area_m2": 32.973703233080755,
}


@pytest.fixture
def fixed_region(geomorphon_session):
    """Session with the region reset to the DEM extent."""
    Tools(session=geomorphon_session).g_region(s=0, n=40, w=0, e=40, res=1)
    return geomorphon_session


@pytest.mark.parametrize(("key", "option", "extra"), COMBOS)
def test_reference_values(fixed_region, key, option, extra):
    """Module statistics match the captured serial reference."""
    tools = Tools(session=fixed_region)
    output = f"out_{key}"
    call = {"elevation": "dem", option: output, "search": SEARCH, "overwrite": True}
    call.update(extra)
    tools.r_geomorphon(**call)

    stats = tools.r_univar(map=output, format="json")
    ref = REFERENCE[key]
    assert stats["n"] == ref["n"]
    assert stats["null_cells"] == ref["null_cells"]
    for field in ("min", "max", "mean", "stddev", "sum"):
        assert stats[field] == pytest.approx(ref[field], rel=1e-6, abs=5e-8)


def test_landform_class_set(fixed_region):
    """The forms output contains every landform class from flat to pit."""
    tools = Tools(session=fixed_region)
    tools.r_geomorphon(
        elevation="dem", forms="forms_all", search=SEARCH, overwrite=True
    )
    text = tools.r_stats(input="forms_all", flags="cn").stdout
    classes = {int(line.split()[0]) for line in text.splitlines() if line.strip()}
    assert classes == ALL_CLASSES


def test_profile_json(fixed_region):
    """The one-off profile reports the expected spur landform and geometry."""
    tools = Tools(session=fixed_region)
    out = tools.r_geomorphon(
        elevation="dem",
        coordinates="13,16",
        profiledata="-",
        profileformat="json",
        search=SEARCH,
    ).stdout
    result = json.loads(out)

    assert {
        "map_info",
        "computation_parameters",
        "intermediate_data",
        "final_results",
    } <= set(result)
    final = result["final_results"]
    assert final["landform_cat"] == 5
    assert final["landform_code"] == "SP"
    assert final["landform_name"] == "spur"
    for field, value in PROFILE_FINAL.items():
        assert final[field] == pytest.approx(value, rel=1e-6, abs=5e-8)
    inter = result["intermediate_data"]
    assert inter["num_positives"] == 3
    assert inter["num_negatives"] == 5
    assert inter["pattern_size"] == 8
