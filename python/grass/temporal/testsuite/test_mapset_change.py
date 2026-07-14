"""Test wether a switch of mapset is taken into account

SPDX-FileCopyrightText: 2025 by the GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later.

:authors: Laurent Courty
"""

import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.temporal as tgis


class TestMapsetChange(TestCase):
    def test_change_mapset(self):
        gs.run_command("g.mapset", mapset="mapset_test1", flags="c")
        mapsets = gs.parse_command("g.mapsets", flags="p", format="json")["mapsets"]
        assert mapsets == ["mapset_test1", "PERMANENT"]
        tgis.init()
        gs.run_command("g.mapset", mapset="mapset_test2", flags="c")
        tgis.init()  # If the mapset change is not working, the test fails here
        mapsets = gs.parse_command("g.mapsets", flags="p", format="json")["mapsets"]
        assert mapsets == ["mapset_test2", "PERMANENT"]


if __name__ == "__main__":
    test()
