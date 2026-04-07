import grass.script as gs
from grass.tools import Tools
import pytest
from grass.jupyter.utils import (
    get_region,
    get_location_proj_string,
    reproject_region,
    set_target_region,
)


def test_get_region(session_with_data):
    """Check that get_region returns a dictionary with region bounds"""
    region = get_region(env=session_with_data.env)
    assert isinstance(region, dict)
    assert "north" in region
    assert "south" in region
    assert "east" in region
    assert "west" in region


def test_get_location_proj_string(tmp_path):
    """Test that get_location_proj_string returns the projection of the environment in PROJ.4 format."""
    project = tmp_path / "test_project_proj"
    gs.create_project(project)
    with gs.setup.init(project):
        gs.run_command("g.proj", flags="c", epsg="4326")
        projection = get_location_proj_string()
        assert "+proj=" in projection


def test_reproject_region(session_with_data):
    """Check that a region is correctly reprojected between two projections"""
    reg = {
        "north": 100000.0,
        "south": 0.0,
        "east": 100000.0,
        "west": 0.0,
        "rows": 10,
        "cols": 10,
    }
    p_in = "+proj=merc +a=6378137 +b=6378137 +units=m +type=crs"
    p_out = "+proj=longlat +datum=WGS84 +type=crs"
    res = reproject_region(reg, p_in, p_out, env=session_with_data.env)
    assert isinstance(res, dict)
    assert "north" in res
    assert res["north"] != reg["north"]
    assert res["east"] != reg["east"]
    assert -90 <= res["north"] <= 90
    assert -180 <= res["east"] <= 180


def test_set_target_region(session_with_data, tmp_path):
    """Check setting a target region in a newly created environment"""
    gisdbase = str(tmp_path)

    base_env = session_with_data.env

    # setup source environment 
    _, src_base_env = gs.create_environment(gisdbase, "src_loc", "PERMANENT")
    gs.create_project(gisdbase, "src_loc", epsg="3857", overwrite=True)

    src_env = base_env.copy()
    src_env["GISRC"] = src_base_env["GISRC"]

    # setup target environment
    _, tgt_base_env = gs.create_environment(gisdbase, "tgt_loc", "PERMANENT")
    gs.create_project(gisdbase, "tgt_loc", epsg="4326", overwrite=True)
    
    tgt_env = base_env.copy()
    tgt_env["GISRC"] = tgt_base_env["GISRC"]

    # Add data to source, then execute function
    Tools(env=src_env).g_region(n=1000, s=0, e=1000, w=0, rows=10, cols=10)
    set_target_region(src_env, tgt_env)

    # Verify
    res = get_region(env=tgt_env)
    assert isinstance(res, dict)
    assert res["north"] == pytest.approx(0.008983, rel=1e-4) 
    assert res["south"] == pytest.approx(0.0, rel=1e-4)
    assert res["east"] == pytest.approx(0.008983, rel=1e-4)
    assert res["west"] == pytest.approx(0.0, rel=1e-4)
