# -*- coding: utf-8 -*-

"""
Tests assertion methods.
"""


import os

import grass.script.core as gcore
from grass.pygrass.modules import Module

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestTextAssertions(TestCase):
    # pylint: disable=R0904

    std_newline = "aaa\nbbb\n"
    platfrom_newline = "aaa{nl}bbb{nl}".format(nl=os.linesep)

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

    def test_assertLooksLike_multiline_platform_dependent(self):
        self.assertLooksLike("a=123\nb=456\nc=789",
                             "a=...{nl}b=...{nl}c=...".format(nl=os.linesep))

    def test_assertLooksLike_numbers(self):
        self.assertLooksLike("abc = 125521",
                             "abc = 125...")
        self.assertLooksLike("abc = 689.156",
                             "abc = 689...")
        self.assertLooksLike("abc = 689.159589",
                             "abc = 689.15...")
        # this should fail according to the implementation
        # first three dots are considered as ellipses
        self.assertRaises(self.failureException,
                          self.assertLooksLike,
                          "abc = 689.159589",
                          "abc = 689....")

    def do_all_combidnations(self, first, second, function):
        function(first, first)
        function(first, second)
        function(second, first)
        function(second, second)

    def test_assertMultiLineEqual(self):
        r"""Test different combinations of ``\n`` and os.linesep"""
        self.do_all_combidnations(self.std_newline, self.platfrom_newline,
                                  function=self.assertMultiLineEqual)

    def test_assertMultiLineEqual_raises(self):
        """Test mixed line endings"""
        self.assertRaises(self.failureException,
                          self.assertMultiLineEqual,
                          "aaa\n\rbbb\r",
                          "aaa\nbbb\n")

    def test_assertEqual(self):
        """Test for of newlines for strings (uses overwritten assertMultiLineEqual())"""
        self.do_all_combidnations(self.std_newline, self.platfrom_newline,
                                  function=self.assertEqual)

    def test_assertEqual_raises(self):
        """Test mixed line endings"""
        self.assertRaises(self.failureException,
                          self.assertEqual,
                          "aaa\n\rbbb\r",
                          "aaa\nbbb\n")


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


class TestAssertModuleKeyValue(TestCase):
    """Test usage of `assertModuleKeyValue` method."""
    # pylint: disable=R0904

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule(SimpleModule('g.region', raster='elevation'))

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def test_pygrass_module(self):
        """Test syntax with Module and required parameters as module"""
        module = Module('r.info', map='elevation', flags='gr',
                        run_=False, finish_=True)
        self.assertModuleKeyValue(module,
                                  reference=dict(min=55.58, max=156.33),
                                  precision=0.01, sep='=')

    def test_pygrass_simple_module(self):
        """Test syntax with SimpleModule as module"""
        module = SimpleModule('r.info', map='elevation', flags='gr')
        self.assertModuleKeyValue(module,
                                  reference=dict(min=55.58, max=156.33),
                                  precision=0.01, sep='=')

    def test_direct_parameters(self):
        """Test syntax with module and its parameters as function parameters"""
        self.assertModuleKeyValue('r.info', map='elevation', flags='gr',
                                  reference=dict(min=55.58, max=156.33),
                                  precision=0.01, sep='=')

    def test_parameters_parameter(self):
        """Test syntax with module parameters in one parameters dictionary"""
        self.assertModuleKeyValue(module='r.info',
                                  parameters=dict(map='elevation', flags='gr'),
                                  reference=dict(min=55.58, max=156.33),
                                  precision=0.01, sep='=')


