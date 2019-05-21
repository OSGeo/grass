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
        """Testing flag g with map lakes"""
        output_str = u"""n=15279
nmissing=0
nnull=0
min=1
max=15279
range=15278
sum=1.16732e+08
mean=7640
mean_abs=7640
population_stddev=4410.67
population_variance=1.9454e+07
population_coeff_variation=0.577312
sample_stddev=4410.81
sample_variance=1.94553e+07
kurtosis=-1.20024
skewness=-2.41826e-14"""
        v_univar = SimpleModule("v.univar", flags="g", map='lakes', column='cat')
        v_univar.run()
        self.assertLooksLike(actual=v_univar.outputs.stdout,
                             reference=output_str)

    def test_flage(self):
        """Testing flag e with map geology"""
        output_str = u"""number of features with non NULL attribute: 1832
number of missing attributes: 0
number of NULL attributes: 0
minimum: 166.947
maximum: 2.72948e+06
range: 2.72932e+06
sum: 7.88761e+07
mean: 43054.7
mean of absolute values: 43054.7
population standard deviation: 132689
population variance: 1.76064e+10
population coefficient of variation: 3.08187
sample standard deviation: 132725
sample variance: 1.7616e+10
kurtosis: 139.157
skewness: 9.7065
1st quartile: 3699.32
median (even number of cells): 10308.4
3rd quartile: 29259.1
90th percentile: 86449.7"""
        v_univar = SimpleModule('v.univar', map='geology', column='PERIMETER', flags='e')
        v_univar.run()
        self.assertLooksLike(actual=v_univar.outputs.stdout,
                             reference=output_str)
 
    def test_flagw(self):
        """Testing flag w with map lakes"""
        output_str = u"""number of features with non NULL attribute: 15279
number of missing attributes: 0
number of NULL attributes: 0
minimum: 2
maximum: 15280
range: 15278
sum: 5.76349e+11
mean: 6190.76
mean of absolute values: 6190.76"""
        v_univar = SimpleModule('v.univar', map='lakes', column='FULL_HYDRO', flags='w')
        v_univar.run()
        self.assertLooksLike(actual=v_univar.outputs.stdout,
                             reference=output_str)

    def test_flagd(self):
        """Testing flag d with map hospitals"""
        univar_string = u"""number of primitives: 160
number of non zero distances: 12561
number of zero distances: 0
minimum: 9.16773
maximum: 760776
range: 760767
sum: 2.69047e+09
mean: 214193
mean of absolute values: 214193
population standard deviation: 128505
population variance: 1.65136e+10
population coefficient of variation: 0.599953
sample standard deviation: 128511
sample variance: 1.6515e+10
kurtosis: 0.277564
skewness: 0.801646"""
        v_univar = SimpleModule('v.univar', map='hospitals', column='CITY', flags='d')
        v_univar.run()
        self.assertLooksLike(actual=v_univar.outputs.stdout,
                             reference=univar_string)
		
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
