"""
Name:       v.vect.stats test
Purpose:    Tests v.vect.stats and its flags/options.
	
Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
	            License (>=v2). Read the file COPYING that comes with GRASS
	            for details.
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

class Testrr(TestCase):
    input='hospitals'
    areas='zipcodes_wake'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
	
    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def test_sum(self):
        """Testing method sum"""
        string="""area_cat|count|sum
        1|0|null
        2|0|null
        3|0|null
        4|0|null
        5|0|null
        6|0|null
        7|0|null
        8|0|null
        9|1|7
        10|0|null
        """
        v_vect_stats = SimpleModule('v.vect.stats', points=self.input, areas=self.areas, method='sum', count_column='num_points', 
                                    stats_column='avg_elev', points_column='cat')
        v_vect_stats.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=v_vect_stats.outputs.stdout)


    def test_average(self):
        """Testing method average"""
        string="""area_cat|count|average
        1|1|2681
        2|0|null
        3|2|3958.5
        4|0|null
        5|0|null
        6|8|4012
        7|7|4185.42857142857
        8|19|4396.78947368421
        9|4|4222
        10|3|4400.33333333333
        """
        v_vect_stats = SimpleModule('v.vect.stats', points=self.input, areas=self.areas, method='average', count_column='num_points', 
                                    stats_column='avg_elev', points_column='cat')
        v_vect_stats.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=v_vect_stats.outputs.stdout)

    def test_median(self):
        """Testing method variance"""
        string="""area_cat|count|variance
        1|1|0
        2|0|null
        3|2|702.25
        4|0|null
        5|0|null
        6|8|7639
        7|7|2661.38775510204
        8|19|69198.7977839335
        9|4|42.5
        10|3|3968.22222222222
        """
        v_vect_stats = SimpleModule('v.vect.stats', points=self.input, areas=self.areas, method='variance', count_column='num_points', 
                                    stats_column='avg_elev', points_column='cat')
        v_vect_stats.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=v_vect_stats.outputs.stdout)
        
    def test_mincat(self):
        """Testing method min_cat"""
        string="""area_cat|count|range
        1|1|0
        2|0|null
        3|2|53
        4|0|null
        5|0|null
        6|8|255
        7|7|168
        8|19|892
        9|4|17
        10|3|152
        """ 
        v_vect_stats = SimpleModule('v.vect.stats', points=self.input, areas=self.areas, method='range', count_column='num_points', 
                                    stats_column='avg_elev', points_column='cat')
        v_vect_stats.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=v_vect_stats.outputs.stdout)

    def test_maxcat(self):
        """Testing method max_cat"""
        string="""area_cat|count|max_cat
        1|0|null
        2|0|null
        3|0|null
        4|0|null
        5|0|null
        6|0|null
        7|0|null
        8|0|null
        9|1|7
        10|0|null
        """
        v_vect_stats = SimpleModule('v.vect.stats', points=self.input, areas=self.areas, method='max_cat', count_column='num_points', 
                                    stats_column='avg_elev', points_column='cat')
        v_vect_stats.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=v_vect_stats.outputs.stdout)

    def test_mode(self):
        """Testing method mode """
        string="""area_cat|count|mode
        1|0|null
        2|0|null
        3|0|null
        4|0|null
        5|0|null
        6|0|null
        7|0|null
        8|0|null
        9|1|7
        10|0|null
        """
        v_vect_stats = SimpleModule('v.vect.stats', points=self.input, areas=self.areas, method='mode', count_column='num_points', 
                                    stats_column='avg_elev', points_column='cat')
        v_vect_stats.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=v_vect_stats.outputs.stdout)

if __name__ == '__main__':
    test()
