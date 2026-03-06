import os
import pytest
import grass.script as gs


@pytest.fixture
def session(tmp_path):
    project = tmp_path / "test_proj"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session.env

# import pytest
# import grass.script as gs


# @pytest.fixture
# def session(tmp_path):
#     project = tmp_path / "project"

#     # Create new location
#     gs.create_project(project)

#     # Start GRASS session
#     with gs.setup.init(project) as session:
#         yield session