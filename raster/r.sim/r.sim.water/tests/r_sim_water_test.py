# SPDX-License-Identifier: GPL-2.0-or-later

"""Minimal analytical tests for r.sim.water (SIMWE).

Tests use small, hand-crafted domains where expected output can be reasoned
about from first principles rather than from pre-computed reference values.
"""

import io
import os
import pathlib

import numpy as np
import pytest

import grass.script as gs
from grass.experimental.mapset import TemporaryMapsetSession
from grass.tools import Tools

# Fixed seed and single thread make results fully deterministic.
SEED = 42
NPROCS = 1

# With default walkers and duration=2, small domains reach
# near-steady state in ~0.2 s per run while keeping Monte Carlo noise low
# enough for structural assertions.
DURATION = 2  # minutes; sufficient for near-steady state on a small domain
RAIN = 100  # mm/hr; generous signal-to-noise ratio on a small domain

# Near-zero diffusion makes walkers move only downslope, which isolates the
# effect of the time step in the mintimestep tests.
NO_DIFFUSION = {"diffusion_coeff": 0.001}


def run_sim(session, *, random_seed=SEED, **kwargs):
    """Run r.sim.water on the session's terrain; return depth as ndarray.

    Assumes the session already contains rasters named elevation, dx, and dy
    (as created by a fixture or inline setup). Additional keyword arguments
    are passed directly to r.sim.water (e.g., rain_value, man_value, rain, man,
    nwalkers).
    """
    defaults = {
        "rain_value": RAIN,
        "infil_value": 0,
        "man_value": 0.1,
        "nprocs": NPROCS,
        "duration": DURATION,
    }
    defaults.update(kwargs)
    # A value of None removes a default, for example rain_value when a rain
    # raster is given.
    defaults = {k: v for k, v in defaults.items() if v is not None}
    tools = Tools(session=session)
    return tools.r_sim_water(
        elevation="elevation",
        dx="dx",
        dy="dy",
        depth=np.array,
        random_seed=random_seed,
        **defaults,
    )


@pytest.fixture(scope="module")
def simwe_session(tmp_path_factory):
    """Module-scoped GRASS session for SIMWE tests with shared base terrain.

    Creates a persistent project and initializes the GRASS runtime environment
    in the PERMANENT mapset, where the base terrain (elevation, dx, dy) is set up.
    This session persists for the entire test module. All function-scoped fixtures
    create temporary mapsets within this session and automatically see the base
    terrain from PERMANENT.
    """
    tmp_path = tmp_path_factory.mktemp("simwe_module")
    project = tmp_path / "simwe"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        # Set up base 1x5 eastward slope terrain in PERMANENT mapset.
        # Use -s flag to save region as default (inherited by temporary mapsets).
        tools.g_region(w=0, e=5, s=0, n=1, res=1, flags="s")
        tools.r_mapcalc(expression="elevation = 6 - col()")
        tools.r_mapcalc(expression="dx = 1.0")
        tools.r_mapcalc(expression="dy = 0.0")
        yield session


@pytest.fixture
def east_slope_session(simwe_session):
    """Session with a 1-row x 5-column eastward slope, ready for r.sim.water.

    Uses the module-scoped simwe_session and creates a new temporary mapset
    for each test. The base terrain (elevation, dx, dy) and computational region
    are inherited from the PERMANENT mapset created in the module scope.

    Terrain (GRASS col() is 1-indexed from west):
      elevation = 6 - col()  -> [5, 4, 3, 2, 1] west to east
      dx = 1.0               (unit slope vector pointing east)
      dy = 0.0               (no north-south component)

    Water flows east and accumulates toward the eastern boundary.
    """
    with TemporaryMapsetSession(env=simwe_session.env) as session:
        yield session


@pytest.fixture(scope="module")
def long_slope_project(tmp_path_factory):
    """Module-scoped project with a 200-cell, 10 m-resolution eastward slope.

    Several time-series and walker-output tests need a domain large enough
    that walkers stay inside it for the full simulation. The terrain lives
    in PERMANENT; each test gets its own temporary mapset via
    long_slope_session so named outputs do not collide.
    """
    project = tmp_path_factory.mktemp("simwe_long") / "simwe"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(w=0, e=2000, s=0, n=10, res=10, flags="s")
        # dx and dy set the slope; 0.1 matches the 1 m drop per 10 m cell.
        tools.r_mapcalc(expression="elevation = 201 - col()")
        tools.r_mapcalc(expression="dx = 0.1")
        tools.r_mapcalc(expression="dy = 0.0")
        yield session


@pytest.fixture
def long_slope_session(long_slope_project):
    with TemporaryMapsetSession(env=long_slope_project.env) as session:
        yield session


@pytest.fixture(scope="module")
def diffusion_project(tmp_path_factory):
    """Module-scoped project with a 200-cell, 1 m-resolution eastward slope.

    Shared by the hmax/halpha/hbeta diffusion tests, which need high rain
    and roughness on a long 1D domain so depths exceed the hmax threshold.
    """
    project = tmp_path_factory.mktemp("simwe_diffusion") / "simwe"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(w=0, e=200, s=0, n=1, res=1, flags="s")
        tools.r_mapcalc(expression="elevation = 201 - col()")
        tools.r_mapcalc(expression="dx = 1.0")
        tools.r_mapcalc(expression="dy = 0.0")
        yield session


