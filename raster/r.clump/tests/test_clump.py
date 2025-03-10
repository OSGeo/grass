import grass.script as gs
import json


def test_clump_basic(setup_maps):
    """Test basic clumped map."""
    session = setup_maps
    gs.run_command(
        "r.clump",
        input="custom_map",
        output="clumped_map",
        overwrite=True,
        env=session.env,
    )

    output_maps = gs.parse_command("g.list", type="raster", env=session.env)
    assert "clumped_map" in output_maps, "Output raster map 'clumped_map' should exist"

    category_output = gs.read_command(
        "r.category", map="clumped_map", output_format="json", env=session.env
    )
    category_data = json.loads(category_output)

    expected_categories = [
        {"category": 1, "description": ""},
        {"category": 2, "description": ""},
        {"category": 3, "description": ""},
        {"category": 4, "description": ""},
    ]

    assert category_data == expected_categories, (
        "Category data does not match expected categories"
    )


def test_clump_diagonal(setup_maps):
    """Test clumped map with diagonal connectivity."""
    session = setup_maps
    gs.run_command(
        "r.clump",
        input="custom_map",
        output="clumped_map",
        flags="d",
        overwrite=True,
        env=session.env,
    )

    output_maps = gs.parse_command("g.list", type="raster", env=session.env)
    assert "clumped_map" in output_maps, "Output raster map 'clumped_map' should exist"

    category_output = gs.read_command(
        "r.category", map="clumped_map", output_format="json", env=session.env
    )
    category_data = json.loads(category_output)

    expected_categories = [
        {"category": 1, "description": ""},
        {"category": 2, "description": ""},
        {"category": 3, "description": ""},
    ]
    assert category_data == expected_categories, (
        "Category data does not match expected categories"
    )


def test_clump_minsize(setup_maps):
    """Test clumped map with minimum size parameter."""
    session = setup_maps
    gs.run_command(
        "r.clump",
        input="custom_map",
        output="clumped_map",
        minsize=2,
        overwrite=True,
        env=session.env,
    )

    output_maps = gs.parse_command("g.list", type="raster", env=session.env)
    assert "clumped_map" in output_maps, "Output raster map 'clumped_map' should exist"

    category_output = gs.read_command(
        "r.category", map="clumped_map", output_format="json", env=session.env
    )
    category_data = json.loads(category_output)

    expected_categories = [
        {"category": 1, "description": ""},
        {"category": 2, "description": ""},
    ]

    assert category_data == expected_categories, (
        "Category data does not match expected categories"
    )


def test_clump_threshold(setup_maps):
    """Test clumped map with threshold parameter."""
    session = setup_maps
    gs.run_command(
        "r.clump",
        input="custom_map1",
        output="clumped_map",
        threshold=0.2,
        overwrite=True,
        env=session.env,
    )

    output_maps = gs.parse_command("g.list", type="raster", env=session.env)
    assert "clumped_map" in output_maps, "Output raster map 'clumped_map' should exist"

    category_output = gs.read_command(
        "r.category", map="clumped_map", output_format="json", env=session.env
    )
    category_data = json.loads(category_output)

    expected_categories = [
        {"category": 1, "description": ""},
        {"category": 2, "description": ""},
        {"category": 3, "description": ""},
    ]

    assert category_data == expected_categories, (
        "Category data does not match expected categories"
    )
