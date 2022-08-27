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
import csv
import pytest

import grass.script as gs



def check_separators(format, mapsets):
    SEPARATORS = ["newline", "space", "comma", "tab", "pipe", None]

    for sep in SEPARATORS:
        text = gs.read_command("g.mapsets", format=format, separator=fsep, flags="l")
        if sep == "newline":
            assert text == "\n".join(mapsets) + "\n"
        elif sep == "space":
            assert text == " ".join(mapsets) + "\n"
        elif sep == "comma":
            assert text == ",".join(mapsets) + "\n"
        elif sep == "tab":
            assert text == "\t".join(mapsets) + "\n"
        elif sep == "pipe":
            assert text == "|".join(mapsets) + "\n"
        else:
            # Default vallue
            assert text == "|".join(mapsets) + "\n"

def test_plain_output(simple_dataset):
    """Test that the separators are properly applied""" 
    check_separators("plain", simple_dataset.mapsets)


def test_json_ouput(simple_dataset):
    """ JSON format """
    text = gs.read_command("g.mapsets", format="json", flags="l")
    print(f"Text JSON: {text}")
    data = json.loads(text)
    assert list(data.keys()) == ["mapsets"]
    assert isinstance(data["mapsets"], list)
    assert len(data["mapsets"]) == 4
    for mapset in simple_dataset.mapsets:
        assert mapset in data["mapsets"]

def test_csv_output(simple_dataset):
    """ CSV format """
    text = gs.read_command("g.mapsets", format="csv", flags="l")
    assert text == ",".join(simple_dataset.mapsets) + "\n"

# def test_vertical_output(simple_dataset):
#     """ Vertical format """
#     text = gs.read_command("g.mapsets", format="vertical", flags="l")
#     assert text == "\n".join(simple_dataset.mapsets) + "\n"


if __name__ == "__main__":
    test()
