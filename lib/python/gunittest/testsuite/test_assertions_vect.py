# -*- coding: utf-8 -*-

"""
Tests assertion methods for vectors.
"""

from grass.exceptions import CalledModuleError
import grass.gunittest as gunittest


V_UNIVAR_BRIDGES_WIDTH_SUBSET = """n=10938
nmissing=0
nnull=0
min=0
max=1451
range=1451
sum=2.6299e+06
mean=240.437
"""

RANDOM_KEYVALUES = """abc=2025000
aaa=55.5787925720215
bbb=156.329864501953
"""

# v.info bridges -t
V_UNIVAR_BRIDGES_TOPO = dict(
    nodes=0,
    points=10938,
    lines=0,
    boundaries=0,
    centroids=0,
    areas=0,
    islands=0,
    primitives=10938,
    map3d=0,
)

# v.info bridges -g and rounded
V_UNIVAR_BRIDGES_REGION = dict(
    north=317757,
    south=14691,
    east=915045,
    west=125794,
    top=0,
    bottom=0,
)

# v.info bridges -g and reduced to minumum
V_UNIVAR_BRIDGES_EXTENDED = dict(
    name='bridges',
    level=2,
    num_dblinks=1,
)


class TestVectorInfoAssertions(gunittest.TestCase):
    """Test assertions of map meta and statistics"""
    # pylint: disable=R0904
    def test_assertVectorFitsUnivar(self):
        self.assertVectorFitsUnivar(map='bridges', column='WIDTH',
                                    reference=V_UNIVAR_BRIDGES_WIDTH_SUBSET,
                                    precision=0.01)
        self.assertRaises(self.failureException,
                          self.assertVectorFitsUnivar,
                          map='bridges', column='YEAR_BUILT',
                          reference=V_UNIVAR_BRIDGES_WIDTH_SUBSET,
                          precision=0.01)
        self.assertRaises(ValueError,
                          self.assertVectorFitsUnivar,
                          map='bridges', column='WIDTH',
                          reference=RANDOM_KEYVALUES)

    def test_assertVectorFitsTopoInfo(self):
        self.assertVectorFitsTopoInfo('bridges', V_UNIVAR_BRIDGES_TOPO)
        self.assertRaises(self.failureException,
                          self.assertVectorFitsTopoInfo,
                          'lakes',
                          V_UNIVAR_BRIDGES_TOPO)
        self.assertRaises(ValueError,
                          self.assertVectorFitsTopoInfo,
                          'bridges', RANDOM_KEYVALUES)
        self.assertRaises(ValueError,
                          self.assertVectorFitsTopoInfo,
                          'bridges', V_UNIVAR_BRIDGES_REGION)

    def test_assertVectorFitsRegionInfo(self):
        self.assertVectorFitsRegionInfo('bridges', V_UNIVAR_BRIDGES_REGION, precision=1.0)
        self.assertRaises(self.failureException,
                          self.assertVectorFitsRegionInfo,
                          'lakes', V_UNIVAR_BRIDGES_REGION, precision=1.0)
        self.assertRaises(ValueError,
                          self.assertVectorFitsRegionInfo,
                          'bridges', RANDOM_KEYVALUES, precision=1.0)
        self.assertRaises(ValueError,
                          self.assertVectorFitsRegionInfo,
                          'bridges', V_UNIVAR_BRIDGES_TOPO, precision=1.0)

    def test_assertVectorFitsExtendedInfo(self):
        self.assertVectorFitsExtendedInfo('bridges', V_UNIVAR_BRIDGES_EXTENDED)
        self.assertRaises(self.failureException,
                          self.assertVectorFitsExtendedInfo,
                          'lakes',
                          V_UNIVAR_BRIDGES_EXTENDED)
        self.assertRaises(ValueError,
                          self.assertVectorFitsExtendedInfo,
                          'bridges',
                          RANDOM_KEYVALUES)
        self.assertRaises(ValueError,
                          self.assertVectorFitsExtendedInfo,
                          'bridges',
                          V_UNIVAR_BRIDGES_TOPO)

    def test_assertVectorInfoEqualsVectorInfo(self):
        self.assertVectorInfoEqualsVectorInfo('bridges', 'bridges', precision=0.00000001)
        self.assertRaises(self.failureException,
                          self.assertVectorInfoEqualsVectorInfo,
                          'lakes', 'bridges', precision=0.00000001)
        self.assertRaises(CalledModuleError,
                          self.assertVectorInfoEqualsVectorInfo,
                          'bridges', 'does_not_exist', precision=0.00000001)


