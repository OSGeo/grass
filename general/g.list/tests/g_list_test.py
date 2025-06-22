import grass.script as gs
import pytest
import sys


def test_default_output(simple_dataset):
    """Test default output of g.list."""
    # Current mapset only
    actual = gs.read_command("g.list", type="all", env=simple_dataset.env).splitlines()
    expected = ["raster_test_1", "vector_test_1"]
    assert actual == expected

    # All mapsets
    actual = gs.read_command(
        "g.list", type="all", mapset="*", env=simple_dataset.env
    ).splitlines()
    expected = [
        "raster_test_1",
        "raster_test_2",
        "vector_test_1",
        "vector_test_2",
    ]
    assert actual == expected


def test_data_types_output(simple_dataset):
    """Test g.list with t flag."""
    # Current mapset only
    actual = gs.read_command(
        "g.list", type="all", flags="t", env=simple_dataset.env
    ).splitlines()
    expected = ["raster/raster_test_1", "vector/vector_test_1"]
    assert actual == expected

    # All mapsets
    actual = gs.read_command(
        "g.list", type="all", flags="t", mapset="*", env=simple_dataset.env
    ).splitlines()
    expected = [
        "raster/raster_test_1",
        "raster/raster_test_2",
        "vector/vector_test_1",
        "vector/vector_test_2",
    ]
    assert actual == expected


def test_full_map_output(simple_dataset):
    """Test g.list with m flag."""
    # Current mapset only
    actual = gs.read_command(
        "g.list", type="all", flags="m", env=simple_dataset.env
    ).splitlines()
    expected = ["raster_test_1@test_1", "vector_test_1@test_1"]
    assert actual == expected

    # All mapsets
    actual = gs.read_command(
        "g.list", type="all", flags="m", mapset="*", env=simple_dataset.env
    ).splitlines()
    expected = [
        "raster_test_1@test_1",
        "raster_test_2@test_2",
        "vector_test_1@test_1",
        "vector_test_2@test_2",
    ]
    assert actual == expected


def test_human_readable_output(simple_dataset):
    """Test g.list with p flag."""
    # Current mapset only
    actual = gs.read_command(
        "g.list", type="all", flags="p", env=simple_dataset.env
    ).splitlines()
    expected = [
        "raster files available in mapset <test_1>:",
        "raster_test_1",
        "no 3D raster files available in current mapset",
        "vector files available in mapset <test_1>:",
        "vector_test_1",
        "no label files available in current mapset",
        "no region definition files available in current mapset",
        "no imagery group files available in current mapset",
    ]
    for line in expected:
        assert line in actual, f"Expected line not found: {line}"

    # All mapsets
    actual = gs.read_command(
        "g.list", type="all", flags="p", mapset="*", env=simple_dataset.env
    ).splitlines()
    expected = [
        "raster files available in mapset <test_1>:",
        "raster_test_1",
        "raster files available in mapset <test_2>:",
        "raster_test_2",
        "no 3D raster files available in current mapset",
        "vector files available in mapset <test_1>:",
        "vector_test_1",
        "vector files available in mapset <test_2>:",
        "vector_test_2",
        "no label files available in current mapset",
        "no region definition files available in current mapset",
        "no imagery group files available in current mapset",
    ]
    for line in expected:
        assert line in actual, f"Expected line not found: {line}"


@pytest.mark.xfail(
    sys.platform == "win32",
    reason="map titles are not listed",
)
def test_verbose_listing_output(simple_dataset):
    """Test g.list with f flag."""
    # Current mapset only
    actual = gs.read_command(
        "g.list", type="all", flags="f", env=simple_dataset.env
    ).splitlines()
    expected = [
        "raster files available in mapset <test_1>:",
        "raster_test_1      Raster title test_1",
        "no raster files available in mapset <PERMANENT>",
        "vector files available in mapset <test_1>:",
        "vector_test_1      Vector title test_1",
        "no vector files available in mapset <PERMANENT>",
        "no 3D raster files available in current mapset",
        "no label files available in current mapset",
        "no region definition files available in current mapset",
        "no imagery group files available in current mapset",
    ]
    for line in expected:
        assert line in actual, f"Expected line not found: {line}"

    # All mapsets
    actual = gs.read_command(
        "g.list", type="all", flags="f", mapset="*", env=simple_dataset.env
    ).splitlines()
    expected = [
        "raster files available in mapset <test_1>:",
        "raster_test_1      Raster title test_1",
        "no raster files available in mapset <PERMANENT>",
        "raster files available in mapset <test_2>:",
        "raster_test_2      Raster title test_2",
        "vector files available in mapset <test_1>:",
        "vector_test_1      Vector title test_1",
        "no vector files available in mapset <PERMANENT>",
        "vector files available in mapset <test_2>:",
        "vector_test_2      Vector title test_2",
        "no 3D raster files available in current mapset",
        "no label files available in current mapset",
        "no region definition files available in current mapset",
        "no imagery group files available in current mapset",
    ]
    for line in expected:
        assert line in actual, f"Expected line not found: {line}"
