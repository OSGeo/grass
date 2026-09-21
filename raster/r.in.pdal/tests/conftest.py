import os
import shutil
import subprocess

import pytest

import grass.script as gs


@pytest.fixture(scope="module")
def point_cloud_files(tmp_path_factory):
    """Create LAS and COPC versions of a synthetic point cloud.

    Points form an 18x18 grid with 1 unit spacing starting at 0.5 and
    z = x + y, georeferenced as EPSG:3358.
    """
    if shutil.which("pdal") is None:
        pytest.skip("pdal command line tool not available")
    tmp_path = tmp_path_factory.mktemp("point_data")
    csv_file = tmp_path / "points.csv"
    las_file = tmp_path / "points.las"
    copc_file = tmp_path / "points.copc.laz"
    lines = ["X,Y,Z"]
    for i in range(18):
        for j in range(18):
            x = i + 0.5
            y = j + 0.5
            lines.append(f"{x},{y},{x + y}")
    csv_file.write_text("\n".join(lines) + "\n")
    subprocess.run(
        [
            "pdal",
            "translate",
            "-i",
            str(csv_file),
            "-o",
            str(las_file),
            "-r",
            "text",
            "-w",
            "las",
            "--writers.las.a_srs=EPSG:3358",
        ],
        check=True,
        capture_output=True,
    )
    try:
        subprocess.run(
            ["pdal", "translate", "-i", str(las_file), "-o", str(copc_file)],
            check=True,
            capture_output=True,
        )
    except subprocess.CalledProcessError:
        pytest.skip("PDAL does not support writing COPC")
    return {"las": las_file, "copc": copc_file}


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Active session in a project with the CRS of the test data"""
    tmp_path = tmp_path_factory.mktemp("r_in_pdal_project")
    project = tmp_path / "test"
    gs.create_project(project, epsg="3358")
    with gs.setup.init(project, env=os.environ.copy()) as session:
        if shutil.which("r.in.pdal", path=session.env["PATH"]) is None:
            pytest.skip("r.in.pdal not available (GRASS built without PDAL)")
        yield session
