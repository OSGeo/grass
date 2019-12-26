"""
Name:       r.what test
Purpose:    Tests r.what and its flags/options.
	
Author:     Sunveer Singh, Google Code-in 2018
Copyright:  (C) 2018 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
	            License (>=v2). Read the file COPYING that comes with GRASS
	            for details.
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

class Testrr(TestCase):
    input='elevation'
    coordinates=(633614.08,224125.12,632972.36,225382.87)
    points='comm_colleges'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.input, flags='p')

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()


    def test_flag_n(self):
        """Testing output with flag n"""
        string="""1|145096.8591495|154534.264883875||*
        2|616341.4371495|146049.750883875||*
        3|410595.7191495|174301.828883875||*
        4|734153.6871495|169168.437883875||*
        """
        r_what = SimpleModule('r.what', map=self.input, coordinates=self.coordinates, flags='n')
        r_what.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=r_what.outputs.stdout)

    def test_flag_f(self):
        """Testing output with flag f"""
        string="""5|706338.2501495|54889.417883875||*
        6|758009.7501495|112019.898883875||*
        7|754002.7501495|200902.234883875||*
        8|704771.7501495|183364.484883875||*"""
        r_what = SimpleModule('r.what', map=self.input, coordinates=self.coordinates, flags='f')
        r_what.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=r_what.outputs.stdout)

    def test_flag_r(self):
        """Testing output with flag r"""
        string="""9|399187.0631495|220018.859883875||*
        10|685098.9371495|33282.089883875||*
        11|577750.8131495|257153.109883875||*
        12|794095.5621495|199742.671883875||*"""
        r_what = SimpleModule('r.what', map=self.input, coordinates=self.coordinates, flags='r')
        r_what.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=r_what.outputs.stdout)

    def test_flag_i(self):
        """Testing output with flag i"""
        string="""13|634688.2501495|100629.616883875||*
        14|287638.7811495|207582.624883875||*
        15|366218.5321495|222940.625883875||*
        16|385212.4371495|236593.109883875||*"""
        r_what = SimpleModule('r.what', map=self.input, coordinates=self.coordinates, flags='i')
        r_what.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=r_what.outputs.stdout)

    def test_flag_c(self):
        """Testing output with flag c"""
        string="""17|628137.4371495|63995.550883875||*
        18|782600.5631495|152698.890883875||*
        19|502813.9381495|235232.577883875||*
        20|705922.6251495|136589.359883875||*"""
        r_what = SimpleModule('r.what', map=self.input, coordinates=self.coordinates, flags='c')
        r_what.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=r_what.outputs.stdout)

    def test_flag_v(self):
        """Testing output with flag v"""
        string="""21|620397.8131495|246847.640883875||*
        22|738465.3751495|237233.983883875||*
        23|708944.7501495|247632.296883875||*
        24|526666.6871495|249780.312883875||*"""
        r_what = SimpleModule('r.what', map=self.input, coordinates=self.coordinates, flags='v', points=self.points)
        r_what.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=r_what.outputs.stdout)


if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
