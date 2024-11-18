import pytest
from grass.script import run_command, read_command, parse_command


@pytest.fixture(scope="module")
def setup_map():
    """Set up a temporary region and generate a clumped map."""
    # Set the region
    run_command("g.region", n=3, s=0, e=3, w=0, res=1)

    # Create the custom map
    #  5   null null
    # null  5   null
    # null  6    6
    run_command(
        "r.mapcalc",
        expression=(
            "custom_map = "
            "if(row() == 1 && col() == 1, 5, "
            "if(row() == 2 && col() == 2, 5, "
            "if(row() == 3 && col() >= 2, 6, null())))"
        ),
        overwrite=True,
    )
    yield

    # Teardown: Remove maps
    run_command(
        "g.remove", flags="f", type="raster", name=["custom_map", "clumped_map"]
    )


def test_clump_basic(setup_map):
    """Test basic clumped map."""

    run_command("r.clump", input="custom_map", output="clumped_map", overwrite=True)

    output_maps = parse_command("g.list", type="raster")
    assert "clumped_map" in output_maps, "Output raster map 'clumped_map' should exist"

    category_output = read_command("r.category", map="clumped_map").strip().split("\n")
    actual_categories = {
        int(line.split("\t")[0]): line.split("\t")[1].strip() if "\t" in line else ""
        for line in category_output
    }

    expected_categories = {1: "", 2: "", 3: ""}

    assert set(actual_categories.keys()) == set(expected_categories.keys())


def test_clump_diagonal(setup_map):
    """Test clumped map with diagonal connectivity."""

    run_command(
        "r.clump", input="custom_map", output="clumped_map", flags="d", overwrite=True
    )

    output_maps = parse_command("g.list", type="raster")
    assert "clumped_map" in output_maps, "Output raster map 'clumped_map' should exist"

    category_output = read_command("r.category", map="clumped_map").strip().split("\n")
    actual_categories = {
        int(line.split("\t")[0]): line.split("\t")[1].strip() if "\t" in line else ""
        for line in category_output
    }

    expected_categories = {1: "", 2: ""}

    assert set(actual_categories.keys()) == set(expected_categories.keys())
