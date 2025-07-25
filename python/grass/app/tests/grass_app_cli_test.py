import sys

import pytest

from grass.app.cli import main


def test_cli_help_runs():
    with pytest.raises(SystemExit) as exception:
        main(["--help"])
    assert exception.value.code == 0


@pytest.mark.skipif(sys.platform.startswith("win"), reason="No man on Windows")
def test_man_subcommand_runs():
    assert main(["man", "g.region"]) == 0


def test_subcommand_run_help():
    assert main(["run", "--help"]) == 0


def test_subcommand_run_tool_special_flag():
    assert main(["run", "g.region", "--interface-description"]) == 0


def test_subcommand_run_tool_regular_run():
    assert main(["run", "g.region", "-p"]) == 0


def test_subcommand_run_tool_failure_run():
    assert main(["run", "g.region", "raster=does_not_exist"]) == 1
