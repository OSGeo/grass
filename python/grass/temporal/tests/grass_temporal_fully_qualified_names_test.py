"""Access space time datasets by fully qualified name (name@mapset).

Regression tests for https://github.com/OSGeo/grass/issues/7641: temporal
tools should resolve a fully qualified STDS name from any accessible mapset,
even when that mapset is not on the current search path.
"""

from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def two_mapset_session(tmp_path_factory):
    """STRDS in PERMANENT plus one in an off-search-path mapset ('user1').

    Ends in PERMANENT so 'user1' is not on the search path. Uses
    gs.setup.init(project) without a copied env so both Tools and the tgis
    library share the same active session (mirrors
    grass_temporal_core_init_skip_db_test.py).
    """
    tmp_path = tmp_path_factory.mktemp("fqn_temporal")
    project = tmp_path / "test_project"
    gs.create_project(project)

    with gs.setup.init(project) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=80, w=0, e=120, res=10)

        tools.r_mapcalc(expression="temp_1 = 1", overwrite=True)
        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="temperature_dataset",
            title="Temperature",
            description="PERMANENT dataset",
        )
        tools.t_register(
            type="raster",
            input="temperature_dataset",
            maps="temp_1",
            start="2026-01-01",
            increment="1 month",
            flags="i",
        )

        gs.run_command("g.mapset", mapset="user1", flags="c", env=session.env)
        tools.r_mapcalc(expression="precip_1 = 2", overwrite=True)
        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="precipitation_dataset",
            title="Precip",
            description="user1 dataset",
        )
        tools.t_register(
            type="raster",
            input="precipitation_dataset",
            maps="precip_1",
            start="2026-01-01",
            increment="1 month",
            flags="i",
        )

        gs.run_command("g.mapset", mapset="PERMANENT", env=session.env)

        yield SimpleNamespace(
            session=session,
            permanent_dataset="temperature_dataset",
            offpath_dataset="precipitation_dataset",
            offpath_id="precipitation_dataset@user1",
            offpath_mapset="user1",
        )


def test_add_mapset_includes_offpath_mapset(two_mapset_session):
    """add_mapset pulls an off-search-path mapset into an existing connection."""
    import grass.temporal as tgis

    tgis.init()
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()
    try:
        assert two_mapset_session.offpath_mapset not in dbif.tgis_mapsets
        dbif.add_mapset(two_mapset_session.offpath_mapset)
        assert two_mapset_session.offpath_mapset in dbif.tgis_mapsets
        # Idempotent: a second call must not raise or duplicate.
        dbif.add_mapset(two_mapset_session.offpath_mapset)
        assert two_mapset_session.offpath_mapset in dbif.tgis_mapsets
        # The off-path dataset is now queryable through this connection.
        sp = tgis.dataset_factory("strds", two_mapset_session.offpath_id)
        assert sp.is_in_db(dbif) is True
    finally:
        dbif.close()


def test_open_old_stds_offpath(two_mapset_session):
    """open_old_stds resolves a fully qualified name off the search path.

    Tools that go through open_old_stds are covered by this change. Tools that
    build their own database interface (e.g. t.info) are handled separately and
    are intentionally out of scope here.
    """
    import grass.temporal as tgis

    tgis.init()
    sp = tgis.open_old_stds(two_mapset_session.offpath_id, "strds")
    assert sp is not None
    assert sp.get_id() == two_mapset_session.offpath_id
