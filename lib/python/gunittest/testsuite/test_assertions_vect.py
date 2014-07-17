# -*- coding: utf-8 -*-

"""
Tests assertion methods for vectors.
"""

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


class TestVectorMapAssertions(gunittest.TestCase):
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

if __name__ == '__main__':
    gunittest.test()
