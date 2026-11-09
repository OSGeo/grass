"""Tests of v.ppa summary function estimates and output formats.

The statistical tests run on the seeded CSR pattern from conftest.py,
so results are deterministic. Tolerances are set to roughly twice the
deviations observed for that pattern.
"""

from io import StringIO

import pytest

from grass.exceptions import CalledModuleError


def test_k_isotropic_tracks_csr(tools):
    """Corrected K on a CSR pattern stays close to pi * d^2."""
    data = tools.v_ppa(input="points_csr", method="k", format="json").json
    assert data["method"] == "k"
    assert data["points"] == 500
    assert data["correction"] == "isotropic"
    assert data["intensity"] == pytest.approx(500 / 1_000_000)
    assert data["window"] == {"west": 0, "east": 1000, "south": 0, "north": 1000}
    results = data["results"]
    assert len(results) == 100
    assert results[-1]["distance"] == 250
    for row in results[10:]:
        assert row["k"] == pytest.approx(row["k_csr"], rel=0.2)


def test_k_correction_none_biased_down(tools):
    """Without edge correction K is biased below the corrected estimate."""
    iso = tools.v_ppa(input="points_csr", method="k", format="json").json
    none = tools.v_ppa(
        input="points_csr", method="k", format="json", correction="none"
    ).json
    assert none["correction"] == "none"
    last_iso = iso["results"][-1]
    last_none = none["results"][-1]
    assert last_none["k"] < last_iso["k"]
    assert last_none["k"] < 0.9 * last_none["k_csr"]


def test_k_max_distance(tools):
    """The max_distance option sets the largest evaluated distance."""
    data = tools.v_ppa(
        input="points_csr", method="k", format="json", max_distance=100
    ).json
    assert data["results"][-1]["distance"] == 100


def test_l_tracks_csr(tools):
    """L on a CSR pattern stays close to the distance itself."""
    data = tools.v_ppa(input="points_csr", method="l", format="json").json
    for row in data["results"][10:]:
        assert row["l"] == pytest.approx(row["distance"], rel=0.1)
        assert row["l_csr"] == row["distance"]


def test_g_tracks_csr(tools):
    """G on a CSR pattern stays close to 1 - exp(-lambda * pi * d^2)."""
    data = tools.v_ppa(input="points_csr", method="g", format="json").json
    assert "correction" not in data
    for row in data["results"]:
        assert row["g"] == pytest.approx(row["g_csr"], abs=0.1)
    assert data["results"][-1]["g"] == 1.0


def test_f_tracks_csr(tools):
    """F on a CSR pattern stays close to 1 - exp(-lambda * pi * d^2)."""
    data = tools.v_ppa(input="points_csr", method="f", format="json", seed=7).json
    for row in data["results"]:
        assert row["f"] == pytest.approx(row["f_csr"], abs=0.1)


def test_f_seed_reproducible(tools):
    """The same seed reproduces F exactly, a different seed does not."""
    first = tools.v_ppa(input="points_csr", method="f", format="json", seed=7).json
    again = tools.v_ppa(input="points_csr", method="f", format="json", seed=7).json
    other = tools.v_ppa(input="points_csr", method="f", format="json", seed=8).json
    assert first == again
    assert first != other


def test_clustered_pattern_detected(tools):
    """K on a clustered pattern lies well above the CSR curve."""
    tools.v_perturb(
        input="points_csr",
        output="points_clustered",
        parameters=5,
        seed=99,
    )
    tools.v_patch(input=["points_csr", "points_clustered"], output="points_pairs")
    data = tools.v_ppa(input="points_pairs", method="k", format="json").json
    small = [row for row in data["results"] if row["distance"] <= 10]
    assert small
    assert all(row["k"] > 2.5 * row["k_csr"] for row in small)


def test_num_distances(tools):
    """All methods honor num_distances."""
    for method in ["g", "f", "k", "l"]:
        data = tools.v_ppa(
            input="points_csr", method=method, format="json", num_distances=42
        ).json
        assert len(data["results"]) == 42


def test_csv_format(tools):
    """CSV output has a header and one line per distance."""
    text = tools.v_ppa(input="points_csr", method="k", format="csv").text
    lines = text.splitlines()
    assert lines[0] == "distance,k,k_csr"
    assert len(lines) == 101
    assert all(len(line.split(",")) == 3 for line in lines[1:])


def test_plain_format(tools):
    """Plain output is space separated with a header."""
    text = tools.v_ppa(input="points_csr", method="l").text
    lines = text.splitlines()
    assert lines[0] == "distance l l_csr"
    assert len(lines) == 101


def test_output_file(tools, tmp_path):
    """Results are written to the file given by the output option."""
    path = tmp_path / "k.csv"
    tools.v_ppa(input="points_csr", method="k", format="csv", output=str(path))
    lines = path.read_text().splitlines()
    assert lines[0] == "distance,k,k_csr"
    assert len(lines) == 101


def test_duplicate_coordinates(tools):
    """Duplicate coordinates count as nearest neighbors at distance 0."""
    tools.v_in_ascii(
        input=StringIO("500|500\n500|500\n100|100\n900|900\n100|900"),
        output="points_duplicates",
    )
    data = tools.v_ppa(input="points_duplicates", method="g", format="json").json
    assert data["points"] == 5
    assert data["results"][0]["g"] >= 0.4


def test_points_outside_region(tools):
    """Points outside the computational region are excluded."""
    tools.g_region(s=0, n=500, w=0, e=500)
    try:
        data = tools.v_ppa(input="points_csr", method="k", format="json").json
        assert 0 < data["points"] < 500
        assert data["window"]["north"] == 500
        assert data["intensity"] == pytest.approx(data["points"] / 250_000)
    finally:
        tools.g_region(s=0, n=1000, w=0, e=1000)


def test_too_few_points(tools):
    """G, K, and L require at least two points."""
    tools.v_in_ascii(input=StringIO("500|500"), output="point_single")
    with pytest.raises(CalledModuleError):
        tools.v_ppa(input="point_single", method="g")


def test_invalid_num_distances(tools):
    """A non-positive num_distances is rejected."""
    with pytest.raises(CalledModuleError):
        tools.v_ppa(input="points_csr", method="g", num_distances=0)
