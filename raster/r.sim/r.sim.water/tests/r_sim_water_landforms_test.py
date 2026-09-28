"""Landform tests for r.sim.water (SIMWE).

Each test simulates a small synthetic elevation surface, given as an array
so that the shape is visible in the code, and asserts what the model must
do on that landform for any random seed (physical assertions, with
tolerances from seed sweeps). The shape tests also pin the result for SEED
(an array that fails whenever the simulated result changes, including a
change in the random number generator); the rotation tests compare two
simulations and pin nothing.
"""

import os

import numpy as np
import pytest

import grass.script as gs
from grass.tools import Tools

# Fixed seed and single thread make results fully deterministic.
SEED = 42
NPROCS = 1

DURATION = 2  # minutes; enough for near-steady state on small domains
RAIN = 100  # mm/hr

# With the default walker count (two per cell) the depth of a single cell or
# column varies too much between seeds to compare cells or columns. In a
# sweep from 2000 to 20000 walkers the spread of per-column sums leveled off
# at 10000.
HIGH_NWALKERS = 10000

# Relative tolerances for comparing two simulations at HIGH_NWALKERS (rtol
# in numpy.testing.assert_allclose, rel in pytest.approx). Each is at least
# twice the largest difference seen across seeds.
TOTAL_DEPTH_RTOL = 0.05  # Depth summed over the whole grid.
COLUMN_PROFILE_RTOL = 0.1  # Depth summed per column, compared column by column.

# The pinned arrays are the float32 result for SEED in millimeters with two
# decimals, so rounding alone moves a value by up to 0.005 mm and the
# tolerance allows twice that. A change in the random number generator moved
# at least one cell by 0.04 mm in every landform at every walker count
# tested, so it still fails the pin.
PIN_ATOL_MM = 0.01

# Elevation surfaces, one cell per element at unit resolution. Rows run north
# to south and columns west to east, matching the depth arrays.
PIT = np.array(
    [
        [3, 3, 3, 3, 3],
        [3, 2, 2, 2, 3],
        [3, 2, 1, 2, 3],
        [3, 2, 2, 2, 3],
        [3, 3, 3, 3, 3],
    ]
)
PEAK = np.array(
    [
        [3, 3, 3, 3, 3, 3, 3],
        [3, 4, 4, 4, 4, 4, 3],
        [3, 4, 5, 5, 5, 4, 3],
        [3, 4, 5, 6, 5, 4, 3],
        [3, 4, 5, 5, 5, 4, 3],
        [3, 4, 4, 4, 4, 4, 3],
        [3, 3, 3, 3, 3, 3, 3],
    ]
)
RIDGE = np.array([[0, 1, 2, 3, 2, 1, 0]] * 5)
SADDLE = np.array(
    [
        [10, 15, 18, 19, 18, 15, 10],
        [5, 10, 13, 14, 13, 10, 5],
        [2, 7, 10, 11, 10, 7, 2],
        [1, 6, 9, 10, 9, 6, 1],
        [2, 7, 10, 11, 10, 7, 2],
        [5, 10, 13, 14, 13, 10, 5],
        [10, 15, 18, 19, 18, 15, 10],
    ]
)
TERRACE = np.array([[9, 9, 9, 9, 7, 6, 6, 6, 4, 3, 3, 3]] * 10)


def surface(row_values, column_values):
    """Elevation grid in which each cell is its row value plus its column value."""
    return np.array(row_values)[:, np.newaxis] + np.array(column_values)


# Ten rows falling southward by 0.5 per row.
DOWNSLOPE = [4.5, 4.0, 3.5, 3.0, 2.5, 2.0, 1.5, 1.0, 0.5, 0.0]
# A V-shaped, a concave parabolic, and a convex parabolic cross-section on
# that slope.
VALLEY = surface(DOWNSLOPE, [3, 2, 1, 0, 1, 2, 3])
HOLLOW = surface(DOWNSLOPE, [1.8, 0.8, 0.2, 0.0, 0.2, 0.8, 1.8])
SPUR = surface(DOWNSLOPE, [-1.8, -0.8, -0.2, 0.0, -0.2, -0.8, -1.8])


