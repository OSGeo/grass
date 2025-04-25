import pytest

from grass.app.cli import main


def test_cli_help_runs():
    with pytest.raises(SystemExit) as exception:
        main(["--help"])
    assert exception.value.code == 0


def test_man_subcommand_runs():
    assert main(["man", "g.region"]) == 0
