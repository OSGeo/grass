"""This is a pytest for r.geomorphon using a deterministic DEM. It covers reference tests, landform tests and a profile check."""

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
    "forms_skip1": {
        "n": 1296,
        "null_cells": 304,
        "min": 1,
        "max": 10,
        "mean": 2.49691358024691,
        "stddev": 2.44381110076768,
        "sum": 3236,
    },
}

# Each entry maps a reference key to its output option and any extra options.
COMBOS = [
    ("forms_default", "forms", {}),
    ("intensity_default", "intensity", {}),
    ("range_default", "range", {}),
    ("forms_extended", "forms", {"flags": "e", "search": 12}),
    ("forms_skip1", "forms", {"skip": 1}),
]

# All twelve raster outputs, produced in one call by the parallel tests.
OUTPUTS = [
    "forms",
    "ternary",
    "positive",
    "negative",
    "intensity",
    "exposition",
    "range",
    "variance",
    "elongation",
    "azimuth",
    "extend",
    "width",
]

# Landform categories from flat (1) to pit (10).
ALL_CLASSES = set(range(1, 11))

# Category labels that r.category reports for the forms output.
EXPECTED_LABELS = {
    1: "flat",
    2: "peak",
    3: "ridge",
    4: "shoulder",
    5: "spur",
    6: "slope",
    7: "hollow",
    8: "footslope",
    9: "valley",
    10: "pit",
}

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
    call = {"elevation": "dem", option: output, "search": SEARCH}
    call.update(extra)
    tools.r_geomorphon(**call)

    stats = tools.r_univar(map=output, format="json")
    ref = REFERENCE[key]
    assert stats["n"] == ref["n"]
    assert stats["null_cells"] == ref["null_cells"]
    for field in ("min", "max", "mean", "stddev", "sum"):
        assert stats[field] == pytest.approx(ref[field], rel=1e-6, abs=5e-8)


def test_landform_class_set(fixed_region):
    """The forms output contains every landform class with its category label."""
    tools = Tools(session=fixed_region)
    tools.r_geomorphon(elevation="dem", forms="forms_all", search=SEARCH)
    text = tools.r_stats(input="forms_all", flags="cn").stdout
    classes = {int(line.split()[0]) for line in text.splitlines() if line.strip()}
    assert classes == ALL_CLASSES
    categories = tools.r_category(map="forms_all").stdout
    for cat, label in EXPECTED_LABELS.items():
        assert f"{cat}\t{label}" in categories


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


def _assert_pairs_identical(tools, pairs):
    """Check that the two rasters in each pair match everywhere, both in value and in which cells are null."""
    value_expr = " + ".join(f"abs(double({a}) - double({b}))" for a, b in pairs)
    null_expr = " + ".join(f"(isnull({a}) != isnull({b}))" for a, b in pairs)
    tools.r_mapcalc(expression=f"_vdiff = {value_expr}", overwrite=True)
    tools.r_mapcalc(expression=f"_ndiff = {null_expr}", overwrite=True)
    # A nonzero maximum in either combined map means the outputs diverged.
    assert tools.r_univar(map="_ndiff", format="json")["max"] == 0
    assert tools.r_univar(map="_vdiff", format="json")["max"] == 0


class TestParallelIdentity:
    """Threaded output must equal serial output exactly."""

    def _run_all(self, tools, suffix, nprocs, extra=None):
        # One call produces all twelve outputs at the given thread count.
        call = {
            "elevation": "dem",
            "search": SEARCH,
            "nprocs": nprocs,
            "overwrite": True,
        }
        for out in OUTPUTS:
            call[out] = f"{out}_{suffix}"
        if extra:
            call.update(extra)
        tools.r_geomorphon(**call)

    def test_threads_1_vs_4(self, fixed_region):
        """All twelve outputs match between nprocs=1 and nprocs=4."""
        tools = Tools(session=fixed_region)
        self._run_all(tools, "s", 1)
        self._run_all(tools, "p", 4)
        _assert_pairs_identical(tools, [(f"{o}_s", f"{o}_p") for o in OUTPUTS])

    def test_band_seam_identity(self, fixed_region):
        """memory=0 forces multiple bands and the output still matches serial."""
        tools = Tools(session=fixed_region)
        self._run_all(tools, "s", 1)
        self._run_all(tools, "b", 4, extra={"memory": 0})
        _assert_pairs_identical(tools, [(f"{o}_s", f"{o}_b") for o in OUTPUTS])

    def test_min_legal_height(self, geomorphon_session):
        """A region of exactly 2*search+2 rows at nprocs=4 matches serial."""
        tools = Tools(session=geomorphon_session)
        tools.g_region(s=0, n=12, w=0, e=40, res=1)
        tools.r_geomorphon(
            elevation="dem", forms="forms_s12", search=SEARCH, nprocs=1, overwrite=True
        )
        tools.r_geomorphon(
            elevation="dem", forms="forms_p12", search=SEARCH, nprocs=4, overwrite=True
        )
        _assert_pairs_identical(tools, [("forms_s12", "forms_p12")])

    def test_mask_identity(self, fixed_region):
        """With a mask active, threaded output matches serial."""
        tools = Tools(session=fixed_region)
        tools.r_mapcalc(expression="msk = if(col() < 20, 1, null())", overwrite=True)
        tools.r_mask(raster="msk")
        try:
            tools.r_geomorphon(
                elevation="dem",
                forms="forms_sm",
                search=SEARCH,
                nprocs=1,
                overwrite=True,
            )
            tools.r_geomorphon(
                elevation="dem",
                forms="forms_pm",
                search=SEARCH,
                nprocs=4,
                overwrite=True,
            )
            _assert_pairs_identical(tools, [("forms_sm", "forms_pm")])
        finally:
            tools.r_mask(flags="r")

    def test_extended_identity(self, fixed_region):
        """The extended correction path matches between serial and threaded."""
        tools = Tools(session=fixed_region)
        tools.r_geomorphon(
            elevation="dem",
            forms="forms_se",
            search=12,
            flags="e",
            nprocs=1,
            overwrite=True,
        )
        tools.r_geomorphon(
            elevation="dem",
            forms="forms_pe",
            search=12,
            flags="e",
            nprocs=4,
            overwrite=True,
        )
        _assert_pairs_identical(tools, [("forms_se", "forms_pe")])