def landform_depth(project, elevation, *, random_seed=SEED, **kwargs):
    """Simulate r.sim.water on an elevation array in a new project; return depth.

    The region is one cell per array element at unit resolution. Only the
    elevation is given, so the tool computes the slope components itself.
    """
    nrows, ncols = elevation.shape
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(w=0, e=ncols, s=0, n=nrows, res=1)
        return tools.r_sim_water(
            elevation=elevation,
            depth=np.array,
            random_seed=random_seed,
            rain_value=RAIN,
            infil_value=0,
            man_value=0.1,
            nprocs=NPROCS,
            duration=DURATION,
            **kwargs,
        )


def assert_depth_pinned(depth, expected_mm):
    """Compare simulated depth in meters with the pinned values in millimeters."""
    np.testing.assert_allclose(
        depth.astype(np.float64) * 1000, expected_mm, rtol=0, atol=PIN_ATOL_MM
    )


def test_pit_collects_water(tmp_path):
    """A square bowl must collect most water in its center cell, symmetrically."""
    depth = landform_depth(tmp_path / "pit", PIT)

    center_depth = float(depth[2, 2])
    total_depth = float(np.sum(depth))

    assert center_depth == np.max(depth), (
        f"Pit center depth ({center_depth:.4f}) should be the domain maximum "
        f"({np.max(depth):.4f})"
    )

    # At unit resolution, across 13 seeds the center held at least 53% of
    # the total depth.
    assert center_depth > 0.3 * total_depth, (
        f"Center should hold >30% of total water: "
        f"center={center_depth:.4f}, total={total_depth:.4f}"
    )

    # Radial symmetry: the 4 cardinal neighbors should have similar depth.
    neighbors = [depth[1, 2], depth[3, 2], depth[2, 1], depth[2, 3]]
    mean_neighbor = float(np.mean(neighbors))
    np.testing.assert_allclose(neighbors, mean_neighbor, rtol=0.3)

    assert all(n < center_depth for n in neighbors), (
        f"All neighbors should have less depth than center: "
        f"neighbors={[f'{n:.4f}' for n in neighbors]}, "
        f"center={center_depth:.4f}"
    )

    # Radial symmetry for the 4 diagonal neighbors.
    diagonals = [depth[1, 1], depth[1, 3], depth[3, 1], depth[3, 3]]
    mean_diagonal = float(np.mean(diagonals))
    np.testing.assert_allclose(diagonals, mean_diagonal, rtol=0.3)

    # Depth decreases from the center outward: center, then the cardinal
    # ring, then the edge midpoints.
    edge_midpoints = [depth[0, 2], depth[4, 2], depth[2, 0], depth[2, 4]]
    assert mean_neighbor > float(np.mean(edge_midpoints)), (
        f"Ring 1 mean depth ({mean_neighbor:.4f}) should exceed "
        f"ring 2 mean depth ({float(np.mean(edge_midpoints)):.4f})"
    )

    assert_depth_pinned(
        depth,
        [
            [0.50, 0.39, 0.45, 0.64, 0.39],
            [0.45, 2.90, 14.30, 2.43, 0.80],
            [0.45, 14.34, 87.17, 14.28, 0.45],
            [0.50, 2.56, 14.76, 2.85, 0.68],
            [0.64, 0.50, 0.26, 0.80, 0.50],
        ],
    )


def test_peak_sheds_water(tmp_path):
    """A conical peak must shed water outward, symmetrically in all directions.

    Water shed from the summit accumulates outward, so the second square ring
    around the summit holds more per cell than the first, and cells at the
    same distance from the summit hold similar depth. The summit itself holds
    more than the ring around it: the slope computed from its equal-height
    neighbors is zero, so its water leaves only by diffusion. The outer ring
    is the domain edge and is left out to avoid edge effects.
    """
    depth = landform_depth(tmp_path / "peak", PEAK, nwalkers=HIGH_NWALKERS)

    # The summit is at NumPy index [3, 3].
    summit = float(depth[3, 3])
    neighbors = [depth[2, 3], depth[4, 3], depth[3, 2], depth[3, 4]]
    diagonals = [depth[2, 2], depth[2, 4], depth[4, 2], depth[4, 4]]
    ring2 = [
        depth[i, j]
        for i in range(1, 6)
        for j in range(1, 6)
        if max(abs(i - 3), abs(j - 3)) == 2
    ]

    # Relative tolerance for single cells; the largest deviation from the
    # ring mean seen across seeds was about 7%.
    cell_rtol = 0.15
    np.testing.assert_allclose(neighbors, np.mean(neighbors), rtol=cell_rtol)
    np.testing.assert_allclose(diagonals, np.mean(diagonals), rtol=cell_rtol)

    mean_ring1 = float(np.mean(neighbors + diagonals))
    mean_ring2 = float(np.mean(ring2))
    assert mean_ring2 > mean_ring1, (
        f"Ring 2 mean depth ({mean_ring2:.6f}) should exceed "
        f"ring 1 mean depth ({mean_ring1:.6f})"
    )
    assert summit > mean_ring1, (
        f"Summit depth ({summit:.6f}) should exceed "
        f"ring 1 mean depth ({mean_ring1:.6f})"
    )

    assert_depth_pinned(
        depth,
        [
            [0.89, 0.68, 0.81, 0.89, 0.82, 0.70, 0.90],
            [0.69, 0.52, 0.48, 0.57, 0.49, 0.52, 0.74],
            [0.79, 0.49, 0.38, 0.42, 0.39, 0.52, 0.79],
            [0.92, 0.58, 0.40, 0.69, 0.42, 0.58, 0.90],
            [0.81, 0.50, 0.41, 0.42, 0.37, 0.49, 0.80],
            [0.71, 0.50, 0.48, 0.60, 0.51, 0.51, 0.69],
            [0.86, 0.67, 0.80, 0.92, 0.80, 0.74, 0.87],
        ],
    )


