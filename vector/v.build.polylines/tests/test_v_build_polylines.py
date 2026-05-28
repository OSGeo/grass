import os
import pytest
import grass.script as gs


@pytest.fixture
def setup_polylines(tmp_path):
    """
    Fixture to create a basic GRASS project with a vector map 'lines' containing
    connected and disconnected line features.

    The vector is created from ASCII standard format input defining three lines:
      - Line 1: from (1,1) to (5,1)
      - Line 2: from (5,1) to (9,1) — connected to Line 1 end
      - Line 3: from (1,3) to (5,3) — separate line
    """
    project = tmp_path / "v_polylines_project"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        # Set computational region to cover the vector area
        gs.run_command("g.region", n=10, s=0, e=10, w=0, res=1, env=session.env)

        # ASCII standard vector input with header describing 3 lines
        line_data = """\
ORGANIZATION: GRASS Test
DIGIT DATE: today
DIGIT NAME: test
MAP NAME: lines
MAP DATE: today
MAP SCALE: 1
OTHER INFO:
ZONE: 0
MAP THRESH: 0.500000
VERTI:
L 2
1 1
5 1
L 2
5 1
9 1
L 2
1 3
5 3
"""

        # Import vector lines from ASCII standard input to map 'lines'
        gs.write_command(
            "v.in.ascii",
            input="-",
            output="lines",
            format="standard",
            overwrite=True,
            env=session.env,
            stdin=line_data,
        )

        yield session  # Provide session to tests


def test_v_build_polylines_merge(setup_polylines):
    """
    Test merging connected line segments into polylines.
    Expect 2 polylines:
      - One merged polyline combining Line 1 and Line 2 (1-5-9 on y=1)
      - One separate polyline for Line 3 (1-5 on y=3)
    """
    session = setup_polylines
    gs.run_command(
        "v.build.polylines",
        input="lines",
        output="merged_lines",
        overwrite=True,
        env=session.env,
    )
    vector_info = gs.parse_command(
        "v.info", map="merged_lines", flags="t", env=session.env
    )
    assert int(vector_info["lines"]) == 2


def test_cats_no_creates_no_categories(setup_polylines):
    """
    Test building polylines without assigning categories.
    The output vector should have no categories.
    """
    session = setup_polylines
    gs.run_command(
        "v.build.polylines",
        input="lines",
        output="no_cats",
        cats="no",
        overwrite=True,
        env=session.env,
    )
    cats = gs.read_command(
        "v.category", input="no_cats", option="print", env=session.env
    )
    assert cats.strip() == ""


def test_cats_first_preserves_first_cat(setup_polylines):
    """
    Test preserving the first category of original lines after building polylines.
    Categories are added starting from 1 with step 1 before merging.
    """
    session = setup_polylines
    # Add categories to original lines
    gs.run_command(
        "v.category",
        input="lines",
        option="add",
        cat=1,
        step=1,
        output="lines_cat",
        overwrite=True,
        env=session.env,
    )
    # Build polylines preserving first categories
    gs.run_command(
        "v.build.polylines",
        input="lines_cat",
        output="first_cat",
        cats="first",
        overwrite=True,
        env=session.env,
    )
    result = gs.read_command(
        "v.category", input="first_cat", option="print", env=session.env
    )
    assert "1" in result


def test_cats_multi_assigns_multiple_cats(setup_polylines):
    """
    Test assigning multiple categories to polylines built from features with multiple cats.
    Categories added starting at 10 with step 10 before merging.
    """
    session = setup_polylines
    gs.run_command(
        "v.category",
        input="lines",
        option="add",
        cat=10,
        step=10,
        output="lines_multi",
        overwrite=True,
        env=session.env,
    )
    gs.run_command(
        "v.build.polylines",
        input="lines_multi",
        output="multi_cat",
        cats="multi",
        overwrite=True,
        env=session.env,
    )
    result = gs.read_command(
        "v.category", input="multi_cat", option="print", env=session.env
    )
    assert result.count("10") >= 1


def test_cats_same_only_merges_similar_cats(setup_polylines):
    """
    Test merging polylines only if their categories are the same.
    Specifically:
      - Remove existing categories from original lines.
      - Add category 5 only to features with IDs 1 and 2.
      - Build polylines merging only those with the same category.
    Expect 2 merged lines (one merged for features 1 and 2, one separate).
    """
    session = setup_polylines
    # Remove all categories from original lines
    gs.run_command(
        "v.category",
        input="lines",
        option="del",
        output="lines_nocat",
        overwrite=True,
        env=session.env,
    )
    # Add category 5 to features with IDs 1 and 2 only
    gs.run_command(
        "v.category",
        input="lines_nocat",
        option="add",
        cat=5,
        step=0,
        output="lines_same",
        ids="1,2",
        overwrite=True,
        env=session.env,
    )
    # Build polylines merging features with the same categories only
    gs.run_command(
        "v.build.polylines",
        input="lines_same",
        output="same_cat",
        cats="same",
        overwrite=True,
        env=session.env,
    )
    info = gs.parse_command("v.info", map="same_cat", flags="t", env=session.env)
    assert int(info["lines"]) == 2


def test_build_only_boundaries(setup_polylines):
    """
    Test building polylines on vector features converted to boundary type.
    Resulting map should have boundary features.
    """
    session = setup_polylines
    # Convert lines to boundary type
    gs.run_command(
        "v.type",
        input="lines",
        output="boundaries",
        from_type="line",
        to_type="boundary",
        overwrite=True,
        env=session.env,
    )
    # Build polylines using only boundary features
    gs.run_command(
        "v.build.polylines",
        input="boundaries",
        output="poly_boundaries",
        type="boundary",
        overwrite=True,
        env=session.env,
    )
    info = gs.parse_command("v.info", map="poly_boundaries", flags="t", env=session.env)
    assert int(info["boundaries"]) > 0


def test_direction_independent_merge(setup_polylines):
    """
    Test merging polylines regardless of direction of line segments.
    This flips the direction of features and then builds polylines.
    Expect the same number of polylines (2) after merge.
    """
    session = setup_polylines
    # Build topology for the 'lines' vector
    gs.run_command("v.build", map="lines", env=session.env)
    # Flip the direction of features with IDs 1, 2, and 3
    gs.run_command(
        "v.edit", map="lines", tool="flip", type="line", ids="1,2,3", env=session.env
    )
    # Build polylines from flipped lines
    gs.run_command(
        "v.build.polylines",
        input="lines",
        output="flipped",
        overwrite=True,
        env=session.env,
    )
    info = gs.parse_command("v.info", map="flipped", flags="t", env=session.env)
    assert int(info["lines"]) == 2
