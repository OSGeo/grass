"""Pytest fixtures for r.proj tests.

Creates and initialises a GRASS session from scratch using a temporary
directory.  This is the ONLY place where gs.setup.init is called – every
test that needs GRASS must request the ``grass_session`` fixture (directly
or via ``temp_region``).
"""

import pytest
import grass.script as gs


@pytest.fixture(scope="session")
def grass_session(tmp_path_factory):
    """Create a temporary GRASS project and initialise a session in it.

    scope=session  →  runs once for the entire pytest run, shared by all tests.
    """
    project_path = str(tmp_path_factory.mktemp("grass_project"))

    # Create a new project with a simple projected CRS (UTM zone 17N)
    gs.create_project(project_path, epsg="26917")

    # Initialise GRASS session – returns a session object (context manager)
    session = gs.setup.init(project_path)

    yield project_path  # tests run here

    # Tear down the session so tmp cleanup works cleanly
    session.finish()


@pytest.fixture()
def temp_region(grass_session):  # noqa: ARG001
    """Save the current computational region; restore it after the test.

    Requesting this fixture also pulls in grass_session automatically.
    """
    saved = gs.parse_command("g.region", flags="gp")
    yield
    gs.run_command(
        "g.region",
        n=saved["n"],
        s=saved["s"],
        e=saved["e"],
        w=saved["w"],
        nsres=saved["nsres"],
        ewres=saved["ewres"],
    )