@pytest.fixture
def diffusion_session(diffusion_project):
    with TemporaryMapsetSession(env=diffusion_project.env) as session:
        yield session


@pytest.fixture(scope="module")
def diffusion_low_hmax_depth(diffusion_project):
    """Depth for the low-hmax (high-diffusion) reference run.

    The hmax=0.001 run is the shared reference for both the hmax and hbeta
    tests (hbeta=0.5 is the tool default, so test_hbeta's baseline run is
    identical to this one). Computed once and returned read-only so
    consumers cannot mutate the shared array.

    The diffusion inequalities are structural (lowering hmax or raising
    halpha roughly halves total depth), not Monte Carlo convergence limited:
    across 20 seeds the relative margins held at ~0.52 (hmax) and ~0.46
    (halpha) with zero failures at every walker count from 2000 to 400000.
    nwalkers=40000 keeps an order-of-magnitude safety factor above where
    the margins are noise-driven.
    """
    with TemporaryMapsetSession(env=diffusion_project.env) as session:
        # Copy the result while the mapset holding its backing file exists.
        depth = np.array(
            run_sim(session, rain_value=1000, man_value=0.5, hmax=0.001, nwalkers=40000)
        )
    depth.flags.writeable = False
    return depth


def test_no_rain_produces_no_depth(east_slope_session):
    """Zero rainfall must yield zero water depth everywhere.

    With no water input there can be no runoff. This holds exactly
    regardless of seed or number of walkers: no particles are created.
    """
    depth = run_sim(east_slope_session, rain_value=0)
    assert np.all(depth == 0), f"Expected all-zero depth with zero rain:\n{depth}"


def test_rain_produces_positive_depth(east_slope_session):
    """Rainfall on a slope must create positive water depth somewhere.

    See test_north_slope_rain_produces_positive_depth for the north-south variant.
    """
    depth = run_sim(east_slope_session)
    assert np.sum(depth) > 0, "Expected positive total depth with rainfall on a slope"


def test_more_rain_gives_more_depth(east_slope_session):
    """Doubling rainfall must increase total water depth.

    Manning's kinematic wave gives h proportional to R^(3/5), so doubling R
    increases depth by a factor of 2^(3/5).
    """
    sum_low = float(np.sum(run_sim(east_slope_session, rain_value=50)))
    sum_high = float(np.sum(run_sim(east_slope_session, rain_value=100)))
    assert sum_high > sum_low, (
        f"Total depth should increase with rainfall rate: "
        f"sum(rain=50)={sum_low:.3e}, sum(rain=100)={sum_high:.3e}"
    )
    assert sum_high / sum_low == pytest.approx(2 ** (3 / 5), rel=0.3)


def test_higher_manning_gives_more_depth(east_slope_session):
    """Rougher surface (higher Manning's n) must increase water depth."""
    sum_smooth = float(np.sum(run_sim(east_slope_session, man_value=0.05)))
    sum_rough = float(np.sum(run_sim(east_slope_session, man_value=0.5)))
    assert sum_rough > sum_smooth, (
        f"Total depth should increase with Manning's n: "
        f"sum(n=0.05)={sum_smooth:.3e}, sum(n=0.5)={sum_rough:.3e}"
    )


