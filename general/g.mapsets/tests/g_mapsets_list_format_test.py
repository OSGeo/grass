############################################################################
#
# MODULE:       Test of g.mapsets
# AUTHOR(S):    Corey White <smortopahri gmail com>
# PURPOSE:      Test parsing and structure of outputs
# COPYRIGHT:    (C) 2022 by Corey White the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

"""Test parsing and structure of outputs from g.mapsets"""

import json
import sys
import pytest
import grass.script as gs
from grass.script import utils as gutils

SEPARATORS = ["newline", "space", "comma", "tab", "pipe", ","]


def _check_parsed_list(mapsets, text, sep="|"):
    """Asserts to run on for each separator"""
    parsed_list = text.splitlines() if sep == "\n" else text.split(sep)
    mapsets_len = len(mapsets)

    assert len(parsed_list) == mapsets_len
    assert text == sep.join(mapsets) + "\n"


@pytest.mark.xfail(
    sys.platform == "win32",
    reason="universal_newlines or text subprocess option not used",
)
@pytest.mark.parametrize("separator", SEPARATORS)
def test_plain_list_output(simple_dataset, separator):
    """Test that the separators are properly applied with list flag"""
    mapsets = simple_dataset.mapsets
    text = gs.read_command(
        "g.mapsets",
        format="plain",
        separator=separator,
        flags="l",
        env=simple_dataset.session.env,
    )
    _check_parsed_list(mapsets, text, gutils.separator(separator))


@pytest.mark.xfail(
    sys.platform == "win32",
    reason="universal_newlines or text subprocess option not used",
)
@pytest.mark.parametrize("separator", SEPARATORS)
def test_plain_print_output(simple_dataset, separator):
    """Test that the separators are properly applied with print flag"""
    mapsets = simple_dataset.accessible_mapsets
    text = gs.read_command(
        "g.mapsets",
        format="plain",
        separator=separator,
        flags="p",
        env=simple_dataset.session.env,
    )
    _check_parsed_list(mapsets, text, gutils.separator(separator))


def test_json_list_output(simple_dataset):
    """Check list of mapsets in JSON format"""
    text = gs.read_command(
        "g.mapsets", format="json", flags="l", env=simple_dataset.session.env
    )
    data = json.loads(text)
    assert list(data.keys()) == ["mapsets"]
    assert isinstance(data["mapsets"], list)
    assert len(data["mapsets"]) == 4
    for mapset in simple_dataset.mapsets:
        assert mapset in data["mapsets"]


def test_json_print_output(simple_dataset):
    """Check search path mapsets in JSON format"""
    text = gs.read_command(
        "g.mapsets", format="json", flags="p", env=simple_dataset.session.env
    )
    data = json.loads(text)
    assert list(data.keys()) == ["mapsets"]
    assert isinstance(data["mapsets"], list)
    assert len(data["mapsets"]) == 2
    for mapset in simple_dataset.accessible_mapsets:
        assert mapset in data["mapsets"]
