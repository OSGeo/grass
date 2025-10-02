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
    assert main(["run", "--crs", "EPSG:3358", "g.proj", "-p", "format=json"]) == 0
    assert json.loads(capfd.readouterr().out)["srid"] == "EPSG:3358"


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
            "format=json",
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    assert json.loads(result.stdout)["srid"] == "EPSG:3358"


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
                "format=json",
            ]
        )
        == 0
    )
    assert json.loads(capfd.readouterr().out)["srid"] == "EPSG:3358"


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
            "format=json",
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    assert json.loads(result.stdout)["srid"] == "EPSG:3358"


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


def test_create_overwrite(tmp_path):
    """Check that creating when project exists fails unless overwrite is True"""
    project = tmp_path / "test_1"
    assert main(["project", "create", str(project)]) == 0
    assert main(["project", "create", str(project)]) == 1
    assert main(["project", "create", "--overwrite", str(project)]) == 0


@pytest.mark.parametrize("crs", ["EPSG:4326", "EPSG:3358"])
def test_create_crs_epsg(tmp_path, crs):
    """Check that created project has the requested EPSG"""
    project = tmp_path / "test_1"
    assert main(["project", "create", str(project), "--crs", crs]) == 0
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
            "format=json",
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    assert json.loads(result.stdout)["srid"] == crs
