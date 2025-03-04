import grass.script as gs


def results_check(actual_categories, expected_categories):
    # Test if keys match
    assert set(actual_categories.keys()) == set(expected_categories.keys())

    # Test to check if labels are empty
    for key in expected_categories.keys():
        assert actual_categories[key] == expected_categories[key]


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

    category_output = gs.parse_command("r.category", map="clumped_map", env=session.env)

    actual_categories = {
        int(line.split("\t")[0]): line.split("\t")[1].strip() if "\t" in line else ""
        for line in category_output
    }

    expected_categories = {1: "", 2: "", 3: "", 4: ""}

    results_check(actual_categories, expected_categories)


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

    category_output = gs.parse_command("r.category", map="clumped_map", env=session.env)

    actual_categories = {
        int(line.split("\t")[0]): line.split("\t")[1].strip() if "\t" in line else ""
        for line in category_output
    }

    expected_categories = {1: "", 2: "", 3: ""}

    results_check(actual_categories, expected_categories)


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

    category_output = gs.parse_command("r.category", map="clumped_map", env=session.env)

    actual_categories = {
        int(line.split("\t")[0]): line.split("\t")[1].strip() if "\t" in line else ""
        for line in category_output
    }

    expected_categories = {1: "", 2: ""}

    results_check(actual_categories, expected_categories)


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

    category_output = gs.parse_command("r.category", map="clumped_map", env=session.env)

    actual_categories = {
        int(line.split("\t")[0]): line.split("\t")[1].strip() if "\t" in line else ""
        for line in category_output
    }

    expected_categories = {1: "", 2: "", 3: ""}

    results_check(actual_categories, expected_categories)
