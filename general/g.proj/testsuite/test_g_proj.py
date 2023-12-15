"""g.proj tests

(C) 2023 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:author: Anna Petrasova
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class GProjWKTTestCase(TestCase):
    """Test g.proj with WKT output"""

    def test_wkt_output(self):
        """Test if g.proj returns WKT"""
        module = SimpleModule("g.proj", flags="w")
        self.assertModule(module)
        self.assertIn("PROJCRS", module.outputs.stdout)


if __name__ == "__main__":
    test()
