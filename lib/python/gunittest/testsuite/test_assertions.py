# -*- coding: utf-8 -*-

"""
Tests assertion methods.
"""


import os

import grass.script.core as gcore
from grass.pygrass.modules import Module

import grass.gunittest
from grass.gunittest.gmodules import SimpleModule


class TestTextAssertions(grass.gunittest.TestCase):
    # pylint: disable=R0904
    def test_assertLooksLike(self):
        self.assertLooksLike("Generated map is <elevation>",
                             "Generated map is <...>")
        self.assertRaises(self.failureException,
                          self.assertLooksLike,
                          "Generated map is elevation.",
                          "Generated map is <...>")
        self.assertLooksLike("Projection string: '+proj=longlat +datum=WGS84'",
                             "Projection string: ...")

    def test_assertLooksLike_multiline(self):
        self.assertLooksLike("a=123\nb=456\nc=789",
                             "a=...\nb=...\nc=...")

    def test_assertLooksLike_numbers(self):
        self.assertLooksLike("abc = 125521",
                             "abc = 125...")
        self.assertLooksLike("abc = 689.156",
                             "abc = 689...")
        self.assertLooksLike("abc = 689.159589",
                             "abc = 689.15...")
        # this should fail accoring to the implementation
        # first three dots are considered as ellipses
        self.assertRaises(self.failureException,
                          self.assertLooksLike,
                          "abc = 689.159589",
                          "abc = 689....")


R_UNIVAR_ELEVATION_SUBSET = """n=2025000
null_cells=0
min=55.5787925720215
max=156.329864501953
"""

RANDOM_KEYVALUES = """abc=2025000
aaa=55.5787925720215
bbb=156.329864501953
"""

R_INFO_ELEVATION_SUBSET = """rows=1350
cols=1500
cells=2025000
datatype=FCELL
"""

# r.info -gre map=elevation
ELEVATION_MAPSET_DICT = {'mapset': 'PERMANENT'}

# r.univar map=elevation
ELEVATION_MINMAX = """min=55.5787925720215
max=156.329864501953
"""

# values rounded manually to maximal expected perecision
ELEVATION_MINMAX_DICT = {'min': 55.58, 'max': 156.33}

V_UNIVAR_BRIDGES_WIDTH_SUBSET = """n=10938
nmissing=0
nnull=0
min=0
max=1451
range=1451
sum=2.6299e+06
mean=240.437
"""


class TestAssertCommandKeyValue(grass.gunittest.TestCase):
    """Test usage of `.assertCommandKeyValue` method."""
    # pylint: disable=R0904

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule(Module('g.region', rast='elevation', run_=False))

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def test_pygrass_module(self):
        """Test syntax with Module as module"""
        module = Module('r.info', map='elevation', flags='gr', run_=False)
        self.assertCommandKeyValue(module,
                                   reference=dict(min=55.58, max=156.33),
                                   precision=0.01, sep='=')

    def test_pygrass_simple_module(self):
        """Test syntax with SimpleModule as module"""
        module = SimpleModule('r.info', map='elevation', flags='gr')
        self.assertCommandKeyValue(module,
                                   reference=dict(min=55.58, max=156.33),
                                   precision=0.01, sep='=')

    def test_direct_parameters(self):
        """Test syntax with module and its parameters as fnction parameters"""
        self.assertCommandKeyValue('r.info', map='elevation', flags='gr',
                                   reference=dict(min=55.58, max=156.33),
                                   precision=0.01, sep='=')

    def test_parameters_parameter(self):
        """Test syntax with module parameters in one parameters dictionary"""
        self.assertCommandKeyValue(module='r.info',
                                   parameters=dict(map='elevation', flags='gr'),
                                   reference=dict(min=55.58, max=156.33),
                                   precision=0.01, sep='=')