def test_infiltration_reduces_depth(east_slope_session):
    """Adding overland-flow infiltration must reduce water depth.

    Infiltration removes water from the flowing sheet, decreasing both
    depth and discharge.
    """
    sum_no_infil = float(
        np.sum(run_sim(east_slope_session, rain_value=RAIN, infil_value=0))
    )
    sum_with_infil = float(
        np.sum(run_sim(east_slope_session, rain_value=RAIN, infil_value=RAIN // 2))
    )
    assert sum_no_infil > sum_with_infil, (
        f"Total depth should decrease when infiltration is added: "
        f"sum(no infil)={sum_no_infil:.3e}, sum(with infil)={sum_with_infil:.3e}"
    )


def test_depth_increases_downstream(tmp_path):
    """Water must get deeper from cell to cell down a uniform slope.

    By continuity, discharge q(x) = R * x grows linearly with distance x
    from the divide. Manning's kinematic wave then gives h(x) proportional
    to x^(3/5), also increasing downstream.

    Uses a 1-row x 6-column domain. See
    test_north_slope_depth_increases_downstream for the north-south variant.
    """
    project = tmp_path / "simwe"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(w=0, e=6, s=0, n=1, res=1)
        # elevation = 7 - col() gives [6, 5, 4, 3, 2, 1] west to east
        tools.r_mapcalc(expression="elevation = 7 - col()")
        tools.r_mapcalc(expression="dx = 1.0")
        tools.r_mapcalc(expression="dy = 0.0")

        # With the default walkers a single cell can be drier than the one
        # above it; across 13 seeds at 2000 walkers every cell exceeded the
        # one above it by at least 4%.
        flat = run_sim(session, nwalkers=2000).flatten()
        assert np.all(np.diff(flat) > 0), (
            f"Depth should increase cell by cell downstream: {flat}"
        )


def test_steeper_slope_gives_less_depth(tmp_path):
    """A steeper slope must produce shallower water depth for the same rainfall.

    Manning's kinematic wave gives h proportional to S^(-3/10), so doubling
    the slope magnitude S reduces depth by a factor of 2^(-3/10).

    Elevation and dx are kept consistent: a 1 m/cell drop uses dx=1, and a
    2 m/cell drop uses dx=2, matching the slope magnitude S in Manning's
    equation.
    """

    def setup_and_run(sub_path, ele_expr, dx_value):
        project = sub_path / "simwe"
        gs.create_project(project)
        with gs.setup.init(project, env=os.environ.copy()) as session:
            tools = Tools(session=session)
            tools.g_region(w=0, e=5, s=0, n=1, res=1)
            tools.r_mapcalc(expression=f"elevation = {ele_expr}")
            tools.r_mapcalc(expression=f"dx = {dx_value}")
            tools.r_mapcalc(expression="dy = 0.0")
            return run_sim(session)

    # gentle: drops 1 m per cell (elevation 5,4,3,2,1), dx = 1
    sum_gentle = float(np.sum(setup_and_run(tmp_path / "gentle", "6 - col()", 1)))
    # steep: drops 2 m per cell (elevation 9,7,5,3,1), dx = 2
    sum_steep = float(np.sum(setup_and_run(tmp_path / "steep", "11 - 2 * col()", 2)))

    assert sum_gentle > sum_steep, (
        f"Gentler slope depth ({sum_gentle:.3e}) should exceed "
        f"steeper slope depth ({sum_steep:.3e})"
    )
    assert sum_gentle / sum_steep == pytest.approx(2 ** (3 / 10), rel=0.3)


def test_discharge_within_mass_balance_bracket(east_slope_session):
    """Rainfall on a slope must produce discharge consistent with mass balance.

    Discharge is output in m3/s. By steady-state continuity, the total
    discharge summed over the domain cannot exceed the rainfall caught by the
    contributing area: sum(q) <= R * total_contributing_area. On the
    east_slope_session domain (1 row x 5 cols, res=1, elevation = 6 - col()),
    the cell at column x (x = 0,...,4) sits (x + 0.5) m downslope of the divide
    and is 1 m wide, so its contributing area is (x + 0.5) m^2 and the analytic
    steady-state total is analytic_sum = R * sum(x + 0.5).

    This is an order-of-magnitude mass-balance bound, not the exact q = R * x
    relation: on this tiny domain boundary and diffusion effects make the
    simulated total only a fraction (empirically ~0.4-0.7 across seeds) of the
    analytic steady-state total. We therefore bracket the simulated sum between
    analytic_sum/3 and analytic_sum. The lower bound already implies that
    discharge is positive, while the bracket also catches gross unit errors
    (e.g. a mm/hr or l/s confusion would be off by ~1000x).
    """
    rain_rate = RAIN / 1000.0 / 3600.0  # mm/hr -> m/s
    analytic_sum = rain_rate * sum(x + 0.5 for x in range(5))

    tools = Tools(session=east_slope_session)
    discharge = tools.r_sim_water(
        elevation="elevation",
        dx="dx",
        dy="dy",
        discharge=np.array,
        rain_value=RAIN,
        duration=DURATION,
        random_seed=SEED,
        nprocs=NPROCS,
    )
    total = float(np.sum(discharge))
    assert analytic_sum / 3 < total < analytic_sum, (
        f"Total discharge {total:.3e} m3/s should fall within the mass-balance "
        f"bracket ({analytic_sum / 3:.3e}, {analytic_sum:.3e}) m3/s"
    )


def test_results_consistent_across_seeds(east_slope_session):
    """Different random seeds must produce similar total depth.

    The Monte Carlo result converges as nwalkers grows. With enough walkers
    the total depth should be stable across seeds within a few percent.
    """
    seeds = [1, 7, 42, 99, 123]
    sums = [
        float(np.sum(run_sim(east_slope_session, nwalkers=1000, random_seed=s)))
        for s in seeds
    ]
    mean_sum = np.mean(sums)
    for seed, total in zip(seeds, sums, strict=True):
        assert total == pytest.approx(mean_sum, rel=0.1), (
            f"Seed {seed} total depth {total:.3e} deviates from mean {mean_sum:.3e}"
        )


def test_rain_raster_matches_scalar(east_slope_session):
    """A uniform rain raster must produce the same depth as the equivalent scalar."""
    tools = Tools(session=east_slope_session)
    tools.r_mapcalc(expression=f"rain_map = {RAIN}")
    sum_scalar = float(np.sum(run_sim(east_slope_session, rain_value=RAIN)))
    sum_raster = float(
        np.sum(run_sim(east_slope_session, rain="rain_map", rain_value=None))
    )
    assert sum_raster == pytest.approx(sum_scalar, rel=1e-6)


def test_random_seed_flag(east_slope_session):
    """The -s flag must generate a random seed and produce valid, varying output.

    Two runs with -s should both produce positive depth but differ from each
    other because each run gets a different auto-generated seed.

    The auto-generated seed is taken from GRASS_RANDOM_SEED or
    SOURCE_DATE_EPOCH when either is set (e.g., in reproducible builds), which
    would make the two runs identical. They are removed from the environment
    so that the test exercises the time- and PID-based seeding.
    """
    env = {
        key: value
        for key, value in east_slope_session.env.items()
        if key not in {"GRASS_RANDOM_SEED", "SOURCE_DATE_EPOCH"}
    }
    tools = Tools(env=env)
    common = {
        "elevation": "elevation",
        "dx": "dx",
        "dy": "dy",
        "depth": np.array,
        "rain_value": RAIN,
        "man_value": 0.1,
        "duration": DURATION,
        "nprocs": NPROCS,
        "flags": "s",
    }
    depth_a = tools.r_sim_water(**common)
    depth_b = tools.r_sim_water(**common)
    assert np.sum(depth_a) > 0, "Expected positive depth with -s flag (run a)"
    assert np.sum(depth_b) > 0, "Expected positive depth with -s flag (run b)"
    assert not np.array_equal(depth_a, depth_b), (
        "Two runs with -s should produce different results"
    )


def test_error_output_is_zero(east_slope_session):
    """The error output is all zeros while its computation is disabled."""
    tools = Tools(session=east_slope_session)
    error = tools.r_sim_water(
        elevation="elevation",
        dx="dx",
        dy="dy",
        error=np.array,
        rain_value=RAIN,
        man_value=0.1,
        duration=DURATION,
        random_seed=SEED,
        nprocs=NPROCS,
    )
    assert error.shape == (1, 5), f"Expected one error value per cell:\n{error}"
    assert np.all(error == 0), f"Expected all-zero error map:\n{error}"


def test_mintimestep_is_a_floor_and_harmless_within_one_cell_per_step(
    east_slope_session, tmp_path
):
    """mintimestep must only raise the time step, and a small raise is harmless.

    The tool computes its own step, 0.025 s on this slope. A 0.01 s floor is
    below that, so the result must be identical. A 0.1 s floor is above it
    but moves walkers only one cell per step, so depth stays the same within
    noise, and so it does with a 3 s floor on 10 m cells at a slope of 0.1.
    """
    depth_default = run_sim(east_slope_session)
    np.testing.assert_array_equal(
        run_sim(east_slope_session, mintimestep=0.01), depth_default
    )

    # Across 13 seeds the two ratios below stayed within 0.96 and 1.05.
    sum_default = float(np.sum(run_sim(east_slope_session, **NO_DIFFUSION)))
    sum_one_cell = float(
        np.sum(run_sim(east_slope_session, mintimestep=0.1, **NO_DIFFUSION))
    )
    assert sum_one_cell / sum_default == pytest.approx(1, rel=0.1), (
        f"A 0.1 s floor changed depth by a factor of {sum_one_cell / sum_default:.3f}"
    )

    project = tmp_path / "coarse"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(w=0, e=50, s=0, n=10, res=10)
        # dx and dy set the slope; 0.1 matches the 1 m drop per 10 m cell and
        # gives 3.2 m/s, so a 3 s floor moves walkers about one cell per step.
        tools.r_mapcalc(expression="elevation = 6 - col()")
        tools.r_mapcalc(expression="dx = 0.1")
        tools.r_mapcalc(expression="dy = 0.0")
        coarse_default = float(np.sum(run_sim(session, **NO_DIFFUSION)))
        coarse_floor = float(np.sum(run_sim(session, mintimestep=3.0, **NO_DIFFUSION)))
    assert coarse_floor / coarse_default == pytest.approx(1, rel=0.1), (
        f"A 3 s floor on 10 m cells changed depth by a factor of "
        f"{coarse_floor / coarse_default:.3f}"
    )


def test_mintimestep_skipping_cells_inflates_depth_predictably(east_slope_session):
    """A floor that makes walkers skip cells inflates depth by a known factor.

    With a 1 s floor a walker moves 10 m per step and leaves the 5 m domain
    at once. The scheme still credits half of that first step to the cell the
    walker started in, so each cell records half the floor as residence
    time, far more than the water really spends there, and depth grows by a
    factor that follows from the residence times of the default run.
    """
    sum_default = float(np.sum(run_sim(east_slope_session, **NO_DIFFUSION)))

    # At the 10 m/s that dx=1 and n=0.1 give, a walker crosses a 1 m cell in
    # 0.1 s. Cell c (0 at the top) collects half a crossing from its own rain
    # and a full crossing from each of the c cells above it. Depth per cell
    # is that residence time to the power 0.6.
    residence_default = [0.05 + 0.1 * cell for cell in range(5)]
    depth_sum_default = sum(t**0.6 for t in residence_default)
    for floor in (1.0, 2.0):
        predicted = 5 * (floor / 2) ** 0.6 / depth_sum_default
        sum_floor = float(
            np.sum(run_sim(east_slope_session, mintimestep=floor, **NO_DIFFUSION))
        )
        # Across 13 seeds the measured ratio stayed within 5% of the prediction.
        assert sum_floor / sum_default == pytest.approx(predicted, rel=0.1), (
            f"A {floor} s floor gave {sum_floor / sum_default:.3f} times the "
            f"default depth, expected about {predicted:.2f}"
        )


def test_longer_simulation_larger_domain(long_slope_session):
    """A longer duration must increase total water depth on a larger domain.

    On a larger domain with slower drainage, longer simulations accumulate
    more water. Uses a 200-cell domain at 10 m resolution so that walkers
    remain within the domain for the full duration.
    """
    sum_short = float(
        np.sum(run_sim(long_slope_session, rain_value=RAIN, man_value=0.3, duration=5))
    )
    sum_long = float(
        np.sum(run_sim(long_slope_session, rain_value=RAIN, man_value=0.3, duration=20))
    )
    assert sum_long > sum_short, (
        f"Longer simulation should produce more depth: "
        f"sum(5 min)={sum_short:.3e}, sum(20 min)={sum_long:.3e}"
    )
    ratio = sum_long / sum_short
    assert ratio >= 1.05, (
        f"20-min simulation should produce at least 5% more depth than 5-min "
        f"(ratio={ratio:.2f})"
    )


def test_duration_affects_time_series_progression(long_slope_session):
    """A longer duration must create more time-series output maps.

    With output_step=5, duration=10 produces maps at t=5,10 while
    duration=20 produces maps at t=5,10,15,20. The exact count of the longer
    run is not pinned: when the last walkers leave the domain early, which
    varies with seed and platform, the steps after that are not written,
    only the final t=20 map. We assert the robust lower bound (at least
    three maps) and that the longer simulation yields strictly more maps
    than the shorter one.
    """
    tools = Tools(session=long_slope_session)

    tools.r_sim_water(
        elevation="elevation",
        dx="dx",
        dy="dy",
        depth="depth_10min",
        rain_value=RAIN,
        man_value=0.3,
        nwalkers=10000,
        duration=10,
        output_step=5,
        random_seed=SEED,
        nprocs=NPROCS,
        flags="t",
    )

    tools.r_sim_water(
        elevation="elevation",
        dx="dx",
        dy="dy",
        depth="depth_20min",
        rain_value=RAIN,
        man_value=0.3,
        nwalkers=10000,
        duration=20,
        output_step=5,
        random_seed=SEED,
        nprocs=NPROCS,
        flags="t",
    )

    maps_10 = list(tools.g_list(type="raster", pattern="depth_10min*", format="json"))
    assert len(maps_10) == 2, (
        f"10-min simulation with output_step=5 should produce 2 maps, got {len(maps_10)}"
    )

    # 20-min run should produce at least 3 time-series maps (t=5, t=10 and
    # t=20, and t=15 as well when the last walkers stay past it).
    maps_20 = list(tools.g_list(type="raster", pattern="depth_20min*", format="json"))
    assert len(maps_20) >= 3, (
        f"20-min simulation with output_step=5 should produce at least 3 maps, "
        f"got {len(maps_20)}"
    )
    assert len(maps_20) > len(maps_10), (
        f"Longer simulation should produce more time-series maps: "
        f"20-min has {len(maps_20)} maps, 10-min has {len(maps_10)} maps"
    )


def test_higher_diffusion_coeff_reduces_depth(east_slope_session):
    """Higher diffusion coefficient must reduce total water depth.

    The diffusion term spreads walkers away from flow concentration zones.
    More diffusion means walkers disperse faster, reducing depth accumulation.
    """
    sum_low = float(np.sum(run_sim(east_slope_session, diffusion_coeff=0.2)))
    sum_high = float(np.sum(run_sim(east_slope_session, diffusion_coeff=2.0)))
    assert sum_low > sum_high, (
        f"Total depth should decrease with higher diffusion: "
        f"sum(dc=0.2)={sum_low:.3e}, sum(dc=2.0)={sum_high:.3e}"
    )


def test_lower_hmax_increases_diffusion(diffusion_session, diffusion_low_hmax_depth):
    """A lower hmax threshold must increase diffusion, reducing total depth.

    When water depth exceeds hmax, diffusion is amplified by (halpha + 1).
    Lowering hmax causes this amplification to kick in sooner.

    Uses a 200-cell domain with high rain and roughness so that depths
    exceed the hmax threshold. The low-hmax run is the shared
    diffusion_low_hmax_depth reference.
    """
    sum_low_hmax = float(np.sum(diffusion_low_hmax_depth))
    sum_default_hmax = float(
        np.sum(
            run_sim(
                diffusion_session,
                rain_value=1000,
                man_value=0.5,
                hmax=0.3,
                nwalkers=40000,
            )
        )
    )
    assert sum_low_hmax < sum_default_hmax, (
        f"Lower hmax should increase diffusion and reduce depth: "
        f"sum(hmax=0.001)={sum_low_hmax:.3e}, sum(hmax=0.3)={sum_default_hmax:.3e}"
    )


def test_higher_halpha_reduces_depth(diffusion_session):
    """A higher halpha must increase the diffusion boost above hmax.

    halpha controls how much extra diffusion is applied when depth exceeds
    hmax: diffusion is multiplied by (halpha + 1). A low hmax is needed
    so that depths actually exceed the threshold.

    Uses a 200-cell domain with high rain and roughness. Both runs use a
    non-default halpha, so neither matches the shared low-hmax reference.
    """
    nw = 40000
    sum_low = float(
        np.sum(
            run_sim(
                diffusion_session,
                rain_value=1000,
                man_value=0.5,
                hmax=0.001,
                halpha=0.5,
                nwalkers=nw,
            )
        )
    )
    sum_high = float(
        np.sum(
            run_sim(
                diffusion_session,
                rain_value=1000,
                man_value=0.5,
                hmax=0.001,
                halpha=50.0,
                nwalkers=nw,
            )
        )
    )
    assert sum_high < sum_low, (
        f"Higher halpha should increase diffusion and reduce depth: "
        f"sum(halpha=0.5)={sum_low:.3e}, sum(halpha=50)={sum_high:.3e}"
    )


def test_hbeta_changes_result(diffusion_session, diffusion_low_hmax_depth):
    """Changing hbeta must produce a different result when depth exceeds hmax.

    hbeta weights the running average of walker velocity above the hmax
    threshold. The effect is small on a 1D domain, so only inequality
    (not direction) is checked.

    Uses a 200-cell domain with high rain, roughness, and low hmax. The
    default-hbeta run is the shared diffusion_low_hmax_depth reference
    (hbeta=0.5 is the tool default).
    """
    depth_high = run_sim(
        diffusion_session,
        rain_value=1000,
        man_value=0.5,
        hmax=0.001,
        hbeta=10.0,
        nwalkers=40000,
    )
    assert not np.array_equal(diffusion_low_hmax_depth, depth_high), (
        "Changing hbeta should produce a different depth result"
    )


def test_man_raster_matches_scalar(east_slope_session):
    """A uniform Manning's n raster must produce the same depth as the equivalent scalar."""
    man_n = 0.1
    tools = Tools(session=east_slope_session)
    tools.r_mapcalc(expression=f"man_map = {man_n}")
    sum_scalar = float(np.sum(run_sim(east_slope_session, man_value=man_n)))
    sum_raster = float(
        np.sum(run_sim(east_slope_session, man="man_map", man_value=None))
    )
    assert sum_raster == pytest.approx(sum_scalar, rel=1e-6)


def test_infil_raster_matches_scalar(east_slope_session):
    """A uniform infiltration raster must produce the same depth as the equivalent scalar."""
    tools = Tools(session=east_slope_session)
    infil_rate = RAIN // 2
    tools.r_mapcalc(expression=f"infil_map = {infil_rate}")
    sum_scalar = float(np.sum(run_sim(east_slope_session, infil_value=infil_rate)))
    sum_raster = float(
        np.sum(run_sim(east_slope_session, infil="infil_map", infil_value=None))
    )
    assert sum_raster == pytest.approx(sum_scalar, rel=1e-6)


def test_dx_dy_optional(tmp_path):
    """Omitting dx and dy must produce valid positive depth.

    The documentation states that dx and dy are optional; when omitted,
    partial derivatives are computed internally from the elevation.
    """
    project = tmp_path / "simwe"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(w=0, e=5, s=0, n=1, res=1)
        tools.r_mapcalc(expression="elevation = 6 - col()")
        depth = tools.r_sim_water(
            elevation="elevation",
            depth=np.array,
            rain_value=RAIN,
            man_value=0.1,
            duration=DURATION,
            random_seed=SEED,
            nprocs=NPROCS,
        )
        assert np.sum(depth) > 0, "Expected positive depth when dx/dy are omitted"


def test_flow_control_increases_depth(east_slope_session):
    """A uniform flow control map must increase total water depth.

    The flow_control raster defines a per-cell trapping probability (0-1).
    When trapped, a walker's velocity is reversed and scaled to 10%,
    slowing it down. Slower walkers linger longer, increasing depth.
    Higher trapping probability means more accumulation.
    """
    tools = Tools(session=east_slope_session)
    # The ten default walkers leave the total with trapping at 0.3 within
    # noise of the total without trapping for about a quarter of the seeds;
    # a thousand separate no trapping, 0.3 and 0.8 for every seed tried.
    walkers = {"nwalkers": 1000}
    sum_no_control = float(np.sum(run_sim(east_slope_session, **walkers)))
    tools.r_mapcalc(expression="flow_ctrl_low = 0.3")
    tools.r_mapcalc(expression="flow_ctrl_high = 0.8")
    sum_low_trap = float(
        np.sum(run_sim(east_slope_session, flow_control="flow_ctrl_low", **walkers))
    )
    sum_high_trap = float(
        np.sum(run_sim(east_slope_session, flow_control="flow_ctrl_high", **walkers))
    )
    assert sum_low_trap > sum_no_control, (
        f"Trapping should increase depth: "
        f"sum(no trap)={sum_no_control:.3e}, sum(trap=0.3)={sum_low_trap:.3e}"
    )
    assert sum_high_trap > sum_low_trap, (
        f"More trapping should increase depth further: "
        f"sum(trap=0.3)={sum_low_trap:.3e}, sum(trap=0.8)={sum_high_trap:.3e}"
    )


def test_time_series_outputs(long_slope_session):
    """The -t flag with output_step must write depth and walker maps per step.

    With duration=10 and output_step=5, the tool writes depth maps at
    minutes 5 and 10 (ts_depth.05 and ts_depth.10) and, with walkers_output,
    vector point maps with an underscore-separated suffix (walkers_05 and
    walkers_10). Total depth at the later step is at least that of the
    earlier one, as water accumulates over time.

    Uses a 200-cell domain at 10 m resolution with high roughness so that
    walkers survive the full simulation. On small or steep domains walkers
    leave before the first output step, producing no time-series maps.
    """
    tools = Tools(session=long_slope_session)
    tools.r_sim_water(
        elevation="elevation",
        dx="dx",
        dy="dy",
        depth="ts_depth",
        rain_value=RAIN,
        man_value=0.3,
        duration=10,
        output_step=5,
        random_seed=SEED,
        nprocs=NPROCS,
        walkers_output="walkers",
        flags="t",
    )

    sum_05 = tools.r_univar(map="ts_depth.05", format="json")["sum"]
    sum_10 = tools.r_univar(map="ts_depth.10", format="json")["sum"]
    assert sum_05 > 0, "Expected positive depth at t=5"
    assert sum_10 >= sum_05, (
        f"Depth at t=10 ({sum_10:.3e}) should be >= depth at t=5 ({sum_05:.3e})"
    )

    info_05 = tools.v_info(map="walkers_05", flags="t", format="json")
    info_10 = tools.v_info(map="walkers_10", flags="t", format="json")
    assert int(info_05["points"]) > 0, "Expected walker points at t=5"
    assert int(info_10["points"]) > 0, "Expected walker points at t=10"


def test_observation_logfile(east_slope_session, tmp_path):
    """Observation points must log water depth at each time step.

    Three observation points are placed on the east_slope_session domain
    at upslope, midslope, and downslope positions. The logfile must contain
    a header with category numbers and data lines with depth values.
    Depth should increase from upslope to downslope. See
    test_north_slope_observation_logfile for the north-south variant.
    """
    tools = Tools(session=east_slope_session)

    points_data = io.StringIO("0.5|0.5|1\n2.5|0.5|2\n4.5|0.5|3\n")
    tools.v_in_ascii(input=points_data, output="points", cat=3)

    logfile = str(tmp_path / "obs_log.txt")
    tools.r_sim_water(
        elevation="elevation",
        dx="dx",
        dy="dy",
        depth=np.array,
        rain_value=RAIN,
        man_value=0.1,
        nwalkers=1000,
        duration=DURATION,
        random_seed=SEED,
        nprocs=NPROCS,
        observation="points",
        logfile=logfile,
    )

    lines = pathlib.Path(logfile).read_text(encoding="utf-8").strip().split("\n")

    # Header: "STEP   CAT0001 CAT0002 CAT0003"
    header = lines[0].split()
    assert header[0] == "STEP"
    assert "CAT0001" in header
    assert "CAT0002" in header
    assert "CAT0003" in header

    # Must have at least one data line after the header.
    assert len(lines) > 1, "Logfile should contain data lines after the header"

    # Data line: "000028 0.0000 0.0001 0.0001" (step, then one depth per point)
    last_vals = [float(v) for v in lines[-1].split()[1:]]
    upslope = last_vals[0]
    midslope = last_vals[1]
    downslope = last_vals[2]
    assert downslope >= midslope >= upslope, (
        f"Depth should increase downstream: "
        f"upslope={upslope:.4f}, midslope={midslope:.4f}, downslope={downslope:.4f}"
    )


def test_walkers_output(long_slope_session):
    """The walkers_output parameter must produce a vector point map.

    Each surviving walker position is written as a 3D point. The tool gives
    each cell one walker more than its share of nwalkers truncated to an
    integer, so the number of points cannot exceed nwalkers plus the number
    of cells, although the option describes nwalkers as the number of
    walkers.

    Uses a large domain (200 cells) to give walkers sufficient residence
    time before exiting. High roughness (0.3) further increases walker
    retention; small domains produce no walker output regardless of roughness.
    """
    tools = Tools(session=long_slope_session)
    nwalkers = 500
    tools.r_sim_water(
        elevation="elevation",
        dx="dx",
        dy="dy",
        depth=np.array,
        rain_value=RAIN,
        man_value=0.3,
        nwalkers=nwalkers,
        duration=DURATION,
        random_seed=SEED,
        nprocs=NPROCS,
        walkers_output="walkers",
    )

    info = tools.v_info(map="walkers", flags="t", format="json")
    npoints = int(info["points"])
    assert npoints > 0, "Expected at least one walker point"
    region = tools.g_region(flags="p", format="json")
    cells = int(region["rows"]) * int(region["cols"])
    assert npoints <= nwalkers + cells, (
        f"Number of walker points ({npoints}) should not exceed nwalkers plus "
        f"the number of cells ({nwalkers} + {cells})"
    )


def test_nprocs_gives_result_within_noise(east_slope_session):
    """Multiple threads produce a result close to a single thread.

    Each walker draws the same random numbers with any number of threads,
    but threads add to the water depth without synchronization, so
    multi-threaded results vary between runs even with a fixed seed. Total
    depth should
    still agree with the single-threaded result within Monte Carlo noise,
    using the same tolerance as test_results_consistent_across_seeds.
    """
    sum_single = float(np.sum(run_sim(east_slope_session, nwalkers=1000)))
    sum_multi = float(np.sum(run_sim(east_slope_session, nwalkers=1000, nprocs=4)))
    tolerance = 0.1
    assert sum_multi == pytest.approx(sum_single, rel=tolerance), (
        f"nprocs=4 result ({sum_multi:.3e}) should match "
        f"nprocs=1 result ({sum_single:.3e}) within {tolerance:.0%}"
    )


def test_north_slope_rain_produces_positive_depth(tmp_path):
    """Rainfall on a north-south slope must create positive water depth.

    Mirrors test_rain_produces_positive_depth with dy instead of dx, verifying
    flow direction when dy != 0 and dx = 0.
    """
    project = tmp_path / "simwe_north"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(w=0, e=1, s=0, n=5, res=1)
        tools.r_mapcalc(expression="elevation = row()")
        tools.r_mapcalc(expression="dx = 0.0")
        tools.r_mapcalc(expression="dy = 1.0")

        depth = run_sim(session)
        assert np.sum(depth) > 0, (
            "Expected positive total depth with rainfall on north-south slope"
        )


def test_north_slope_depth_increases_downstream(tmp_path):
    """Water must get deeper from cell to cell down a north-south slope.

    By continuity, discharge q(y) = R * y grows linearly with distance y
    from the divide. Manning's kinematic wave then gives h(y) proportional
    to y^(3/5), increasing toward the northern boundary.

    Uses a 6-row x 1-column domain with the outlet in row 0. Mirrors
    test_depth_increases_downstream with dy instead of dx.

    Note: dy = 1.0 because elevation increases southward (with increasing row).
    """
    project = tmp_path / "simwe_north_grad"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(w=0, e=1, s=0, n=6, res=1)
        # elevation = row() gives [1, 2, 3, 4, 5, 6] north to south
        # North (low row index) has low elevation, water flows north
        tools.r_mapcalc(expression="elevation = row()")
        tools.r_mapcalc(expression="dx = 0.0")
        tools.r_mapcalc(expression="dy = 1.0")

        # Across 13 seeds at 2000 walkers every cell exceeded the one above
        # it (to the south) by at least 3%.
        depth = run_sim(session, nwalkers=2000).flatten()
        assert np.all(np.diff(depth) < 0), (
            f"Depth should increase cell by cell toward the north: {depth}"
        )


def test_north_slope_observation_logfile(tmp_path):
    """Observation parameter works with north-south (dy != 0) flow.

    Mirrors test_observation_logfile with dy instead of dx. Water flows north
    (downhill), so observation points should show increasing depth from south
    (cat0001) to north (cat0003). Uses a 10-row x 1-column domain for adequate
    water flow to observation points.
    """
    project = tmp_path / "simwe_north_obs"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(w=0, e=1, s=0, n=10, res=1)
        tools.r_mapcalc(expression="elevation = row()")
        tools.r_mapcalc(expression="dx = 0.0")
        tools.r_mapcalc(expression="dy = 1.0")

        # Three observation points along the north-south axis.
        points_data = io.StringIO("0.5|1.5|1\n0.5|5.5|2\n0.5|9.5|3\n")
        tools.v_in_ascii(input=points_data, output="points", cat=3)

        logfile = str(tmp_path / "obs_log_north.txt")
        tools.r_sim_water(
            elevation="elevation",
            dx="dx",
            dy="dy",
            depth=np.array,
            rain_value=RAIN,
            man_value=0.1,
            nwalkers=1000,
            duration=DURATION,
            random_seed=SEED,
            nprocs=NPROCS,
            observation="points",
            logfile=logfile,
        )

        lines = pathlib.Path(logfile).read_text(encoding="utf-8").strip().split("\n")

        # Header: "STEP   CAT0001 CAT0002 CAT0003"
        header = lines[0].split()
        assert header[0] == "STEP"
        assert "CAT0001" in header
        assert "CAT0002" in header
        assert "CAT0003" in header

        # Must have at least one data line after the header.
        assert len(lines) > 1, "Logfile should contain data lines after the header"

        # Data line: depth increases from south (cat0001) to north (cat0003)
        last_vals = [float(v) for v in lines[-1].split()[1:]]
        south = last_vals[0]
        mid = last_vals[1]
        north = last_vals[2]
        assert north >= mid >= south, (
            f"Depth should increase downstream (northward): "
            f"south={south:.4f}, mid={mid:.4f}, north={north:.4f}"
        )
