"""Fixtures for t.rast.mapcalc tests."""

import os
from types import SimpleNamespace

import pytest

import grass.script as gs


def _setup_mapcalc_session(tmp_path_factory, use_seed):
    """Create a temporary GRASS project with STRDS data for t.rast.mapcalc tests."""
    tmp_path = tmp_path_factory.mktemp("t_rast_mapcalc")
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        env = session.env
        gs.run_command("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1", env=env)
        gs.run_command(
            "g.region",
            s=0,
            n=80,
            w=0,
            e=120,
            b=0,
            t=50,
            res=10,
            res3=10,
            env=env,
        )
        if use_seed:
            for name, val in [
                ("prec_1", 550),
                ("prec_2", 450),
                ("prec_3", 320),
                ("prec_4", 510),
                ("prec_5", 300),
                ("prec_6", 650),
            ]:
                gs.run_command(
                    "r.mapcalc",
                    expression=f"{name} = rand(0, {val})",
                    flags="s",
                    overwrite=True,
                    env=env,
                )
        else:
            gs.run_command(
                "r.mapcalc",
                expression="prec_1 = rand(0, 550)",
                overwrite=True,
                env=env,
            )
            gs.run_command(
                "r.mapcalc",
                expression="prec_2 = rand(0, 450)",
                overwrite=True,
                env=env,
            )
            gs.run_command(
                "r.mapcalc",
                expression="prec_3 = rand(0, 320)",
                overwrite=True,
                env=env,
            )
            gs.run_command(
                "r.mapcalc",
                expression="prec_4 = rand(0, 510)",
                overwrite=True,
                env=env,
            )
            gs.run_command(
                "r.mapcalc",
                expression="prec_5 = rand(0, 300)",
                overwrite=True,
                env=env,
            )
            gs.run_command(
                "r.mapcalc",
                expression="prec_6 = rand(0, 650)",
                overwrite=True,
                env=env,
            )
        gs.run_command(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="precip_abs1",
            title="A test",
            description="A test",
            overwrite=True,
            env=env,
        )
        gs.run_command(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="precip_abs2",
            title="A test",
            description="A test",
            overwrite=True,
            env=env,
        )
        gs.run_command(
            "t.register",
            flags="i",
            type="raster",
            input="precip_abs1",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="3 months",
            overwrite=True,
            env=env,
        )
        gs.run_command(
            "t.register",
            type="raster",
            input="precip_abs2",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            overwrite=True,
            env=env,
        )
        yield SimpleNamespace(env=env)
        gs.run_command(
            "t.remove",
            flags="rf",
            type="strds",
            inputs="precip_abs1,precip_abs2,precip_abs3,precip_abs4",
            quiet=True,
            env=env,
        )
        gs.run_command(
            "g.remove",
            flags="f",
            type="raster",
            pattern="prec_*",
            quiet=True,
            env=env,
        )
        gs.run_command(
            "g.remove",
            flags="f",
            type="raster",
            pattern="new_prec_*",
            quiet=True,
            env=env,
        )


@pytest.fixture(scope="module")
def mapcalc_session_basic(tmp_path_factory):
    """Session with STRDS data for basic tests (rand without seed)."""
    yield from _setup_mapcalc_session(tmp_path_factory, use_seed=False)


@pytest.fixture(scope="module")
def mapcalc_session_operators(tmp_path_factory):
    """Session with STRDS data for operator tests (rand with seed)."""
    yield from _setup_mapcalc_session(tmp_path_factory, use_seed=True)
