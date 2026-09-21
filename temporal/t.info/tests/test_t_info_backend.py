# SPDX-License-Identifier: GPL-2.0-or-later
"""Test the t.info report about the temporal database backend (-d flag)

The expected values capture the output of t.info as it is now, so that later
changes to the interface can be checked against the established behavior.
"""

import re

import grass.script as gs
from grass.tools import Tools

BACKEND_SHELL_KEYS = [
    "dbmi_python_interface",
    "dbmi_string",
    "sql_template_path",
    "tgis_version",
    "tgis_db_version",
    "creation_time",
]


def test_backend_plain(temporal_data):
    """The -d flag reports the backend in a human readable table"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(flags="d").text
    lines = text.splitlines()

    assert "Temporal DBMI backend information" in lines[0]
    assert " | DBMI Python interface:...... sqlite3" in lines
    assert any(line.startswith(" | Temporal database string:...") for line in lines)
    assert any(line.startswith(" | SQL template path:..........") for line in lines)
    assert any(line.startswith(" | tgis_version ..........") for line in lines)
    assert any(line.startswith(" | tgis_db_version ..........") for line in lines)
    assert any(line.startswith(" | creation_time ..........") for line in lines)
    assert lines[-1].startswith(" +---------")
    assert "" not in [line.strip() for line in lines]


def test_backend_shell(temporal_data):
    """The -d flag together with -g reports the backend as key value pairs"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(flags="dg").text
    info = gs.parse_key_val(text)

    assert list(info.keys()) == BACKEND_SHELL_KEYS
    assert info["dbmi_python_interface"] == "'sqlite3'"
    assert info["dbmi_string"].endswith("sqlite.db'")
    assert info["tgis_version"] == "'2'"
    assert info["tgis_db_version"] == "'3'"


def test_backend_shell_values_are_quoted(temporal_data):
    """All values of the backend information are quoted in the shell output"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(flags="dg").text

    for line in text.splitlines():
        assert re.fullmatch(r"\w+='.*'", line), line


def test_backend_ignores_input(temporal_data):
    """The -d flag reports the backend only, even if a dataset is given"""
    tools = Tools(session=temporal_data.session)
    with_input = tools.t_info(flags="d", type="strds", input="precip_abs1").text
    without_input = tools.t_info(flags="d").text

    assert with_input == without_input
    assert "precip_abs1" not in with_input
