"""
Name:       r.report test
Purpose:    Tests r.report and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

import os
from grass.gunittest.case import TestCase

OPTION_1 = {
    "location": "nc_spm_08_grass7",
    "created": "Fri Dec 6 17:00:21 2013",
    "region": {
        "north": 279073.97546639,
        "south": 113673.97546639,
        "east": 798143.31179672,
        "west": 595143.31179672,
        "sn-res": 200,
        "ew-res": 200,
    },
    "mask": None,
    "maps": [
        {
            "name": "South-West Wake county",
            "description": "geology derived from vector map",
            "layer": "geology_30m",
            "type": "raster",
        }
    ],
    "category": {
        "categories": [
            {
                "category": 217,
                "description": "CZfg",
                "sqmi": 27.78,
                "acres": 17781.703,
                "categories": [
                    {
                        "category": 1,
                        "description": "developed",
                        "sqmi": 18,
                        "acres": 17781.703,
                    }
                ],
            }
        ],
        "total": {"sqmi": 77.60, "acres": 49668.182},
    },
}

OPTION_2 = {
    "location": "nc_spm_08_grass7",
    "created": "Fri Dec 6 17:00:21 2013",
    "region": {
        "north": 279073.97546639,
        "south": 113673.97546639,
        "east": 798143.31179672,
        "west": 595143.31179672,
        "sn-res": 200,
        "ew-res": 200,
    },
    "mask": None,
    "maps": {
        "geology_30m_raster": {
            "name": "South-West Wake county",
            "description": "geology derived from vector map",
        }
    },
    "categories": {
        "217": {
            "description": "CZfg",
            "sqmi": 27.78,
            "acres": 11781.703,
            "1": {"description": "developed", "sqmi": 18, "acres": 17781.703},
        },
        "total": {"sqmi": 77.60, "acres": 49668.182},
    },
}

OPTION_3 = {
    "location": "nc_spm_08_grass7",
    "created": "Fri Dec 6 17:00:21 2013",
    "region": {
        "north": 279073.97546639,
        "south": 113673.97546639,
        "east": 798143.31179672,
        "west": 595143.31179672,
        "sn-res": 200,
        "ew-res": 200,
    },
    "mask": None,
    "maps": [
        {
            "name": "South-West Wake county",
            "description": "geology derived from vector map",
            "layer": "geology_30m",
            "type": "raster",
        }
    ],
    "totals": {"sqmi": 77.60, "acres": 49668.182},
    "fields": ["description", "sqmi", "acres"],
    "categories": {
        "217": {
            "values": ["CZfg", 27.78, 11781.703],
            "categories": {"1": {"values": ["developed", 18, 17781.703]}},
        }
    },
}


class TestRasterreport(TestCase):
    outfile = "test_out.csv"

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()
        if os.path.isfile(cls.outfile):
            os.remove(cls.outfile)

    def test_flage(self):
        """Testing flag 'e' with map elevation"""
        self.assertModule("r.report", map="elevation", flags="e")

    def test_flagc(self):
        """Testing flag 'c' with map elevation"""
        self.assertModule("r.report", map="elevation", flags="c")

    def test_flagf(self):
        """Testing flag 'f' with map lakes"""
        self.assertModule("r.report", map="lakes", flags="f")

    def test_flagh(self):
        """Testing flag 'h' with map lakes"""
        self.assertModule("r.report", map="lakes", flags="h")

    def test_flagn(self):
        """Testing flag 'n' with map elevation"""
        self.assertModule("r.report", map="elevation", flags="n")

    def test_flaga(self):
        """Testing flag 'a' with map lakes"""
        self.assertModule("r.report", map="lakes", flags="a")

    def test_output(self):
        """Checking file existence"""
        self.assertModule("r.report", map="lakes", output=self.outfile)
        self.assertFileExists(self.outfile)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