class TestVectorGeometryAssertions(gunittest.TestCase):
    """Test assertions of map geometry"""
    # pylint: disable=R0904
    maps_to_remove = []
    simple_base_file = 'data/simple_vector_map_ascii_4p_2l_2c_3b_dp14.txt'
    simple_modified_file = 'data/simple_vector_map_ascii_4p_2l_2c_3b_dp14_modified.txt'
    simple_diff_header_file = 'data/simple_vector_map_ascii_4p_2l_2c_3b_dp14_diff_header.txt'
    precision = 0.00001
    digits = 14

    @classmethod
    def tearDownClass(cls):
        # TODO: this should be decided globaly by cleanup variable
        # perhaps cls.gremove() wheoul be the right option
        # when invoking separately, no need to delete maps since mapset
        # is deleted
        if cls.maps_to_remove:
            cls.runModule('g.remove', flags='f', type='vect',
                          pattern=','.join(cls.maps_to_remove))

    def test_assertVectorEqualsVector_basic(self):
        """Check completely different maps."""
        self.assertVectorEqualsVector(actual='bridges', reference='bridges',
                                      precision=0.01, digits=15)
        self.assertRaises(self.failureException,
                          self.assertVectorEqualsVector,
                          actual='bridges', reference='lakes',
                          precision=0.01, digits=7)
        self.assertRaises(CalledModuleError,
                          self.assertVectorEqualsVector,
                          actual='does_not_exist', reference='lakes',
                          precision=0.01, digits=7)

    def test_assertVectorEqualsVector_geometry_same_header(self):
        """Check small slighlty different maps with same header in ASCII."""
        amap = 'simple_vector_map_base_geom'
        bmap = 'simple_vector_map_modified_geom'
        self.runModule('v.in.ascii', format='standard',
                       input=self.simple_base_file,
                       output=amap)
        self.maps_to_remove.append(amap)
        self.runModule('v.in.ascii', format='standard',
                       input=self.simple_modified_file,
                       output=bmap)
        self.maps_to_remove.append(bmap)
        self.assertVectorEqualsVector(actual=amap, reference=amap,
                                      precision=self.precision, digits=self.digits)
        self.assertRaises(self.failureException,
                          self.assertVectorEqualsVector,
                          actual=amap, reference=bmap,
                          precision=self.precision, digits=self.digits)

    def test_assertVectorEqualsVector_geometry(self):
        """Check small slighlty different maps with different headers in ASCII."""
        amap = 'simple_vector_map_base'
        bmap = 'simple_vector_map_different_header'
        self.runModule('v.in.ascii', format='standard',
                       input=self.simple_base_file,
                       output=amap)
        self.maps_to_remove.append(amap)
        self.runModule('v.in.ascii', format='standard',
                       input=self.simple_diff_header_file,
                       output=bmap)
        self.maps_to_remove.append(bmap)
        self.assertVectorEqualsVector(actual=amap, reference=bmap,
                                      precision=self.precision, digits=self.digits)

    def test_assertVectorAsciiEqualsVectorAscii_diff_header(self):
        """Test ASCII files with different header.

        Prove that files were not deleted if not requested.
        """
        self.assertVectorAsciiEqualsVectorAscii(actual=self.simple_base_file,
                                                reference=self.simple_diff_header_file)
        self.assertFileExists(self.simple_base_file)
        self.assertFileExists(self.simple_diff_header_file)

    def test_assertVectorAsciiEqualsVectorAscii_diff_content(self):
        """Test ASCII files with slighlty different content.

        Prove that files were not deleted if not requested.
        """
        self.assertRaises(self.failureException,
                          self.assertVectorAsciiEqualsVectorAscii,
                          actual=self.simple_base_file,
                          reference=self.simple_modified_file)
        self.assertFileExists(self.simple_base_file)
        self.assertFileExists(self.simple_modified_file)

    def test_assertVectorEqualsAscii_by_import(self):
        amap = 'simple_vector_map_imported_base'
        self.runModule('v.in.ascii', format='standard',
                       input=self.simple_base_file,
                       output=amap)
        self.maps_to_remove.append(amap)
        self.assertVectorEqualsAscii(amap, self.simple_diff_header_file,
                                     precision=self.precision, digits=self.digits)
        self.assertRaises(self.failureException,
                          self.assertVectorEqualsAscii,
                          amap, self.simple_modified_file,
                          precision=self.precision, digits=self.digits)
        self.assertFileExists(self.simple_base_file)
        self.assertFileExists(self.simple_modified_file)
        self.assertFileExists(self.simple_diff_header_file)


if __name__ == '__main__':
    gunittest.test()
