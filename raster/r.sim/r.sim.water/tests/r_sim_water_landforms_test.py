# SPDX-License-Identifier: GPL-2.0-or-later

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
            [0.55, 0.60, 0.45, 0.33, 0.55],
            [0.45, 2.84, 14.42, 2.67, 0.64],
            [0.39, 14.55, 87.43, 13.70, 0.45],
            [0.39, 2.45, 14.30, 2.36, 0.76],
            [0.60, 0.39, 0.55, 0.60, 0.50],
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
            [0.86, 0.69, 0.79, 0.92, 0.81, 0.73, 0.86],
            [0.72, 0.52, 0.50, 0.58, 0.48, 0.52, 0.70],
            [0.84, 0.50, 0.38, 0.43, 0.40, 0.51, 0.81],
            [0.89, 0.59, 0.41, 0.73, 0.40, 0.58, 0.90],
            [0.79, 0.47, 0.36, 0.41, 0.36, 0.49, 0.83],
            [0.69, 0.51, 0.49, 0.58, 0.49, 0.50, 0.71],
            [0.87, 0.70, 0.84, 0.89, 0.82, 0.70, 0.86],
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
            [0.96, 0.66, 0.39, 0.59, 0.40, 0.65, 0.98],
            [1.15, 0.77, 0.58, 1.37, 0.57, 0.77, 1.14],
            [1.09, 0.71, 0.50, 1.07, 0.51, 0.72, 1.09],
            [1.14, 0.77, 0.58, 1.39, 0.58, 0.78, 1.10],
            [0.98, 0.65, 0.41, 0.58, 0.39, 0.63, 0.98],
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
            [0.37, 0.48, 0.73, 2.11, 0.69, 0.47, 0.35],
            [0.43, 0.60, 0.80, 1.54, 0.79, 0.56, 0.40],
            [0.43, 0.64, 0.97, 2.38, 0.95, 0.63, 0.43],
            [0.41, 0.62, 1.02, 3.17, 1.04, 0.64, 0.44],
            [0.41, 0.60, 1.11, 3.84, 1.11, 0.60, 0.41],
            [0.42, 0.64, 1.16, 4.44, 1.18, 0.64, 0.42],
            [0.43, 0.64, 1.21, 5.01, 1.23, 0.61, 0.41],
            [0.43, 0.63, 1.27, 5.53, 1.25, 0.60, 0.44],
            [0.44, 0.63, 1.33, 6.03, 1.30, 0.68, 0.42],
            [0.50, 0.75, 1.51, 6.58, 1.51, 0.74, 0.48],
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
            [0.38, 0.18, 0.35, 0.15, 0.24, 0.18, 0.33],
            [0.45, 0.43, 0.42, 0.24, 0.38, 0.30, 0.46],
            [0.85, 0.64, 0.38, 0.64, 0.51, 0.59, 0.68],
            [1.41, 0.83, 0.77, 3.21, 0.77, 0.90, 1.42],
            [0.69, 0.70, 0.41, 0.91, 0.45, 0.66, 0.74],
            [0.43, 0.33, 0.41, 0.33, 0.42, 0.20, 0.46],
            [0.36, 0.30, 0.22, 0.20, 0.26, 0.18, 0.45],
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
            [0.32, 0.44, 0.66, 0.94, 0.77, 0.47, 0.40],
            [0.42, 0.63, 0.89, 1.43, 0.97, 0.64, 0.44],
            [0.50, 0.69, 1.24, 2.02, 1.23, 0.66, 0.38],
            [0.47, 0.66, 1.34, 2.83, 1.41, 0.60, 0.43],
            [0.43, 0.85, 1.57, 3.41, 1.37, 0.65, 0.44],
            [0.42, 0.59, 1.54, 4.10, 1.55, 0.81, 0.43],
            [0.45, 0.70, 1.57, 4.55, 1.78, 0.66, 0.47],
            [0.47, 0.67, 1.65, 5.12, 1.75, 0.70, 0.37],
            [0.44, 0.77, 1.79, 5.59, 1.77, 0.63, 0.48],
            [0.52, 0.77, 2.24, 6.88, 2.12, 0.84, 0.49],
        ],
    )

    assert_depth_pinned(
        depth_spur,
        [
            [0.68, 0.41, 0.48, 0.51, 0.43, 0.47, 0.66],
            [0.89, 0.67, 0.49, 0.59, 0.54, 0.70, 0.81],
            [0.87, 0.59, 0.71, 0.78, 0.48, 0.71, 1.02],
            [1.01, 0.84, 0.74, 0.91, 0.60, 0.68, 1.00],
            [1.23, 0.83, 0.74, 0.91, 0.62, 0.73, 0.94],
            [1.17, 0.79, 0.74, 0.94, 0.73, 0.77, 1.20],
            [1.07, 0.77, 0.78, 0.91, 0.75, 0.80, 1.07],
            [1.11, 0.91, 0.61, 0.95, 0.74, 0.84, 1.22],
            [1.27, 0.76, 0.71, 0.94, 0.67, 0.81, 1.15],
            [1.38, 1.04, 0.87, 1.20, 0.78, 0.94, 1.40],
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
            [1.72, 3.47, 3.01, 0.72, 0.72, 1.60, 2.73, 0.88, 0.48, 1.69, 6.92, 3.87],
            [1.96, 4.40, 2.52, 0.55, 0.88, 1.66, 3.96, 1.22, 1.40, 3.31, 13.03, 8.10],
            [2.59, 4.41, 3.31, 0.92, 0.88, 2.07, 7.22, 1.43, 1.43, 3.31, 14.28, 9.66],
            [2.17, 3.50, 2.68, 0.67, 0.83, 2.01, 6.20, 1.50, 1.53, 3.78, 16.37, 10.49],
            [3.03, 4.53, 2.45, 0.72, 0.78, 2.04, 7.47, 1.43, 1.29, 4.15, 17.83, 11.21],
            [3.47, 4.72, 2.75, 0.92, 0.97, 2.04, 6.24, 1.60, 1.47, 4.10, 17.20, 11.12],
            [3.56, 4.91, 3.58, 0.61, 0.78, 1.96, 5.18, 1.14, 0.92, 3.64, 16.76, 10.80],
            [1.69, 3.92, 1.66, 0.48, 0.72, 2.01, 6.33, 1.47, 1.36, 3.47, 12.64, 8.50],
            [3.50, 2.47, 2.68, 0.83, 0.92, 2.12, 5.88, 1.63, 1.36, 2.77, 9.52, 6.49],
            [2.30, 2.15, 1.87, 0.83, 0.48, 1.40, 4.43, 1.14, 1.14, 2.01, 5.54, 3.64],
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