def test_ridge_sheds_water_to_flanks(tmp_path):
    """A ridge must shed water down both flanks, symmetrically.

    Water shed from the crest accumulates down each flank, so the lower flank
    column holds more than the shoulder column next to the crest, and the two
    sides mirror each other. The crest itself holds about twice the depth of
    its shoulders: the slope computed from its equal-height neighbors is
    zero, so its water leaves only by diffusion. The edge columns are left
    out to avoid edge effects.
    """
    depth = landform_depth(tmp_path / "ridge", RIDGE, nwalkers=HIGH_NWALKERS)
    profile = np.sum(depth, axis=0)

    # Columns 2 and 4 are the shoulders next to the crest (3); columns 1 and
    # 5 are the lower flanks.
    for shoulder, flank in ((2, 1), (4, 5)):
        assert profile[flank] > profile[shoulder], (
            f"Lower flank column {flank} ({profile[flank]:.6f}) should "
            f"exceed shoulder column {shoulder} ({profile[shoulder]:.6f})"
        )
        assert profile[3] > profile[shoulder], (
            f"Crest ({profile[3]:.6f}) should exceed shoulder column "
            f"{shoulder} ({profile[shoulder]:.6f})"
        )

    # The per-column profile mirrors across the crest.
    np.testing.assert_allclose(profile[:3], profile[::-1][:3], rtol=COLUMN_PROFILE_RTOL)

    assert_depth_pinned(
        depth,
        [
            [1.00, 0.66, 0.40, 0.57, 0.42, 0.68, 1.02],
            [1.09, 0.76, 0.58, 1.37, 0.56, 0.77, 1.11],
            [1.11, 0.73, 0.52, 1.12, 0.50, 0.72, 1.09],
            [1.13, 0.78, 0.58, 1.45, 0.58, 0.75, 1.11],
            [1.00, 0.64, 0.40, 0.59, 0.40, 0.66, 1.00],
        ],
    )


