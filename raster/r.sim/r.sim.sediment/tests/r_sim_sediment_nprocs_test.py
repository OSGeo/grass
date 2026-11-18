# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the number of threads r.sim.sediment uses"""

import pytest


@pytest.fixture
def tools(session_tools):
    """Tools with constant water depth and soil parameter maps"""
    session_tools.r_mapcalc(expression="water_depth = 0.1")
    session_tools.r_mapcalc(expression="coeff = 0.001")
    return session_tools


def threads(tools, **kwargs):
    """Return the number of threads from the run summary"""
    return tools.r_sim_sediment(
        elevation="rows_raster",
        water_depth="water_depth",
        detachment_coeff="coeff",
        transport_coeff="coeff",
        shear_stress="coeff",
        sediment_flux="flux",
        nwalkers=10,
        duration=1,
        flags="p",
        format="json",
        overwrite=True,
        **kwargs,
    )["threads"]


def test_nprocs_from_gisenv(tools, session_in_mapset):
    """NPROCS=0 from g.gisenv replaces the default of one thread"""
    env = session_in_mapset.env.copy()
    env["OMP_NUM_THREADS"] = "2"
    tools.g_gisenv(set="NPROCS=0")
    assert threads(tools, env=env) == threads(tools, nprocs=2)
