import pytest
import grass.script as gs


def test_g_version_no_flag():
    """Test that g.version output contains the word 'GRASS'."""
    output = gs.read_command("g.version").strip()

    assert (
        "GRASS" in output
    ), "Expected 'GRASS' in g.version output, but it was not found."


def test_c_flag():
    """Test the output of g.version -c for Copyright and License Statement."""

    expected_text = "Copyright and License Statement"

    output = gs.read_command("g.version", flags="c").strip()

    assert expected_text in output, (
        f"Expected '{expected_text}' in g.version -c output, " f"but got: '{output}'"
    )


def test_e_flag():
    """Test that g.version -e contains the expected keys."""
    expected_keys = ["PROJ:", "GDAL/OGR:", "SQLite:"]

    output = gs.read_command("g.version", flags="e").strip()

    for key in expected_keys:
        assert (
            key in output
        ), f"Expected key '{key}' in g.version -e output, but it was not found."


def test_b_flag():
    """Test that g.version -b output contains the word 'GRASS'."""
    output = gs.read_command("g.version", flags="b").strip()

    assert "GRASS" in output, (
        "Expected 'GRASS' in g.version -b output, but it was not found."
    )


def test_g_flag():
    """Test that g.version -g contains the expected keys."""
    expected_keys = [
        "version=",
        "date=",
        "revision=",
        "build_date=",
        "build_platform=",
        "build_off_t_size="
    ]

    output = gs.read_command("g.version", flags="g").strip()

    for key in expected_keys:
        assert (
            key in output
        ), f"Expected key '{key}' in g.version -g output, but it was not found."


def test_r_flag():
    """Test that g.version -r contains the expected keys."""
    expected_keys = ["libgis revision:", "libgis date:"]

    output = gs.read_command("g.version", flags="r").strip()

    for key in expected_keys:
        assert (
            key in output
        ), f"Expected key '{key}' in g.version -r output, but it was not found."


def test_x_flag():
    """Test that g.version -x output contains the correct citation information."""
    expected = [
        "@Manual{GRASS_GIS_software,",
        "title = {Geographic Resources Analysis Support System (GRASS) Software, Version X.Y},",
        "author = {{GRASS Development Team}},",
        "organization = {Open Source Geospatial Foundation},",
        "address = {USA},",
        "year = {YEAR},",
        "url = {https://grass.osgeo.org},",
    ]

    output = gs.read_command("g.version", flags="x").strip()

    for item in expected:
        assert item in output, (
            f"Expected '{item}' in g.version -x output, but it was not found."
        )
