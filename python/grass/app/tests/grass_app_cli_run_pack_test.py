import json
import sys
import subprocess

import pytest


def test_run_with_crs_as_pack_as_input(pack_raster_file4x5_rows):
    """Check that we accept pack as input."""
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--crs",
            str(pack_raster_file4x5_rows),
            "r.univar",
            f"map={pack_raster_file4x5_rows}",
            "format=json",
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    assert (
        json.loads(result.stdout)["cells"] == 1
    )  # because we don't set the computational region


@pytest.mark.parametrize("epsg_code", [3358, 4326])
@pytest.mark.parametrize("extension", [".grass_raster", ".grr", ".rpack"])
def test_run_with_crs_as_pack_as_output(tmp_path, epsg_code, extension):
    """Check outputting pack with different CRSs and extensions"""
    raster = tmp_path / f"test{extension}"
    subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--crs",
            f"EPSG:{epsg_code}",
            "r.mapcalc.simple",
            "expression=row() + col()",
            f"output={raster}",
        ],
        check=True,
    )
    assert raster.exists()
    assert raster.is_file()
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--crs",
            str(raster),
            "g.proj",
            "-p",
            "format=projjson",
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    result_dict = json.loads(result.stdout)
    assert result_dict["id"]["authority"] == "EPSG"
    assert result_dict["id"]["code"] == epsg_code


def test_run_with_crs_as_pack_with_multiple_steps(tmp_path):
    """Check that we accept pack as both input and output.

    The extension is only tested for the output.
    Tests basic properties of the output.
    """
    crs = "EPSG:3358"
    extension = ".grass_raster"
    raster_a = tmp_path / f"test_a{extension}"
    raster_b = tmp_path / f"test_b{extension}"
    subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--crs",
            crs,
            "r.mapcalc.simple",
            "expression=row() + col()",
            f"output={raster_a}",
        ],
        check=True,
    )
    assert raster_a.exists()
    assert raster_a.is_file()
    subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--crs",
            crs,
            "r.mapcalc.simple",
            "expression=1.5 * A",
            f"a={raster_a}",
            f"output={raster_b}",
        ],
        check=True,
    )
    assert raster_b.exists()
    assert raster_b.is_file()
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--crs",
            crs,
            "r.univar",
            f"map={raster_b}",
            "format=json",
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    assert (
        json.loads(result.stdout)["cells"] == 1
    )  # because we don't set the computational region
