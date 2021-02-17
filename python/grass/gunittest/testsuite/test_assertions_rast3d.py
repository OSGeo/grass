# -*- coding: utf-8 -*-

"""
Tests assertion methods for 3D rasters.
"""

from grass.exceptions import CalledModuleError
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestRaster3dMapAssertions(TestCase):
    # pylint: disable=R0904
    constant_map = 'raster3d_assertions_constant'
    rcd_increasing_map = 'raster3d_assertions_rcd_increasing'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # TODO: here we should actually not call self.runModule but call_module
        cls.runModule('g.region', n=200, s=100, e=400, w=200,
                      t=500, b=450, res3=1)
        cls.runModule('r3.mapcalc', expression='%s = 155' % cls.constant_map)
        cls.runModule('r3.mapcalc',
                      expression='%s = row() + col() + depth()' % cls.rcd_increasing_map)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        # TODO: input as list does not work, why?
        cls.runModule('g.remove', flags='f', type='raster_3d',
                      name=','.join([cls.constant_map, cls.rcd_increasing_map]))

    def test_assertRaster3dFitsUnivar(self):
        reference = dict(n=1000000,
                         null_cells=0,
                         cells=1000000,
                         min=155,
                         max=155,
                         range=0,
                         mean=155,
                         mean_of_abs=155,
                         stddev=0,
                         variance=0,
                         coeff_var=0,
                         sum=155000000)
        self.assertRaster3dFitsUnivar(self.constant_map, reference=reference,
                                      precision=0.000001)
        self.assertRaises(self.failureException,
                          self.assertRaster3dFitsUnivar,
                          self.rcd_increasing_map,
                          reference=reference, precision=1)
        self.assertRaises(ValueError,
                          self.assertRaster3dFitsUnivar,
                          self.constant_map, reference=dict(a=4, b=5, c=6))
        self.assertRaises(CalledModuleError,
                          self.assertRaster3dFitsUnivar,
                          'does_not_exists', reference=dict(a=4, b=5, c=6))

    def test_assertRaster3dFitsInfo(self):
        reference = dict(north=200,
                         south=100,
                         east=400,
                         west=200,
                         bottom=450,
                         top=500,
                         nsres=1,
                         ewres=1,
                         tbres=1,
                         rows=100,
                         cols=200,
                         depths=50)
        self.assertRaster3dFitsInfo(self.constant_map, reference=reference)

        reference['north'] = 500
        self.assertRaises(self.failureException,
                          self.assertRaster3dFitsInfo,
                          self.constant_map, reference=reference)
        self.assertRaises(ValueError,
                          self.assertRaster3dFitsInfo,
                          self.constant_map, reference=dict(a=5))

    def test_common_values_info_univar(self):
        minmax = dict(min=3, max=350)
        self.assertRaster3dFitsUnivar(self.rcd_increasing_map,
                                      minmax, precision=0.01)
        self.assertRaster3dFitsInfo(self.rcd_increasing_map,
                                    minmax, precision=0.01)

    def test_string_as_parameter(self):
        self.assertRaster3dFitsInfo(self.constant_map,
                                    reference="max=155", precision=1)
        self.assertRaster3dFitsUnivar(self.rcd_increasing_map,
                                      reference="n=1000000", precision=0)

    def test_assertRasters3dNoDifference(self):
        """Test basic usage of assertRastersNoDifference"""
        # precision might need to be increased
        self.assertRasters3dNoDifference(actual=self.rcd_increasing_map,
                                       reference=self.rcd_increasing_map,
                                       precision=0,
                                       msg="The same maps should have no difference")
        self.assertRaises(self.failureException,
                          self.assertRasters3dNoDifference,
                          actual=self.constant_map,
                          reference=self.rcd_increasing_map,
                          precision=1,
                          msg="Different maps should have difference")

    def test_assertRasters3dNoDifference_mean(self):
        """Test usage of assertRastersNoDifference with mean"""
        self.assertRasters3dNoDifference(actual=self.rcd_increasing_map,
                                       reference=self.rcd_increasing_map,
                                       precision=0,  # this might need to be increased
                                       statistics=dict(mean=0),
                                       msg="The difference of same maps should have small mean")
        self.assertRaises(self.failureException,
                          self.assertRasters3dNoDifference,
                          actual=self.constant_map,
                          reference=self.rcd_increasing_map,
                          precision=1,
                          statistics=dict(mean=0),
                          msg="The difference of different maps should have huge mean")


if __name__ == '__main__':
    test()
