import os
import pytest
import json
import grass.script as gs


@pytest.fixture
def setup_maps(tmp_path):
    """Set up a GRASS session and create test raster maps."""

    # Initialize GRASS project
    project = tmp_path / "r_clump_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        # Set the region
        gs.run_command(
            "g.region",
            n=3,
            s=0,
            e=3,
            w=0,
            res=1,
            env=session.env,
        )

        gs.mapcalc(
            "custom_map = "
            "if(row() == 1 && col() == 1, 5, "
            "if(row() == 1 && col() == 2, 5.5, "
            "if(row() == 2 && col() == 2, 5, "
            "if(row() == 3 && col() >= 2, 5.5, null()))))",
            overwrite=True,
            env=session.env,
        )

        gs.mapcalc(
            "custom_map1 = "
            "if(row() == 1 && col() == 1, 1, "
            "if(row() == 1 && col() == 2, 2, "
            "if(row() == 1 && col() == 3, 3, "
            "if(row() == 2 && col() == 1, 4, "
            "if(row() == 2 && col() == 2, 5, "
            "if(row() == 2 && col() == 3, 6, "
            "if(row() == 3 && col() == 1, 7, "
            "if(row() == 3 && col() == 2, 8, "
            "if(row() == 3 && col() == 3, 9, null())))))))))",
            overwrite=True,
            env=session.env,
        )

        yield session  # Pass the session to tests


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
