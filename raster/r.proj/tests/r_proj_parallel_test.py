"""Parallel correctness tests for r.proj. Each test compares the
module's own nprocs=1 run against a multithreaded run, and the fallback
test forces the tile cache path."""

import grass.script as gs

# Mirror of the names created in conftest.py.
SRC_PROJECT = "src4326"
INPUT_MID = "input_mid"
INPUT_POLAR = "input_polar"


def _env(session, **overrides):
    """Session env copy with per-run overrides; never mutates the original."""
    env = dict(session.env)
    for key, value in overrides.items():
        env[key] = str(value)
    return env


def _set_region_from_source(env, input_raster, method):
    """Set the output region to r.proj's suggested bounds for the input."""
    text = gs.read_command(
        "r.proj",
        project=SRC_PROJECT,
        mapset="PERMANENT",
        input=input_raster,
        method=method,
        flags="g",
        env=env,
    )
    region = dict(token.split("=") for token in text.split())
    gs.run_command(
        "g.region",
        n=region["n"],
        s=region["s"],
        e=region["e"],
        w=region["w"],
        rows=region["rows"],
        cols=region["cols"],
        env=env,
    )


def _project(env, input_raster, output, method, **extra):
    gs.run_command(
        "r.proj",
        project=SRC_PROJECT,
        mapset="PERMANENT",
        input=input_raster,
        output=output,
        method=method,
        overwrite=True,
        quiet=True,
        env=env,
        **extra,
    )


def _stats(env, raster):
    return gs.parse_command("r.univar", map=raster, flags="g", env=env)


def _assert_bitwise_identical(env, a, b, diff):
    """Check a and b are bitwise identical and have the same null cells."""
    gs.run_command(
        "r.mapcalc", expression=f"{diff} = abs({a} - {b})", overwrite=True, env=env
    )
    sa = _stats(env, a)
    sb = _stats(env, b)
    sd = _stats(env, diff)
    assert int(sa["n"]) > 0, "output is empty; the comparison would be vacuous"
    assert int(sa["n"]) == int(sb["n"])
    assert int(sa["null_cells"]) == int(sb["null_cells"])
    assert float(sd["max"]) == 0.0


def test_bilinear_parallel_matches_serial(session_3857):
    """Bilinear method needs to match serial bitwise. It first checks that
    bilinear and nearest outputs differ, so a silent fallback to nearest
    cannot happen."""
    session = session_3857
    base = _env(session)
    _set_region_from_source(base, INPUT_MID, "bilinear")

    _project(base, INPUT_MID, "bilin_serial", "bilinear", nprocs=1)
    _project(base, INPUT_MID, "nearest_ref", "nearest", nprocs=1)
    gs.run_command(
        "r.mapcalc",
        expression="dispatch_live = abs(bilin_serial - nearest_ref)",
        overwrite=True,
        env=base,
    )
    assert float(_stats(base, "dispatch_live")["max"]) > 0, (
        "bilinear output equals nearest; dispatch may have fallen back"
    )

    _project(base, INPUT_MID, "bilin_parallel", "bilinear", nprocs=4)
    _assert_bitwise_identical(base, "bilin_serial", "bilin_parallel", "bilin_diff")


def test_nearest_memory_banding(session_3857):
    """The nearest method at a small memory cap (memory=5, nprocs=4) has to
    match the default memory serial run bitwise."""
    session = session_3857
    base = _env(session)
    _set_region_from_source(base, INPUT_MID, "nearest")

    _project(base, INPUT_MID, "mem_serial", "nearest", nprocs=1)
    _project(base, INPUT_MID, "mem_banded", "nearest", nprocs=4, memory=5)
    _assert_bitwise_identical(base, "mem_serial", "mem_banded", "mem_diff")


def test_pole_nearest_parallel_matches_serial(session_pole):
    """Nearest at the north pole matches serial bitwise."""
    session = session_pole
    base = _env(session)
    # Fixed 1200 km box centered on the pole (EPSG:3413 meters), 50x50.
    gs.run_command(
        "g.region",
        n=600000,
        s=-600000,
        e=600000,
        w=-600000,
        rows=50,
        cols=50,
        env=base,
    )

    _project(base, INPUT_POLAR, "pole_serial", "nearest", nprocs=1)
    _project(base, INPUT_POLAR, "pole_parallel", "nearest", nprocs=4)
    _assert_bitwise_identical(base, "pole_serial", "pole_parallel", "pole_diff")


def test_forced_fallback_matches_banded(session_3857):
    """Forcing the tile cache with R_PROJ_FORCE_TILECACHE=1 gives the same
    output as the banded path."""
    session = session_3857
    base = _env(session)
    _set_region_from_source(base, INPUT_MID, "nearest")

    _project(
        _env(session, R_PROJ_FORCE_TILECACHE=1),
        INPUT_MID,
        "fallback_tilecache",
        "nearest",
        nprocs=1,
    )
    _project(base, INPUT_MID, "banded", "nearest", nprocs=4)
    _assert_bitwise_identical(base, "fallback_tilecache", "banded", "fallback_diff")


def _project_capture(env, input_raster, output, method, **extra):
    """Run r.proj and return the messages it writes to stderr."""
    env = dict(env, GRASS_VERBOSE="3")
    proc = gs.start_command(
        "r.proj",
        project=SRC_PROJECT,
        mapset="PERMANENT",
        input=input_raster,
        output=output,
        method=method,
        overwrite=True,
        env=env,
        stderr=gs.PIPE,
        **extra,
    )
    stderr = proc.communicate()[1]
    if isinstance(stderr, bytes):
        stderr = stderr.decode()
    assert proc.returncode == 0, stderr
    return stderr


def test_forced_reload_matches_serial(session_3857):
    """FG_SHRINK_SPAN shrinks each band so the reload runs, and the reloaded run
    still matches the nprocs=1 run bitwise."""
    session = session_3857
    base = _env(session)
    _set_region_from_source(base, INPUT_MID, "nearest")

    _project(base, INPUT_MID, "reload_serial", "nearest", nprocs=1)

    shrunk = _project_capture(
        _env(session, FG_SHRINK_SPAN=2),
        INPUT_MID,
        "reload_shrunk",
        "nearest",
        nprocs=4,
    )
    # The shrunk strips leave out rows the projection needs, so r.proj reloads
    # and logs it here. Without this marker the run never hit the reload path.
    assert "Reloading band strip" in shrunk

    # The full-size strips already cover every needed row, so this run never
    # reloads. That is what ties the marker above to the shrink.
    unshrunk = _project_capture(base, INPUT_MID, "reload_unshrunk", "nearest", nprocs=4)
    assert "Reloading band strip" not in unshrunk

    _assert_bitwise_identical(base, "reload_serial", "reload_shrunk", "reload_diff")
