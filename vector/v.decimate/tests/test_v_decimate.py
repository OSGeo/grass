import os
import io
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools
from grass.exceptions import CalledModuleError


@pytest.fixture
def setup_point_map(tmp_path):
    """
    Create a temporary GRASS GIS project and load a 3D vector point map with category values.
    Points clustered to allow meaningful decimation tests.
    """
    mapname = "test_points"
    points = [
        (100, 100, 1.0, 1),
        (100.05, 100.05, 1.1, 1),
        (100.1, 100.1, 1.2, 1),
        (101, 100, 2.0, 2),
        (102, 100, 3.0, 2),
        (103, 100, 4.0, 2),
        (104, 100, 5.0, 3),
        (105, 100, 6.0, 3),
        (106, 100, 7.0, 3),
    ]
    coords = "\n".join(f"{x}|{y}|{z}|{cat}" for x, y, z, cat in points)

    project = tmp_path / "grassdata"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=110, s=90, e=110, w=90, res=1)

        tools.v_in_ascii(
            input=io.StringIO(coords),
            output=mapname,
            separator="|",
            flags="z",
            z=3,
            cat=4,
        )

        yield SimpleNamespace(input_map=mapname, session=session)


@pytest.mark.parametrize("preserve_value", [2, 4])
def test_v_decimate_preserve(setup_point_map, preserve_value):
    """Test count-based decimation using the preserve parameter."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)

    output_map = f"decimated_preserve_{preserve_value}"
    tools.v_decimate(
        input=input_map, output=output_map, preserve=preserve_value, overwrite=True
    )

    orig_info = tools.v_info(map=input_map, flags="t", format="json")
    dec_info = tools.v_info(map=output_map, flags="t", format="json")

    assert dec_info["points"] > 0, f"Decimated map {output_map} has no points"
    assert dec_info["points"] < orig_info["points"], (
        f"Decimation did not reduce points: original={orig_info['points']}, decimated={dec_info['points']}"
    )


def test_v_decimate_skip(setup_point_map):
    """Test count-based decimation using the skip parameter."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)

    output_map = "decimated_skip_3"
    tools.v_decimate(input=input_map, output=output_map, skip=3, overwrite=True)

    orig_count = tools.v_info(map=input_map, flags="t", format="json")["points"]
    dec_count = tools.v_info(map=output_map, flags="t", format="json")["points"]

    assert dec_count < orig_count, (
        f"Decimated map has {dec_count} points; expected less than original {orig_count}"
    )
    assert dec_count > 0, f"Decimated map {output_map} has no points"


def test_decimate_offset_and_limit(setup_point_map):
    """Test offset and limit parameters to extract a slice of points."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)
    output_map = "out_offset_limit"

    tools.v_decimate(
        input=input_map, output=output_map, offset=2, limit=3, overwrite=True
    )

    count = tools.v_info(map=output_map, flags="t", format="json")["points"]

    assert count == 3, f"Expected 3 points after offset and limit, got {count}"


def test_decimate_zrange(setup_point_map):
    """Test filtering points by z-range during decimation."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)
    output_map = "out_zrange"

    tools.v_decimate(
        input=input_map, output=output_map, zrange=(3.0, 5.0), overwrite=True
    )

    count = tools.v_info(map=output_map, flags="t", format="json")["points"]

    assert count == 3, f"Expected 3 points in zrange 3.0-5.0, got {count}"


def test_decimate_no_topology(setup_point_map):
    """Test decimation with no topology build (-b flag) and grid-based decimation."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)
    output_map = "out_no_topo"

    tools.v_decimate(
        input=input_map,
        output=output_map,
        flags="bg",  # -b no topology, -g grid-based
        cell_limit=3,
        overwrite=True,
    )

    info = tools.v_info(map=output_map, format="json")

    assert info["points"] > 0, "No points found after decimation with no topology"
    assert info["nodes"] == 0, "Topology nodes found despite no topology flag (-b)"


def test_decimate_no_table(setup_point_map):
    """Test decimation with no attribute table (-t flag) and grid-based decimation."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)
    output_map = "out_no_table"

    tools.v_decimate(
        input=input_map,
        output=output_map,
        flags="tg",  # -t no table, -g grid-based
        cell_limit=1,
        overwrite=True,
    )

    info = tools.v_info(map=output_map, format="json")

    assert info["points"] > 0, "No points found after decimation with no table"


def test_v_decimate_with_cats(setup_point_map):
    """Test decimation filtered by category values."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)
    output_map = "decimated_cat2"

    tools.v_decimate(
        input=input_map, output=output_map, cats="2", layer=1, overwrite=True
    )

    orig_info = tools.v_info(map=input_map, flags="t", format="json")
    dec_info = tools.v_info(map=output_map, flags="t", format="json")

    assert dec_info["points"] <= orig_info["points"], (
        f"Decimated points ({dec_info['points']}) should be less or equal to original points ({orig_info['points']})"
    )


def test_v_decimate_limit_and_offset(setup_point_map):
    """Test count-based decimation with limit and offset parameters."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)
    output_map = "decimated_limited"

    tools.v_decimate(
        input=input_map, output=output_map, limit=5, offset=2, overwrite=True
    )

    dec_info = tools.v_info(map=output_map, flags="t", format="json")

    assert dec_info["points"] > 0, f"Decimated map {output_map} has no points"


def test_v_decimate_flag_x(setup_point_map):
    """Test decimation with flags 'xg' - store only coordinates, grid based decimation."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)
    output_map = "decimated_flag_x"

    tools.v_decimate(
        input=input_map, output=output_map, flags="xg", cell_limit=1, overwrite=True
    )

    info = tools.v_info(map=output_map, format="json")

    assert info["points"] > 0, "No points found after decimation with flags 'xg'"


def test_v_decimate_flag_b_no_topology(setup_point_map):
    """Test decimation with no topology (-b flag) and grid-based decimation."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)
    output_map = "decimated_flag_b"

    tools.v_decimate(
        input=input_map, output=output_map, flags="bg", cell_limit=2, overwrite=True
    )

    info = tools.v_info(map=output_map, format="json")

    assert info["points"] > 0, "No points found after decimation with flags 'bg'"
    assert info["nodes"] == 0, "Topology nodes found despite no topology flag (-b)"


def test_v_decimate_missing_required_flags_raises(setup_point_map):
    """Test that v.decimate raises error when -g flag is given without required companion flags or parameters."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)

    with pytest.raises(CalledModuleError, match="requires at least one of"):
        tools.v_decimate(
            input=input_map,
            output="fail_test",
            flags="g",
            overwrite=True,
        )


def test_v_decimate_flags_cg(setup_point_map):
    """Test decimation with flags 'cg' - category and grid based."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)
    output_map = "decimated_cg"

    tools.v_decimate(
        input=input_map, output=output_map, flags="cg", overwrite=True, layer=1
    )

    info = tools.v_info(map=output_map, format="json")

    assert info["points"] > 0, "No points found after decimation with flags 'cg'"


def test_v_decimate_with_layer_selection(setup_point_map):
    """Test count-based decimation (skip) on a specific layer."""
    input_map = setup_point_map.input_map
    session = setup_point_map.session
    tools = Tools(session=session)
    output_map = "decimated_layer"

    tools.v_decimate(
        input=input_map, output=output_map, layer=1, skip=2, overwrite=True
    )

    info = tools.v_info(map=output_map, format="json")

    assert info["points"] > 0, "No points found after decimation on specific layer"
