"""
Name:       v.univar test
Purpose:    Tests v.univar and its flags/options.
	
Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
	
class TestProfiling(TestCase):

    def test_flagg(self):
        """Testing flag g with map bridges"""
        output_str="""n=2537
        nmissing=0
        nnull=0
        min=1
        max=2537
        range=2536
        sum=3.21945e+06
        mean=1269
        mean_abs=1269
        population_stddev=732.369
        population_variance=536364
        population_coeff_variation=0.577123
        sample_stddev=732.513
        sample_variance=536576
        kurtosis=-1.20142
        skewness=0"""
        v_univar = SimpleModule("v.univar", flags="g", map='census', column='cat')
        self.assertModuleKeyValue(module='v.univar', map='census', column='cat', flags='g', reference=output_str,
	                                  precision=2, sep="=")

    def test_flage(self):
        """Testing flag e with map geology"""
        self.assertModule('v.univar', map='geology', column='PERIMETER', flags='e')
 
    def test_flagw(self):
        """Testing flag w with map lakes"""
        self.assertModule('v.univar', map='hospitals', column='cat', flags='w')

    def test_flagd(self):
        """Testing flag d with map hospitals"""
        univar_string="""n=160
        nmissing=0
        nnull=0
        min=1
        max=160
        range=159
        sum=12880
        mean=80.5
        mean_abs=80.5"""
        self.assertModule('v.univar', map='hospitals', column='CITY', flags='d')
		
    def test_output(self):
        """Testing output of v.univar"""
        univar_string="""n=160
        min=1
        max=160
        range=159
        mean=80.5
        sum=12880"""
        self.assertVectorFitsUnivar(map='hospitals', column='cat', reference=univar_string, precision=3)

    def test_output2(self):
        """Testing output of v.univar"""
        univar_string="""n=357
        min=1
        max=357
        range=356
        mean=179.82
        sum=63836"""
        self.assertVectorFitsUnivar(map='roadsmajor', column='MAJORRDS_', reference=univar_string, precision=3)

if __name__ == '__main__':
	    test()
