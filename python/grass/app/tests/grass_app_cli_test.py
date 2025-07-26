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
