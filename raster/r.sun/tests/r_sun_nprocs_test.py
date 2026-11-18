# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the number of threads r.sun uses"""

import re

import numpy as np
import pytest

from grass.tools import Tools


def radiation(tools, **kwargs):
    """Return the global radiation for a day as an array"""
    return tools.r_sun(elevation="rows_raster", day=172, glob_rad=np.array, **kwargs)


@pytest.mark.parametrize("nprocs", [0, -1])
def test_nprocs_gives_result_of_one_thread(session_tools, nprocs):
    """nprocs of 0 and below are accepted and do not change the result"""
    np.testing.assert_array_equal(
        radiation(session_tools, nprocs=nprocs), radiation(session_tools, nprocs=1)
    )


def threads(session, **kwargs):
    """Return the number of threads r.sun reports"""
    result = Tools(session=session, consistent_return_value=True).r_sun(
        elevation="rows_raster", day=172, glob_rad="radiation", overwrite=True, **kwargs
    )
    return int(re.search(r"Number of threads <(\d+)>", result.stderr).group(1))


def test_nprocs_from_gisenv(session_in_mapset):
    """NPROCS set by g.gisenv replaces the default of one thread"""
    expected = threads(session_in_mapset, nprocs=2)
    assert threads(session_in_mapset) == 1
    Tools(session=session_in_mapset).g_gisenv(set="NPROCS=2")
    assert threads(session_in_mapset) == expected
