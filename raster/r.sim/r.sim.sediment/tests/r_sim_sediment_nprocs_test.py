# SPDX-FileCopyrightText: 2026 GRASS Development Team
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


def test_nprocs_from_gisenv(tools):
    """NPROCS set by g.gisenv replaces the default of one thread"""
    expected = threads(tools, nprocs=2)
    tools.g_gisenv(set="NPROCS=2")
    assert threads(tools) == expected


def test_nprocs_zero_is_openmp_default(tools, session_in_mapset):
    """With nprocs=0, OpenMP decides, here by OMP_NUM_THREADS"""
    env = session_in_mapset.env.copy()
    env["OMP_NUM_THREADS"] = "2"
    assert threads(tools, nprocs=0, env=env) == threads(tools, nprocs=2)


def test_negative_nprocs_counts_down(tools, session_in_mapset):
    """A negative nprocs leaves that many CPUs unused, but uses at least one"""
    env = session_in_mapset.env.copy()
    env.pop("OMP_NUM_THREADS", None)
    all_cpus = threads(tools, nprocs=0, env=env)
    assert threads(tools, nprocs=-2, env=env) == max(all_cpus - 2, 1)


def test_mask_uses_one_thread(tools):
    """With a raster mask, one thread is used whatever nprocs is"""
    tools.r_mask(raster="rows_raster")
    assert threads(tools, nprocs=2) == 1