def test_valley_concentrates_flow_downstream(tmp_path):
    """A V-shaped valley draining southward must concentrate water on its floor.

    The floor holds far more per cell than the hillslopes, depth on the floor
    increases downstream, and the two sides mirror each other.
    """
    depth = landform_depth(tmp_path / "valley", VALLEY, nwalkers=HIGH_NWALKERS)

    # Across 13 seeds the ratio was never below 5.6.
    floor_mean = float(np.mean(depth[:, 3]))
    hillslope_cols = np.concatenate([depth[:, :3], depth[:, 4:]], axis=1)
    hillslope_mean = float(np.mean(hillslope_cols))
    assert floor_mean > 3 * hillslope_mean, (
        f"Valley floor mean depth ({floor_mean:.6f}) should be >3x "
        f"hillslope mean ({hillslope_mean:.6f})"
    )

    # Downstream is toward higher row indices; compare the two halves.
    upper_floor = float(np.sum(depth[:5, 3]))
    lower_floor = float(np.sum(depth[5:, 3]))
    assert lower_floor > upper_floor, (
        f"Lower valley floor ({lower_floor:.6f}) should exceed "
        f"upper valley floor ({upper_floor:.6f})"
    )

    # The per-column profile mirrors across the floor.
    profile = np.sum(depth, axis=0)
    np.testing.assert_allclose(profile[:3], profile[::-1][:3], rtol=COLUMN_PROFILE_RTOL)

    assert_depth_pinned(
        depth,
        [
            [0.34, 0.49, 0.77, 2.17, 0.74, 0.47, 0.36],
            [0.44, 0.60, 0.77, 1.55, 0.80, 0.59, 0.39],
            [0.43, 0.63, 0.96, 2.42, 0.97, 0.59, 0.43],
            [0.41, 0.63, 1.03, 3.19, 1.03, 0.64, 0.41],
            [0.41, 0.65, 1.11, 3.88, 1.08, 0.62, 0.42],
            [0.43, 0.61, 1.16, 4.46, 1.16, 0.64, 0.41],
            [0.45, 0.63, 1.21, 5.02, 1.23, 0.64, 0.41],
            [0.45, 0.65, 1.32, 5.50, 1.27, 0.61, 0.42],
            [0.44, 0.64, 1.37, 6.04, 1.32, 0.63, 0.47],
            [0.44, 0.73, 1.54, 6.61, 1.55, 0.77, 0.50],
        ],
    )


def test_saddle_bifurcates_flow(tmp_path):
    """A saddle must send more water into its two low quadrants than its high ones.

    The surface rises away from the center along the north-south axis and
    falls along the east-west axis, so the east and west quadrants are low
    and the north and south quadrants are high.
    """
    depth = landform_depth(tmp_path / "saddle", SADDLE)

    # A cell is in a low quadrant when it is farther from the center column
    # than from the center row, and in a high quadrant in the opposite case.
    rows, cols = np.indices(depth.shape)
    row_dist = np.abs(rows - 3)
    col_dist = np.abs(cols - 3)
    low_mask = col_dist > row_dist
    high_mask = row_dist > col_dist

    low_sum = float(np.sum(depth[low_mask]))
    high_sum = float(np.sum(depth[high_mask]))
    assert low_sum > high_sum, (
        f"Low quadrant depth ({low_sum:.6f}) should exceed "
        f"high quadrant depth ({high_sum:.6f})"
    )

    # The saddle point has zero computed gradient, so water reaching it
    # leaves only by diffusion, and about four fifths of that water is rain
    # from the center column, which slopes toward it from north and south.
    # It is therefore wetter than the high quadrants.
    high_mean = high_sum / np.sum(high_mask)
    saddle_point_depth = float(depth[3, 3])
    assert saddle_point_depth > high_mean, (
        f"Saddle depth ({saddle_point_depth:.6f}) should exceed "
        f"high quadrant mean ({high_mean:.6f})"
    )

    assert_depth_pinned(
        depth,
        [
            [0.26, 0.28, 0.15, 0.20, 0.24, 0.15, 0.26],
            [0.41, 0.30, 0.28, 0.39, 0.22, 0.51, 0.49],
            [0.72, 0.51, 0.46, 0.81, 0.33, 0.56, 0.85],
            [1.30, 0.83, 0.66, 3.08, 1.07, 0.95, 1.39],
            [0.78, 0.54, 0.30, 0.78, 0.47, 0.61, 0.84],
            [0.43, 0.35, 0.30, 0.41, 0.28, 0.43, 0.45],
            [0.30, 0.20, 0.26, 0.20, 0.24, 0.28, 0.24],
        ],
    )


