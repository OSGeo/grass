"""Parallel-correctness tests for r.proj.

r.proj takes a nprocs= option that sets the compute thread count, so each run
below passes nprocs= for that run, and the fallback test also sets
R_PROJ_FORCE_TILECACHE on its own env copy. Nothing shared is mutated, so the
serial and parallel runs of a test cannot leak thread or path state into each
other.

The baseline is the module's own single-thread run at nprocs=1 rather than an
external serial binary. The question these tests answer is whether adding
threads, or taking the tile-cache fallback, changes the output of this same
binary. That comparison is exact and reproducible in CI where an external
oracle would not be.

Nearest is asserted bitwise with an absolute diff max of zero. Bilinear is
asserted bitwise too because each output cell is interpolated independently in
a fixed operation order, so threading does not reorder its arithmetic. The
epsilon-1e-6 fallback from the proposal may be invoked only on an actual CI
reordering failure, naming the platform that showed it.
"""

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
    """Set the output region to r.proj's suggested bounds for the input.

    r.proj -g prints the whole region as space-separated key=value pairs on
    one line, so split on whitespace first, then on '='."""
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
    """Assert a and b are bitwise identical: equal counts, equal null
    pattern, and a zero-valued absolute difference over a non-empty map."""
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
    """Bilinear: parallel output must equal the serial output bitwise.

    A dispatch-liveness guard runs first: bilinear must differ from nearest
    on the same frame, so a silent fallback to nearest cannot make the
    identity assert pass vacuously (the Bug A regression guard)."""
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
    """Nearest with a constrained memory cap (memory=5, OMP=4) must match the
    default-memory serial run bitwise, exercising band sizing at a small cap."""
    session = session_3857
    base = _env(session)
    _set_region_from_source(base, INPUT_MID, "nearest")

    _project(base, INPUT_MID, "mem_serial", "nearest", nprocs=1)
    _project(base, INPUT_MID, "mem_banded", "nearest", nprocs=4, memory=5)
    _assert_bitwise_identical(base, "mem_serial", "mem_banded", "mem_diff")


def test_pole_nearest_parallel_matches_serial(session_pole):
    """Nearest into a frame centered on the north pole: the warped access
    pattern near the pole must still give bitwise-identical parallel output."""
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
    """The forced serial tile-cache path must equal the banded parallel path
    bitwise. R_PROJ_FORCE_TILECACHE=1 takes the readcell tile-cache route
    (a different algorithm), so this is a cross-path check, not just a
    thread-count one."""
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
