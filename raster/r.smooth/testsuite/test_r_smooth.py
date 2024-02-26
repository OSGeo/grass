"""
Name:      r.smooth tests
Purpose:   Test corectness of output

Author:    Maris Nartiss
Copyright: (C) 2024 by Maris Nartiss and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import pathlib

# from tempfile import NamedTemporaryFile
# from grass.script import decode
# from grass.gunittest.checkers import keyvalue_equals

from grass.script import read_command
from grass.script.core import tempname
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class OptionTest(TestCase):
    """Test correctness of option logic"""

    @classmethod
    def setUpClass(cls):
        """Import sample maps with known properties"""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, res=1)

        cls.data_dir = os.path.join(pathlib.Path(__file__).parent.absolute(), "data")
        cls.in_rast_1 = tempname(10)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "simple1.ascii"),
            output=cls.in_rast_1,
        )
        cls.out_rast_1 = tempname(10)

    @classmethod
    def tearDownClass(cls):
        """Remove temporary data"""
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", name=cls.in_rast_1)
        cls.runModule("g.remove", flags="f", type="raster", name=cls.out_rast_1)

    def test_m(self):
        """A placeholder"""
        out = read_command(
            "r.smooth",
            input_=self.in_rast_1,
            output=self.out_rast_1,
            # memory=0.0022,
            memory=10,
            steps=10,
            threshold=2,
            lambda_=0.45,
            conditional="t",
            quiet=True,
        )
        print(out)


if __name__ == "__main__":
    test()
