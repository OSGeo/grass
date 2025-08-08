import pytest
import io
import grass.script as gs
from grass.tools import Tools
from grass.exceptions import CalledModuleError


@pytest.fixture
def setup_point_map(tmp_path):
    """
    Create a temporary GRASS GIS project and load a vector point map with 3D points.
    Points are clustered to allow meaningful decimation tests, especially grid-based with z-difference.
    """
    mapname = "test_points"
    points = [
        (100, 100, 1.0),
        (100.05, 100.05, 1.1),
        (100.1, 100.1, 1.2),
        (101, 100, 2.0),
        (102, 100, 3.0),
        (103, 100, 4.0),
        (104, 100, 5.0),
        (105, 100, 6.0),
        (106, 100, 7.0),
    ]
    coords = "\n".join(f"{x}|{y}|{z}" for x, y, z in points)

    project = tmp_path / "grassdata"
    gs.create_project(project)

    with gs.setup.init(project) as session:
        tools = Tools(session=session)
        # Set computational region covering points with 1m resolution
        tools.g_region(n=110, s=90, e=110, w=90, res=1)

        # Import points as 3D vector map (z flag and z=3 column)
        tools.v_in_ascii(
            input=io.StringIO(coords),
            output=mapname,
            separator="|",
            flags="z",
            z=3,
            overwrite=True,
        )

        yield mapname, session


def _get_point_count(mapname, tools):
    """Helper to get number of points in a vector map."""
    info = tools.v_info(map=mapname, flags="t", format="json")
    return info["points"]


@pytest.mark.parametrize("preserve_value", [2, 4])
def test_v_decimate_preserve(setup_point_map, preserve_value):
    """Test count-based decimation using preserve parameter."""
    input_map, session = setup_point_map
    tools = Tools(session=session)

    output_map = f"decimated_preserve_{preserve_value}"

    tools.v_decimate(
        input=input_map, output=output_map, preserve=preserve_value, overwrite=True
    )

    result = tools.g_list(type="vector", pattern=output_map, format="json")
    names = [entry["name"] for entry in result]
    assert output_map in names, f"Decimated map {output_map} not found in vector list"


def test_v_decimate_skip(setup_point_map):
    """Test count-based decimation using skip parameter."""
    input_map, session = setup_point_map
    tools = Tools(session=session)

    output_map = "decimated_skip_3"
    tools.v_decimate(input=input_map, output=output_map, skip=3, overwrite=True)

    result = tools.g_list(type="vector", pattern=output_map, format="json")
    assert any(v["name"] == output_map for v in result), (
        f"Decimated map {output_map} not found"
    )


def test_decimate_offset_and_limit(setup_point_map):
    """Test offset and limit parameters to extract a slice of points."""
    input_map, session = setup_point_map
    tools = Tools(session=session)
    output_map = "out_offset_limit"

    tools.v_decimate(
        input=input_map, output=output_map, offset=2, limit=3, overwrite=True
    )

    count = _get_point_count(output_map, tools)
    assert count == 3, f"Expected 3 points after offset and limit, got {count}"


def test_decimate_zrange(setup_point_map):
    """Test filtering points by z-range during decimation."""
    input_map, session = setup_point_map
    tools = Tools(session=session)
    output_map = "out_zrange"

    tools.v_decimate(
        input=input_map, output=output_map, zrange=(3.0, 5.0), overwrite=True
    )

    count = _get_point_count(output_map, tools)
    assert count == 3, f"Expected 3 points in zrange 3.0-5.0, got {count}"


def test_decimate_no_topology(setup_point_map):
    """Test decimation with no topology build (-b flag) and grid-based decimation."""
    input_map, session = setup_point_map
    tools = Tools(session=session)
    output_map = "out_no_topo"

    tools.v_decimate(
        input=input_map,
        output=output_map,
        flags="bg",  # -b no topology, -g grid based
        cell_limit=3,
        overwrite=True,
    )

    info = tools.v_info(map=output_map, format="json")
    assert info["points"] > 0, "No points found after decimation with no topology"
    assert info["nodes"] == 0, "Topology nodes found despite no topology flag (-b)"


