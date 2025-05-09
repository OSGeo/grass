import grass.script as gs


def test_region_manager_with_g_region(session_2x2):
    """Test RegionManager with g.region call."""
    assert "WIND_OVERRIDE" not in session_2x2.env

    with gs.RegionManager(env=session_2x2.env) as manager:
        assert "WIND_OVERRIDE" in session_2x2.env
        assert session_2x2.env["WIND_OVERRIDE"] == manager.region_name
        region = gs.region(env=session_2x2.env)
        assert region["rows"] == 2
        assert region["cols"] == 2

        gs.run_command("g.region", n=10, s=0, e=10, w=0, res=1, env=session_2x2.env)
        region = gs.region(env=session_2x2.env)
        assert region["rows"] == 10
        assert region["cols"] == 10

        manager.set_region(n=6, s=0, e=6, w=0, res=1)
        region = gs.region(env=session_2x2.env)
        assert region["rows"] == 6
        assert region["cols"] == 6

    assert "WIND_OVERRIDE" not in session_2x2.env
    region = gs.region(env=session_2x2.env)
    assert region["rows"] == 2
    assert region["cols"] == 2


def test_region_manager_with_region_parameters(session_2x2):
    """Test RegionManager with input region parameters."""
    assert "WIND_OVERRIDE" not in session_2x2.env

    with gs.RegionManager(n=10, s=0, e=10, w=0, res=1, env=session_2x2.env) as manager:
        assert "WIND_OVERRIDE" in session_2x2.env
        assert session_2x2.env["WIND_OVERRIDE"] == manager.region_name
        region = gs.region(env=session_2x2.env)
        assert region["rows"] == 10
        assert region["cols"] == 10

        gs.run_command("g.region", n=5, s=0, e=5, w=0, res=1, env=session_2x2.env)
        region = gs.region(env=session_2x2.env)
        assert region["rows"] == 5
        assert region["cols"] == 5

        manager.set_region(n=6, s=0, e=6, w=0, res=1)
        region = gs.region(env=session_2x2.env)
        assert region["rows"] == 6
        assert region["cols"] == 6

    assert "WIND_OVERRIDE" not in session_2x2.env
    region = gs.region(env=session_2x2.env)
    assert region["rows"] == 2
    assert region["cols"] == 2


def test_region_manager_env(session_2x2):
    """Test RegionManagerEnv simple use case."""
    assert "GRASS_REGION" not in session_2x2.env

    with gs.RegionManagerEnv(env=session_2x2.env) as manager:
        assert "GRASS_REGION" in session_2x2.env
        region = gs.region(env=session_2x2.env)
        assert region["rows"] == 2
        assert region["cols"] == 2

        manager.set_region(n=5, s=0, e=5, w=0, res=1)
        region = gs.region(env=session_2x2.env)
        assert region["rows"] == 5
        assert region["cols"] == 5

    assert "GRASS_REGION" not in session_2x2.env
    region = gs.region(env=session_2x2.env)
    assert region["rows"] == 2
    assert region["cols"] == 2


def test_region_manager_env_problem_with_g_region(session_2x2):
    """Test RegionManagerEnv with region parameters and g.region call."""
    assert "GRASS_REGION" not in session_2x2.env

    with gs.RegionManagerEnv(
        n=10, s=0, e=10, w=0, res=1, env=session_2x2.env
    ) as manager:
        assert "GRASS_REGION" in session_2x2.env
        region = gs.region(env=session_2x2.env)
        assert region["rows"] == 10
        assert region["cols"] == 10

        # g.region will not change GRASS_REGION (but reads from it)
        gs.run_command("g.region", res=2, env=session_2x2.env)
        region = gs.region(env=session_2x2.env)
        assert region["rows"] == 10
        assert region["cols"] == 10

        manager.set_region(n=2, s=0, e=2, w=0, res=1)
        region = gs.region(env=session_2x2.env)
        assert region["rows"] == 2
        assert region["cols"] == 2

    assert "GRASS_REGION" not in session_2x2.env
    # region changed by g.region
    region = gs.region(env=session_2x2.env)
    assert region["rows"] == 5
    assert region["cols"] == 5