def test_hollow_concentrates_more_than_spur(tmp_path):
    """A hollow must concentrate more water on its axis than a spur.

    Both surfaces fall southward by 0.5 per row and have the same parabolic
    cross-section, concave for the hollow and convex for the spur. On the
    center column the slope is the same for both, so the depth difference
    there comes from convergence alone. The spur's crest is not its driest
    column (the columns beside it are), so the test compares the two
    landforms rather than columns within the spur.
    """
    # 1000 walkers keep the depth pattern of both surfaces readable in the
    # result at no measurable cost in runtime.
    depth_hollow = landform_depth(tmp_path / "hollow", HOLLOW, nwalkers=1000)
    depth_spur = landform_depth(tmp_path / "spur", SPUR, nwalkers=1000)

    # The factor 2 is a regression threshold: across 13 seeds the measured
    # ratio was never below 3.8.
    axis_hollow = float(np.sum(depth_hollow[:, 3]))
    axis_spur = float(np.sum(depth_spur[:, 3]))
    assert axis_hollow > 2 * axis_spur, (
        f"Hollow axis depth ({axis_hollow:.6f}) should exceed twice the "
        f"spur axis depth ({axis_spur:.6f})"
    )

    # The hollow's axis is its wettest column.
    column_sums = np.sum(depth_hollow, axis=0)
    assert int(np.argmax(column_sums)) == 3, (
        f"Hollow axis should be the wettest column, column sums: "
        f"{[f'{c:.6f}' for c in column_sums]}"
    )

    # The maximum depth, at the outlet end of the axis, follows the same
    # order with the same threshold (measured ratio never below 4.8).
    max_hollow = float(np.max(depth_hollow))
    max_spur = float(np.max(depth_spur))
    assert max_hollow > 2 * max_spur, (
        f"Hollow peak depth ({max_hollow:.6f}) should exceed twice the "
        f"spur peak depth ({max_spur:.6f})"
    )

    assert_depth_pinned(
        depth_hollow,
        [
            [0.34, 0.54, 0.79, 0.67, 0.67, 0.52, 0.36],
            [0.41, 0.57, 0.96, 1.22, 0.90, 0.64, 0.46],
            [0.47, 0.71, 1.25, 2.08, 1.20, 0.73, 0.37],
            [0.41, 0.60, 1.46, 2.84, 1.33, 0.57, 0.42],
            [0.48, 0.65, 1.46, 3.42, 1.43, 0.71, 0.46],
            [0.38, 0.73, 1.56, 4.01, 1.56, 0.76, 0.42],
            [0.43, 0.68, 1.77, 4.53, 1.72, 0.74, 0.39],
            [0.46, 0.65, 1.79, 4.99, 1.68, 0.65, 0.43],
            [0.37, 0.71, 1.87, 5.54, 1.87, 0.80, 0.42],
            [0.43, 0.79, 2.18, 6.95, 2.21, 0.77, 0.43],
        ],
    )

    assert_depth_pinned(
        depth_spur,
        [
            [0.73, 0.46, 0.29, 0.45, 0.36, 0.51, 0.55],
            [0.74, 0.63, 0.56, 0.64, 0.45, 0.56, 0.80],
            [0.97, 0.77, 0.64, 0.78, 0.56, 0.72, 1.09],
            [1.08, 0.77, 0.61, 0.89, 0.54, 0.68, 1.00],
            [1.13, 0.80, 0.64, 0.92, 0.71, 0.66, 0.98],
            [1.08, 0.83, 0.69, 0.96, 0.67, 0.80, 1.11],
            [1.19, 0.75, 0.77, 1.07, 0.77, 0.76, 1.16],
            [1.14, 0.85, 0.73, 1.01, 0.81, 0.75, 1.09],
            [1.20, 0.72, 0.81, 1.05, 0.79, 0.91, 1.26],
            [1.40, 1.09, 0.93, 1.24, 0.81, 0.88, 1.29],
        ],
    )