def test_decimate_no_table(setup_point_map):
    """Test decimation with no attribute table (-t flag) and grid-based decimation."""
    input_map, session = setup_point_map
    tools = Tools(session=session)
    output_map = "out_no_table"

    tools.v_decimate(
        input=input_map,
        output=output_map,
        flags="tg",  # -t no table, -g grid based
        cell_limit=1,
        overwrite=True,
    )

    info = tools.v_info(map=output_map, format="json")
    assert info["points"] > 0, "No points found after decimation with no table"


def test_v_decimate_with_cats(setup_point_map):
    """Test decimation filtered by category values."""
    input_map, session = setup_point_map
    tools = Tools(session=session)
    output_map = "decimated_cat2"

    tools.v_decimate(
        input=input_map, output=output_map, cats="2", layer=1, overwrite=True
    )

    result = tools.g_list(type="vector", pattern=output_map, format="json")
    assert any(v["name"] == output_map for v in result), (
        f"Decimated map {output_map} not found"
    )


def test_v_decimate_limit_and_offset(setup_point_map):
    """Test count-based decimation with limit and offset parameters."""
    input_map, session = setup_point_map
    tools = Tools(session=session)
    output_map = "decimated_limited"

    tools.v_decimate(
        input=input_map, output=output_map, limit=5, offset=2, overwrite=True
    )

    result = tools.g_list(type="vector", pattern=output_map, format="json")
    assert any(v["name"] == output_map for v in result), (
        f"Decimated map {output_map} not found"
    )


def test_v_decimate_flag_x(setup_point_map):
    """Test decimation with flags 'xg' - store only coordinates, grid based decimation."""
    input_map, session = setup_point_map
    tools = Tools(session=session)
    output_map = "decimated_flag_x"

    tools.v_decimate(
        input=input_map, output=output_map, flags="xg", cell_limit=1, overwrite=True
    )

    info = tools.v_info(map=output_map, format="json")
    assert info["points"] > 0, "No points found after decimation with flags 'xg'"


def test_v_decimate_flag_b_no_topology(setup_point_map):
    """Test decimation with no topology (-b flag) and grid-based decimation."""
    input_map, session = setup_point_map
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
    input_map, session = setup_point_map
    tools = Tools(session=session)

    with pytest.raises(CalledModuleError) as excinfo:
        tools.v_decimate(
            input=input_map,
            output="fail_test",
            flags="g",  # -g requires at least one of -f, -c, cell_limit, -z, zdiff
            overwrite=True,
        )
    assert "requires at least one of" in str(excinfo.value), (
        "Expected error about missing required flags or parameters"
    )


def test_v_decimate_flags_cg(setup_point_map):
    """Test decimation with flags 'cg' - category and grid based."""
    input_map, session = setup_point_map
    tools = Tools(session=session)
    output_map = "decimated_cg"

    tools.v_decimate(
        input=input_map,
        output=output_map,
        flags="cg",
        overwrite=True,
        layer=1,
    )

    info = tools.v_info(map=output_map, format="json")
    assert info["points"] > 0, "No points found after decimation with flags 'cg'"


def test_v_decimate_with_layer_selection(setup_point_map):
    """Test count-based decimation (skip) on a specific layer."""
    input_map, session = setup_point_map
    tools = Tools(session=session)
    output_map = "decimated_layer"

    tools.v_decimate(
        input=input_map, output=output_map, layer=1, skip=2, overwrite=True
    )
    info = tools.v_info(map=output_map, format="json")
    assert info["points"] > 0, "No points found after decimation on specific layer"


def test_v_decimate_with_all_layers(setup_point_map):
    """Test count-based decimation (skip) on all layers (layer=-1)."""
    input_map, session = setup_point_map
    tools = Tools(session=session)
    output_map = "decimated_all_layers"

    tools.v_decimate(
        input=input_map,
        output=output_map,
        layer=-1,  # All layers
        skip=2,
        overwrite=True,
    )
    info = tools.v_info(map=output_map, format="json")
    assert info["points"] > 0, "No points found after decimation on all layers"
