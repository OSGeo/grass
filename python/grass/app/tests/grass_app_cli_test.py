import json
import subprocess
import sys

import pytest

from grass.app.cli import main


def test_cli_help_runs():
    """Check help of the main command"""
    with pytest.raises(SystemExit) as exception:
        main(["--help"])
    assert exception.value.code == 0


@pytest.mark.skipif(sys.platform.startswith("win"), reason="No man on Windows")
def test_man_subcommand_runs():
    """Check that man subcommand runs without an error"""
    assert main(["man", "g.region"]) == 0


def test_subcommand_man_no_page():
    """argparse gives 2 without parameters"""
    with pytest.raises(SystemExit) as exception:
        main(["man"])
    assert exception.value.code == 2


def test_subcommand_run_help():
    """Check help of a subcommand"""
    assert main(["run", "--help"]) == 0


def test_subcommand_run_no_tool():
    """argparse gives 2 without parameters"""
    assert main(["run"]) == 2


def test_subcommand_run_tool_help():
    """Check help of a tool"""
    assert main(["run", "g.region", "--help"]) == 0


def test_subcommand_run_tool_special_flag():
    """Check that a special flag is supported"""
    assert main(["run", "g.region", "--interface-description"]) == 0


def test_subcommand_run_tool_regular_run():
    """Check that a tool runs without error"""
    assert main(["run", "g.region", "-p"]) == 0


def test_subcommand_run_tool_failure_run():
    """Check that a tool produces non-zero return code"""
    assert main(["run", "g.region", "raster=does_not_exist"]) == 1


@pytest.mark.skipif(
    sys.platform.startswith("win"), reason="pytest capfd not reliable on Windows"
)
def test_subcommand_run_with_crs_as_epsg(capfd):
    """Check that CRS provided as EPSG is applied"""
    assert main(["run", "--crs", "EPSG:3358", "g.proj", "-p", "format=projjson"]) == 0
    result_dict = json.loads(capfd.readouterr().out)
    assert result_dict["id"]["authority"] == "EPSG"
    assert result_dict["id"]["code"] == 3358


def test_subcommand_run_with_crs_as_epsg_subprocess():
    """Check that CRS provided as EPSG is applied"""
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--crs",
            "EPSG:3358",
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
    assert result_dict["id"]["code"] == 3358


@pytest.mark.skipif(
    sys.platform.startswith("win"), reason="pytest capfd not reliable on Windows"
)
def test_subcommand_run_with_crs_as_pack(pack_raster_file4x5_rows, capfd):
    """Check that CRS provided as pack file is applied"""
    assert (
        main(
            [
                "run",
                "--crs",
                str(pack_raster_file4x5_rows),
                "g.proj",
                "-p",
                "format=projjson",
            ]
        )
        == 0
    )
    result_dict = json.loads(capfd.readouterr().out)
    assert result_dict["id"]["authority"] == "EPSG"
    assert result_dict["id"]["code"] == 3358


def test_subcommand_run_with_crs_as_pack_subprocess(pack_raster_file4x5_rows, capfd):
    """Check that CRS provided as pack file is applied"""
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--crs",
            str(pack_raster_file4x5_rows),
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
    assert result_dict["id"]["code"] == 3358


def test_create_lock_unlock(tmp_path):
    """Check that we can create, lock, unlock (smoke test)"""
    project = tmp_path / "test_1"
    assert main(["project", "create", str(project)]) == 0
    assert main(["mapset", "lock", str(project / "PERMANENT")]) == 0
    assert main(["mapset", "unlock", str(project / "PERMANENT")]) == 0


def test_create_mapset_lock_unlock(tmp_path):
    """Check that we can create, lock, unlock (smoke test)"""
    project = tmp_path / "test_1"
    assert main(["project", "create", str(project)]) == 0
    assert main(["mapset", "create", str(project / "data_1")]) == 0
    assert main(["mapset", "lock", str(project / "data_1")]) == 0
    assert main(["mapset", "unlock", str(project / "data_1")]) == 0


def test_create_mapset(tmp_path):
    """Check that we can create mapset and we can set its computational region"""
    project = tmp_path / "test_1"
    mapset = project / "data_1"
    assert main(["project", "create", str(project), "--crs", "EPSG:3358"]) == 0
    assert main(["mapset", "create", str(mapset)]) == 0
    assert mapset.exists()
    assert mapset.is_dir()
    rows = 13
    cols = 17
    assert (
        main(
            [
                "run",
                "--project",
                str(mapset),
                "g.region",
                f"rows={rows}",
                f"cols={cols}",
            ]
        )
        == 0
    )
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--project",
            str(mapset),
            "g.region",
            "-p",
            "format=json",
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    region = json.loads(result.stdout)
    assert region["rows"] == rows
    assert region["cols"] == cols
    # Also check it is linked with the project.
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--project",
            str(mapset),
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
    assert result_dict["id"]["code"] == 3358
    # And check that we are really using the newly created mapset,
    # so the computational region in the default mapset is different.
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--project",
            str(project),
            "g.region",
            "-p",
            "format=json",
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    region = json.loads(result.stdout)
    assert region["rows"] == 1
    assert region["cols"] == 1


def test_mapset_create_exists(tmp_path):
    """Check that creating mapset fails when mapset already exists"""
    project = tmp_path / "test_1"
    mapset = project / "data_1"
    assert main(["project", "create", str(project)]) == 0
    assert main(["mapset", "create", str(mapset)]) == 0
    assert main(["mapset", "create", str(mapset)]) == 1
    # There is no overwrite option for mapset yet, so we don't test that.


def test_create_overwrite(tmp_path):
    """Check that creating when project exists fails unless overwrite is True"""
    project = tmp_path / "test_1"
    assert main(["project", "create", str(project)]) == 0
    assert main(["project", "create", str(project)]) == 1
    assert main(["project", "create", "--overwrite", str(project)]) == 0


@pytest.mark.parametrize("epsg_code", [4326, 3358])
def test_create_crs_epsg(tmp_path, epsg_code):
    """Check that created project has the requested EPSG code"""
    project = tmp_path / "test_1"
    assert main(["project", "create", str(project), "--crs", f"EPSG:{epsg_code}"]) == 0
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "grass.app",
            "run",
            "--project",
            str(project),
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