class TestRasterMapAssertions(TestCase):
    # pylint: disable=R0904

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # TODO: here we should actually not call self.runModule but call_module
        cls.runModule(SimpleModule('g.region', raster='elevation'))

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def test_assertRasterFitsUnivar(self):
        self.assertRasterFitsUnivar('elevation', R_UNIVAR_ELEVATION_SUBSET,
                                    precision=0.01)
        self.assertRaises(self.failureException,
                          self.assertRasterFitsUnivar,
                          'geology', R_UNIVAR_ELEVATION_SUBSET, precision=0.01)
        self.assertRaises(ValueError,
                          self.assertRasterFitsUnivar,
                          'elevation', RANDOM_KEYVALUES)

    def test_assertRasterFitsInfo(self):
        self.assertRasterFitsInfo('elevation', R_INFO_ELEVATION_SUBSET)
        self.assertRaises(self.failureException,
                          self.assertRasterFitsInfo,
                          'geology', R_INFO_ELEVATION_SUBSET)
        self.assertRaises(ValueError,
                          self.assertRasterFitsInfo,
                          'elevation', RANDOM_KEYVALUES)

    def test_common_values_info_univar(self):
        self.assertRasterFitsUnivar('elevation',
                                    ELEVATION_MINMAX, precision=0.01)
        self.assertRasterFitsInfo('elevation',
                                  ELEVATION_MINMAX, precision=0.01)

    def test_dict_as_parameter(self):
        """This also tests if we are using r.info -e flag and that precision is
        not required for strings.
        """
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
                          reference='geology',
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
                          reference='geology',
                          precision=1,
                          statistics=dict(mean=0),
                          msg="The difference of different maps should have huge mean")

    def test_assertRastersEqual(self):
        """Test basic usage of assertRastersEqual"""
        self.assertRastersEqual(actual='lakes',
                                       reference='lakes',
                                       precision=0,
                                       msg="The same maps should have no difference")
        self.assertRaises(self.failureException,
                          self.assertRastersEqual,
                          actual='elevation',
                          reference='lakes',
                          precision=1,
                          msg="Different maps should have difference")


class TestMapExistsAssertions(TestCase):
    # pylint: disable=R0904

    raster_cell = 'TestMapExistsAssertions_raster_cell'
    raster_dcell = 'TestMapExistsAssertions_raster_dcell'
    raster3d = 'TestMapExistsAssertions_raster3D'
    vector = 'TestMapExistsAssertions_vector'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', n=10, e=10, s=0, w=0, t=10, b=0, res=1)
        cls.runModule('r.mapcalc', expression=cls.raster_cell + ' = 1')
        cls.runModule('r.mapcalc', expression=cls.raster_dcell + ' = 1.0')
        cls.runModule('r3.mapcalc', expression=cls.raster3d + ' = 1.0')
        cls.runModule('v.edit', map=cls.vector, tool='create')

    @classmethod
    def tearDownClass(cls):
        cls.runModule('g.remove', flags='f',
                      type=['raster', 'raster3d', 'vector'],
                      name=[cls.raster_cell, cls.raster_dcell,
                            cls.raster3d, cls.vector])
        cls.del_temp_region()

    def test_rast_cell_exists(self):
        self.assertRasterExists(self.raster_cell)

    def test_rast_dcell_exists(self):
        self.assertRasterExists(self.raster_dcell)

    def test_rast_does_not_exist(self):
        self.assertRaises(self.failureException,
                          self.assertRasterExists,
                          'does_not_exists')

    def test_rast3d_exists(self):
        self.assertRaster3dExists(self.raster3d)

    def test_rast3d_does_not_exist(self):
        self.assertRaises(self.failureException,
                          self.assertRaster3dExists,
                          'does_not_exists')

    def test_vect_exists(self):
        self.assertVectorExists(self.vector)

    def test_vect_does_not_exist(self):
        self.assertRaises(self.failureException,
                          self.assertVectorExists,
                          'does_not_exists')

    def test_rast_does_not_exist_in_current_mapset(self):
        # expecting that there is elevation in PERMANENT
        # TODO: use skip decorator
        # TODO: add the same tests but for vect and rast3d
        self.assertRaises(self.failureException,
                          self.assertRasterExists,
                          'elevation',
                          msg="Rasters from different mapsets should be ignored")


class TestFileAssertions(TestCase):
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
    test()
