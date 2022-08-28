############################################################################
#
# MODULE:       Test of g.mapsets
# AUTHOR(S):    Corey White <smortopahri gmail com>
# PURPOSE:      Test parsing and structure of CSV and JSON outputs
# COPYRIGHT:    (C) 2022 by Corey White the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

"""Test parsing and structure of CSV and JSON outputs from g.mapsets"""

import json
import pytest
import grass.script as gs


@pytest.mark.parametrize("flag", ["p", "l", "lp"])
@pytest.mark.parametrize(
    "separator", ["newline", "space", "comma", "tab", "pipe", ",", None]
)
def test_plain_output(simple_dataset, separator, flag):
    """Test that the separators are properly applied"""
    mapsets = simple_dataset.mapsets
    mapsets_len = len(mapsets)
    if flag == "p":
        mapsets = simple_dataset.accessible_mapsets
        mapsets_len = len(mapsets)

    text = gs.read_command("g.mapsets", format="plain", separator=separator, flags=flag)

    def _check_parsed_list(text, sep):
        """Asserts to run on for each separator"""
        parsed_list = text.splitlines()

        # Make sure new line conditions are handled correctly
        if sep != "\n":
            parsed_list = text.split(sep)
            if flag == "l":
                assert parsed_list[-1] == "test3\n"
            if flag == "p":
                assert parsed_list[-1] == "PERMANENT\n"
        else:
            if flag == "l":
                assert parsed_list[-1] == "test3"
            if flag == "p":
                assert parsed_list[-1] == "PERMANENT"

        # Check the beginning of list is correct
        if flag == "p":
            assert parsed_list[0] == "test3"

        if flag == "l":
            assert parsed_list[0] == "PERMANENT"

        assert len(parsed_list) == mapsets_len
        assert text == sep.join(mapsets) + "\n"

    if separator == "newline":
        _check_parsed_list(text, "\n")
    elif separator == "space":
        _check_parsed_list(text, " ")
    elif separator == "comma":
        _check_parsed_list(text, ",")
    elif separator == "tab":
        assert text == "\t".join(mapsets) + "\n"
    elif separator == "pipe":
        assert text == "|".join(mapsets) + "\n"
    elif separator == ",":
        assert text == ",".join(mapsets) + "\n"
    else:
        # Default vallue
        assert text == "|".join(mapsets) + "\n"


def test_json_ouput(simple_dataset):
    """JSON format"""
    text = gs.read_command("g.mapsets", format="json", flags="l")
    data = json.loads(text)
    assert list(data.keys()) == ["mapsets"]
    assert isinstance(data["mapsets"], list)
    assert len(data["mapsets"]) == 4
    for mapset in simple_dataset.mapsets:
        assert mapset in data["mapsets"]


def test_accessible_mapsets_json_ouput(simple_dataset):
    """JSON format"""
    text = gs.read_command("g.mapsets", format="json", flags="p")
    data = json.loads(text)
    assert list(data.keys()) == ["mapsets"]
    assert isinstance(data["mapsets"], list)
    assert len(data["mapsets"]) == 2
    for mapset in simple_dataset.accessible_mapsets:
        assert mapset in data["mapsets"]
