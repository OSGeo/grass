"""
Name:       v.profile test
Purpose:    Tests v.profile input parsing and simle output generation.
            Uses NC Basic data set.

Author:     Maris Nartiss
Copyright:  (C) 2017 by Maris Nartiss and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
TODO:       Convert to synthetic dataset. It would allow to shorten output sample length.
            Cover more input/output combinations.
"""

import os

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

output_full = """Number|Distance|cat|feature_id|featurenam|class|st_alpha|st_num|county|county_num|primlat_dm|primlon_dm|primlatdec|primlondec|srclat_dms|srclon_dms|srclatdec|srclondec|elev_m|map_name
1|19537.97|572|986138|"Greshams Lake"|"Reservoir"|"NC"|37|"Wake"|183|"078:34:31W"|"35:52:43N"|35.878484|-78.57528|""|""|""|""|77|"Wake Forest"
2|19537.97|1029|999647|"Greshams Lake Dam"|"Dam"|"NC"|37|"Wake"|183|"078:34:31W"|"35:52:43N"|35.878484|-78.57528|""|""|""|""|77|"Wake Forest"
"""

output_nocols = """1|19537.97|572|986138|"Greshams Lake"|"Reservoir"|"NC"|37|"Wake"|183|"078:34:31W"|"35:52:43N"|35.878484|-78.57528|""|""|""|""|77|"Wake Forest"
2|19537.97|1029|999647|"Greshams Lake Dam"|"Dam"|"NC"|37|"Wake"|183|"078:34:31W"|"35:52:43N"|35.878484|-78.57528|""|""|""|""|77|"Wake Forest"
"""

output_filtered = """Number|Distance|cat|feature_id|featurenam|class|st_alpha|st_num|county|county_num|primlat_dm|primlon_dm|primlatdec|primlondec|srclat_dms|srclon_dms|srclatdec|srclondec|elev_m|map_name
1|19537.97|1029|999647|"Greshams Lake Dam"|"Dam"|"NC"|37|"Wake"|183|"078:34:31W"|"35:52:43N"|35.878484|-78.57528|""|""|""|""|77|"Wake Forest"
"""

output_coords = """Number|Distance|cat|feature_id|featurenam|class|st_alpha|st_num|county|county_num|primlat_dm|primlon_dm|primlatdec|primlondec|srclat_dms|srclon_dms|srclatdec|srclondec|elev_m|map_name
1|24.34|572|986138|"Greshams Lake"|"Reservoir"|"NC"|37|"Wake"|183|"078:34:31W"|"35:52:43N"|35.878484|-78.57528|""|""|""|""|77|"Wake Forest"
2|24.34|1029|999647|"Greshams Lake Dam"|"Dam"|"NC"|37|"Wake"|183|"078:34:31W"|"35:52:43N"|35.878484|-78.57528|""|""|""|""|77|"Wake Forest"
"""


class TestProfiling(TestCase):
    in_points = 'points_of_interest'
    in_map = 'roadsmajor'
    where = "cat='354'"
    prof_ponts = (647952, 236176, 647950, 236217)
    outfile = 'test_out.csv'

    def testParsing(self):
        """Test input parameter parsing"""
        # Positive buffer size
        self.assertModuleFail('v.profile', input=self.in_points, coordinates=self.prof_ponts, buffer=0)
        # Where without input map
        self.assertModuleFail('v.profile', input=self.in_points, coordinates=self.prof_ponts, buffer=10,
            profile_where=self.where)
        # Both coords and input vector is not supported
        self.assertModuleFail('v.profile', input=self.in_points, coordinates=self.prof_ponts, buffer=10,
            profile_map=self.in_map)
        # Neither of coords or input line are provided
        self.assertModuleFail('v.profile', input=self.in_points, buffer=10)
        # Two coordinate parirs are reqiured
        self.assertModuleFail('v.profile', input=self.in_points, coordinates=(647952, 236176), buffer=10)
        # Wrong object match count
        self.assertModuleFail('v.profile', input=self.in_points, profile_map=self.in_map, buffer=10,
            profile_where="ROAD_NAME='wrong'")
        self.assertModuleFail('v.profile', input=self.in_points, profile_map=self.in_map, buffer=10,
            profile_where="ROAD_NAME='US-1'")
        self.assertModuleFail('v.profile', input=self.in_points, profile_map=self.in_map, buffer=10)
        self.assertModuleFail('v.profile', input=self.in_points, coordinates=self.prof_ponts, buffer=10,
            where="class='wrong'")
        # Wrong output name
        self.assertModuleFail('v.profile', input=self.in_points, coordinates=self.prof_ponts, buffer=10,
            map_output='5cats')
    
    def testFileExists(self):
        """This function checks if the output file is written correctly"""
        self.runModule('v.profile', input=self.in_points, coordinates=self.prof_ponts, buffer=10,
            output=self.outfile)
        self.assertFileExists(self.outfile)
        if os.path.isfile(self.outfile):
            os.remove(self.outfile)
    
    def testOutput(self):
        """Test correctness of output"""
        # Normal output
        vpro = SimpleModule('v.profile', input=self.in_points, profile_map=self.in_map, buffer=200,
            profile_where=self.where)
        vpro.run()
        self.assertLooksLike(reference=output_full, actual=vpro.outputs.stdout)
        # Without column names
        vpro = SimpleModule('v.profile', input=self.in_points, profile_map=self.in_map, buffer=200,
            profile_where=self.where, c=True)
        vpro.run()
        self.assertLooksLike(reference=output_nocols, actual=vpro.outputs.stdout)
        # Filtering input points
        vpro = SimpleModule('v.profile', input=self.in_points, profile_map=self.in_map, buffer=200,
            where="class='Dam'", profile_where=self.where)
        vpro.run()
        self.assertLooksLike(reference=output_filtered, actual=vpro.outputs.stdout)
        # Providing profiling line from coordinates
        vpro = SimpleModule('v.profile', input=self.in_points, coordinates=self.prof_ponts, buffer=200)
        vpro.run()
        self.assertLooksLike(reference=output_coords, actual=vpro.outputs.stdout)


if __name__ == '__main__':
    test()
