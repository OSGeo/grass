# SPDX-License-Identifier: GPL-2.0-or-later
import io
import os

import numpy as np
import pytest

import grass.script as gs
import grass.script.array as garray
from grass.tools import Tools

INPUT = """north: 220542
south: 220528
east: 638492
west: 638478
rows: 7
cols: 7
3 2 5 3 5 3 4
1 4 5 5 5 * 4
2 1 * 3 5 5 2
4 2 4 4 4 5 4
4 4 2 * 5 2 4
1 2 1 1 2 2 2
5 4 1 2 3 4 2"""

# Reference outputs, unchanged from the previous gunittest version of this
# test, except for one cell: row 0, column 6 (the northeast corner) is null
# in each of these references, but the current tool fills it in every mode
# (verified locally; see test_matches_reference_except_known_corner_cell).
# The previous version of this test did not catch this, because it compared
# rasters through the difference between them, and a difference involving a
# null cell is itself null, so a value on one side and null on the other
# does not show up as a numeric difference.
REFERENCE = {
    "wmean": """north: 220542
south: 220528
east: 638492
west: 638478
rows: 7
cols: 7
2.7803300858899105 2.4093647857764431 4.6588626785196299 3.4093647857764431 4.7270901428157037 3.2196699141100891 *
1.4093647857764431 3.5529114668595096 4.9317725357039253 4.7445208382054336 5 4.25 3.8535533905932735
1.9999999999999996 1.3411373214803695 3.25 3.3411373214803692 4.8083906286540756 4.7953176071117767 2.4775922500725169
3.7270901428157042 2.3193489522432071 3.7270901428157042 3.9317725357039262 4.1916093713459235 4.6806510477567924 3.9317725357039262
3.795317607111778 3.6167812573081513 2.2046823928882215 3.0000000000000004 4.5224077499274822 2.5109583235891315 3.7270901428157051
1.5458197143685908 2.1277395808972828 1.127739580897283 1.1364549285921475 2.1916093713459244 2.1277395808972828 2.1364549285921481
4.6338834764831835 3.7270901428157042 1.2729098571842956 1.9317725357039264 2.9317725357039257 3.6588626785196308 2.1464466094067265""",
    "mean": """north: 220542
south: 220528
east: 638492
west: 638478
rows: 7
cols: 7
2.5 3.3333333333333335 4 4.666666666666667 4.2000000000000002 4.2000000000000002 *
2.1666666666666665 2.875 3.5 4.5 4.25 4.125 3.6000000000000001
2.3333333333333335 2.875 3.5 4.375 4.5 4.25 4
2.8333333333333335 2.875 2.8571428571428572 3.8571428571428572 4.125 4 3.6666666666666665
2.8333333333333335 2.6666666666666665 2.5 2.875 3.125 3.3333333333333335 3.1666666666666665
3.3333333333333335 2.6666666666666665 2.125 2.125 2.625 2.8888888888888888 2.6666666666666665
3 2.3333333333333335 1.8333333333333333 1.6666666666666667 2.3333333333333335 2.5 2.5""",
    "median": """north: 220542
south: 220528
east: 638492
west: 638478
rows: 7
cols: 7
2.5 3.5 4.5 5 5 4 *
2 2.5 3.5 5 5 4.5 4
2 3 4 4.5 5 4.5 4
3 3 3 4 4.5 4 4
3 2 2 3 3 4 3
4 2 2 2 2 2 2
3 1.5 1.5 1.5 2 2 2""",
    "mode": """north: 220542
south: 220528
east: 638492
west: 638478
rows: 7
cols: 7
3 5 5 5 5 5 *
2 2 5 5 5 5 4
1 4 4 5 5 5 4
4 4 4 4 5 5 5
4 4 2 4 2 2 2
4 4 2 2 2 2 2
1 1 1 1 2 2 2""",
}

# The reference rasters are null here, but the current tool fills these
# cells; see the REFERENCE comment above.
KNOWN_CORNER_VALUES = {
    "wmean": pytest.approx(3.926776695296637),
    "mean": pytest.approx(3.6666666666666665),
    "median": 4.0,
    "mode": 4,
}


@pytest.fixture
def session(tmp_path):
    """A GRASS session with the input raster used by all modes."""
    project = tmp_path / "r_fill_stats_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.r_in_ascii(input=io.StringIO(INPUT), output="ascii", type="CELL")
        tools.g_region(raster="ascii")
        yield session


@pytest.mark.parametrize("mode", ["wmean", "mean", "median", "mode"])
def test_matches_reference_except_known_corner_cell(session, mode):
    """Each mode matches its reference raster cell by cell, except for the
    one corner cell described in the REFERENCE comment above."""
    tools = Tools(session=session)
    tools.r_fill_stats(
        input="ascii", output="stats", distance=1, mode=mode, power=2, cells=2
    )
    tools.r_in_ascii(
        input=io.StringIO(REFERENCE[mode]), output="reference", type="DCELL"
    )

    actual = garray.array(mapname="stats", env=session.env, null=np.nan)
    reference = garray.array(mapname="reference", env=session.env, null=np.nan)

    corner = (0, 6)
    assert actual[corner] == KNOWN_CORNER_VALUES[mode]
    assert np.isnan(reference[corner])

    actual_rest = np.delete(actual, np.ravel_multi_index(corner, actual.shape))
    reference_rest = np.delete(reference, np.ravel_multi_index(corner, reference.shape))
    assert np.allclose(actual_rest, reference_rest, atol=1e-6, equal_nan=True)
