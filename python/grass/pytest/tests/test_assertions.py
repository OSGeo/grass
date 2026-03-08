import grass.script as gs
from grass.pytest.assertions import rasterExists


def test_rasterExists(session):
    gs.run_command("r.mapcalc", expression="test_map = 1", env=session)

    assert rasterExists("test_map", env=session)
    assert not rasterExists("missing_map", env=session)