def test_terrace_accumulates_on_steps(tmp_path):
    """Terraced slopes must hold water on the flat steps, not on the steep drops.

    Three flat steps, separated by one-column drops, descend from west to
    east. Each step holds more than the drop above it, and each step holds
    more than the step above it.
    """
    depth = landform_depth(tmp_path / "terrace", TERRACE)

    # Column indices of each feature in TERRACE.
    step1_cols = slice(0, 4)
    drop1_col = 4
    step2_cols = slice(5, 8)
    drop2_col = 8
    step3_cols = slice(9, 12)

    step1_mean = float(np.mean(depth[:, step1_cols]))
    drop1_mean = float(np.mean(depth[:, drop1_col]))
    step2_mean = float(np.mean(depth[:, step2_cols]))
    drop2_mean = float(np.mean(depth[:, drop2_col]))
    step3_mean = float(np.mean(depth[:, step3_cols]))

    assert step2_mean > drop1_mean, (
        f"Step 2 mean ({step2_mean:.6f}) should exceed drop 1 mean ({drop1_mean:.6f})"
    )
    assert step3_mean > drop2_mean, (
        f"Step 3 mean ({step3_mean:.6f}) should exceed drop 2 mean ({drop2_mean:.6f})"
    )
    assert step2_mean > step1_mean, (
        f"Step 2 ({step2_mean:.6f}) should exceed step 1 ({step1_mean:.6f})"
    )
    assert step3_mean > step2_mean, (
        f"Step 3 ({step3_mean:.6f}) should exceed step 2 ({step2_mean:.6f})"
    )

    assert_depth_pinned(
        depth,
        [
            [0.83, 0.92, 2.27, 0.31, 0.48, 1.10, 4.06, 0.78, 1.01, 1.93, 8.25, 4.35],
            [1.78, 2.47, 2.73, 0.97, 1.05, 1.96, 6.18, 1.57, 1.33, 3.01, 12.26, 7.19],
            [1.98, 2.97, 3.50, 0.78, 0.67, 1.43, 4.73, 1.43, 1.33, 3.37, 13.64, 8.68],
            [3.56, 5.51, 3.31, 0.83, 0.88, 1.78, 6.33, 1.36, 1.43, 3.96, 15.10, 9.38],
            [3.82, 5.70, 3.64, 0.97, 0.97, 2.45, 7.33, 1.93, 1.43, 3.96, 17.13, 10.67],
            [4.35, 6.22, 3.58, 0.72, 0.83, 2.17, 5.73, 1.22, 1.40, 4.03, 18.60, 11.42],
            [3.76, 5.41, 3.58, 0.83, 0.67, 1.72, 5.00, 1.33, 1.22, 3.50, 16.33, 10.49],
            [3.52, 3.64, 3.17, 0.67, 0.72, 1.78, 5.99, 1.10, 1.05, 2.97, 12.78, 9.85],
            [3.35, 3.37, 2.77, 0.55, 0.67, 1.98, 5.48, 1.47, 1.50, 2.81, 10.44, 8.40],
            [2.50, 1.50, 2.27, 0.55, 0.48, 1.22, 3.54, 1.18, 1.14, 2.17, 7.91, 4.13],
        ],
    )


@pytest.mark.parametrize(
    "elevation", [RIDGE, VALLEY, TERRACE], ids=["ridge", "valley", "terrace"]
)
def test_rotation_invariance(tmp_path, elevation):
    """A landform and its 90-degree rotation must produce equivalent depth.

    The rotated surface is simulated on its own grid and its result rotated
    back, so the two depth grids share one orientation. Total depth and the
    per-column sums must then agree within walker noise.
    """
    depth = landform_depth(tmp_path / "a", elevation, nwalkers=HIGH_NWALKERS)
    depth_rotated = landform_depth(
        tmp_path / "b", np.rot90(elevation, k=-1), nwalkers=HIGH_NWALKERS
    )
    depth_back = np.rot90(depth_rotated)

    total = float(np.sum(depth))
    total_back = float(np.sum(depth_back))
    assert total == pytest.approx(total_back, rel=TOTAL_DEPTH_RTOL), (
        f"Total depth ({total:.6f}) vs rotated ({total_back:.6f})"
    )
    np.testing.assert_allclose(
        np.sum(depth, axis=0), np.sum(depth_back, axis=0), rtol=COLUMN_PROFILE_RTOL
    )


def test_saddle_rotation_invariance(tmp_path):
    """A saddle and its 90-degree rotation must produce equivalent depth.

    The saddle point amplifies walker noise in the center row and column, so
    instead of per-column sums the test compares the blocks on either side
    of the center column, which are far smoother.
    """
    depth = landform_depth(tmp_path / "a", SADDLE, nwalkers=HIGH_NWALKERS)
    depth_rotated = landform_depth(
        tmp_path / "b", np.rot90(SADDLE, k=-1), nwalkers=HIGH_NWALKERS
    )
    depth_back = np.rot90(depth_rotated)

    total = float(np.sum(depth))
    total_back = float(np.sum(depth_back))
    assert total == pytest.approx(total_back, rel=TOTAL_DEPTH_RTOL), (
        f"Total depth ({total:.6f}) vs rotated ({total_back:.6f})"
    )

    # Relative tolerance for the side blocks; the largest difference seen
    # across seeds was 0.3%.
    side_rtol = 0.02
    sides = float(np.sum(depth[:, :3]) + np.sum(depth[:, 4:]))
    sides_back = float(np.sum(depth_back[:, :3]) + np.sum(depth_back[:, 4:]))
    assert sides == pytest.approx(sides_back, rel=side_rtol), (
        f"Side block depth ({sides:.6f}) vs rotated ({sides_back:.6f})"
    )