class TestRasterMapAssertations(grass.gunittest.TestCase):
    # pylint: disable=R0904

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # TODO: here we should actually not call self.runModule but call_module
        cls.runModule(Module('g.region', rast='elevation', run_=False))

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def test_assertRasterFitsUnivar(self):
        self.assertRasterFitsUnivar('elevation', R_UNIVAR_ELEVATION_SUBSET,
                                    precision=0.01)
        self.assertRaises(self.failureException,
                          self.assertRasterFitsUnivar,
                          'aspect', R_UNIVAR_ELEVATION_SUBSET, precision=0.01)
        self.assertRaises(ValueError,
                          self.assertRasterFitsUnivar,
                          'elevation', RANDOM_KEYVALUES)

    def test_assertRasterFitsInfo(self):
        self.assertRasterFitsInfo('elevation', R_INFO_ELEVATION_SUBSET)
        self.assertRaises(self.failureException,
                          self.assertRasterFitsInfo,
                          'elev_lid792_1m', R_INFO_ELEVATION_SUBSET)
        self.assertRaises(ValueError,
                          self.assertRasterFitsInfo,
                          'elevation', RANDOM_KEYVALUES)

    def test_common_values_info_univar(self):
        self.assertRasterFitsUnivar('elevation',
                                    ELEVATION_MINMAX, precision=0.01)
        self.assertRasterFitsInfo('elevation',
                                  ELEVATION_MINMAX, precision=0.01)

    def test_dict_as_parameter(self):
        # this also tests if we are using r.info -e flag
        self.assertRasterFitsInfo('elevation', ELEVATION_MAPSET_DICT)

    def test_assertRastersNoDifference(self):
        """Test basic usage of assertRastersNoDifference"""
        self.assertRastersNoDifference(actual='elevation',
                                       reference='elevation',
                                       precision=0,  # this might need to be increased
                                       msg="The same maps should have no difference")
        self.assertRaises(self.failureException,
                          self.assertRastersNoDifference,
                          actual='elevation',
                          reference='aspect',
                          precision=1,
                          msg="Different maps should have difference")

    def test_assertRastersNoDifference_mean(self):
        """Test usage of assertRastersNoDifference with mean"""
        self.assertRastersNoDifference(actual='elevation',
                                       reference='elevation',
                                       precision=0,  # this might need to be increased
                                       statistics=dict(mean=0),
                                       msg="The difference of same maps should have small mean")
        self.assertRaises(self.failureException,
                          self.assertRastersNoDifference,
                          actual='elevation',
                          reference='aspect',
                          precision=1,
                          statistics=dict(mean=0),
                          msg="The difference of different maps should have huge mean")


class TestVectorMapAssertations(grass.gunittest.TestCase):
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


class TestFileAssertations(grass.gunittest.TestCase):
    # pylint: disable=R0904

    @classmethod
    def setUpClass(cls):
        # we expect WIND to be always present
        gisenv = gcore.gisenv()
        cls.existing_file = os.path.join(gisenv['GISDBASE'],
                                         gisenv['LOCATION_NAME'],
                                         'PERMANENT', 'WIND')
        cls.emtpy_file = cls.__name__ + '_this_is_an_empty_file'
        open(cls.emtpy_file, 'w').close()
        cls.file_with_md5 = cls.__name__ + '_this_is_a_file_with_known_md5'
        file_content = 'Content of the file with known MD5.\n'
        with open(cls.file_with_md5, 'w') as f:
            f.write(file_content)
        # MD5 sum created using:
        # echo 'Content of the file with known MD5.' > some_file.txt
        # md5sum some_file.txt
        cls.file_md5 = '807bba4ffac4bb351bc3f27853009949'

        cls.file_with_same_content = cls.__name__ + '_file_with_same_content'
        with open(cls.file_with_same_content, 'w') as f:
            f.write(file_content)

        cls.file_with_different_content = cls.__name__ + '_file_with_different_content'
        with open(cls.file_with_different_content, 'w') as f:
            f.write(file_content + ' Something else here.')

    @classmethod
    def tearDownClass(cls):
        os.remove(cls.emtpy_file)
        os.remove(cls.file_with_md5)
        os.remove(cls.file_with_same_content)
        os.remove(cls.file_with_different_content)

    def test_assertFileExists(self):
        self.assertFileExists(filename=self.existing_file)
        self.assertRaises(self.failureException,
                          self.assertFileExists,
                          filename='this_one_does_not_exists')

    def test_assertFileExists_empty_file(self):
        self.assertFileExists(filename=self.emtpy_file, skip_size_check=True)
        self.assertRaises(self.failureException,
                          self.assertFileExists,
                          filename=self.emtpy_file)

    def test_assertFileMd5(self):
        self.assertFileMd5(filename=self.file_with_md5, md5=self.file_md5)
        self.assertRaises(self.failureException,
                          self.assertFileMd5,
                          filename=self.file_with_md5, md5='wrongmd5')

    def test_assertFilesEqualMd5(self):
        self.assertFilesEqualMd5(filename=self.file_with_md5,
                                 reference=self.file_with_same_content)
        self.assertRaises(self.failureException,
                          self.assertFilesEqualMd5,
                          filename=self.file_with_md5,
                          reference=self.file_with_different_content)


if __name__ == '__main__':
    grass.gunittest.test()
