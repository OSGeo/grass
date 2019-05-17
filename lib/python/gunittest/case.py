# -*- coding: utf-8 -*-
"""GRASS Python testing framework test case

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras
"""
from __future__ import print_function

import os
import subprocess
import sys
import hashlib
import uuid
import unittest

from grass.pygrass.modules import Module
from grass.exceptions import CalledModuleError
from grass.script import shutil_which, text_to_string, encode

from .gmodules import call_module, SimpleModule
from .checkers import (check_text_ellipsis,
                       text_to_keyvalue, keyvalue_equals, diff_keyvalue,
                       file_md5, text_file_md5, files_equal_md5)
from .utils import safe_repr
from .gutils import is_map_in_mapset

pyversion = sys.version_info[0]
if pyversion == 2:
    from StringIO import StringIO
else:
    from io import StringIO
    unicode = str


class TestCase(unittest.TestCase):
    # we dissable R0904 for all TestCase classes because their purpose is to
    # provide a lot of assert methods
    # pylint: disable=R0904
    """

    Always use keyword arguments for all parameters other than first two. For
    the first two, it is recommended to use keyword arguments but not required.
    Be especially careful and always use keyword argument syntax for *msg*
    parameter.
    """
    longMessage = True  # to get both standard and custom message
    maxDiff = None  # we can afford long diffs
    _temp_region = None  # to control the temporary region
    html_reports = False  # output additional HTML files with failure details
    readable_names = False  # prefer shorter but unreadable map and file names

    def __init__(self, methodName):
        super(TestCase, self).__init__(methodName)
        self.grass_modules = []
        self.supplementary_files = []
        # Python unittest doc is saying that strings use assertMultiLineEqual
        # but only unicode type is registered
        # TODO: report this as a bug? is this in Python 3.x?
        self.addTypeEqualityFunc(str, 'assertMultiLineEqual')

    def _formatMessage(self, msg, standardMsg):
        """Honor the longMessage attribute when generating failure messages.

        If longMessage is False this means:

        * Use only an explicit message if it is provided
        * Otherwise use the standard message for the assert

        If longMessage is True:

        * Use the standard message
        * If an explicit message is provided, return string with both messages

        Based on Python unittest _formatMessage, formatting changed.
        """
        if not self.longMessage:
            return msg or standardMsg
        if msg is None:
            return standardMsg
        try:
            # don't switch to '{}' formatting in Python 2.X
            # it changes the way unicode input is handled
            return '%s \n%s' % (msg, standardMsg)
        except UnicodeDecodeError:
            return '%s \n%s' % (safe_repr(msg), safe_repr(standardMsg))

    @classmethod
    def use_temp_region(cls):
        """Use temporary region instead of the standard one for this process.

        If you use this method, you have to call it in `setUpClass()`
        and call `del_temp_region()` in `tearDownClass()`. By this you
        ensure that each test method will have its own region and will
        not influence other classes.

        ::

            @classmethod
            def setUpClass(self):
                self.use_temp_region()

            @classmethod
            def tearDownClass(self):
                self.del_temp_region()

        You can also call the methods in `setUp()` and `tearDown()` if
        you are using them.

        Copies the current region to a temporary region with
        ``g.region save=``, then sets ``WIND_OVERRIDE`` to refer
        to that region.
        """
        # we use just the class name since we rely on the invocation system
        # where each test file is separate process and nothing runs
        # in parallel inside
        name = "tmp.%s" % (cls.__name__)
        call_module("g.region", save=name, overwrite=True)
        os.environ['WIND_OVERRIDE'] = name
        cls._temp_region = name

    @classmethod
    def del_temp_region(cls):
        """Remove the temporary region.

        Unsets ``WIND_OVERRIDE`` and removes any region named by it.
        """
        assert cls._temp_region
        name = os.environ.pop('WIND_OVERRIDE')
        if name != cls._temp_region:
            # be strict about usage of region
            raise RuntimeError("Inconsistent use of"
                               " TestCase.use_temp_region, WIND_OVERRIDE"
                               " or temporary region in general\n"
                               "Region to which should be now deleted ({n})"
                               " by TestCase class"
                               "does not correspond to currently set"
                               " WIND_OVERRIDE ({c})",
                               n=cls._temp_region, c=name)
        call_module("g.remove", quiet=True, flags='f', type='region', name=name)
        # TODO: we don't know if user calls this
        # so perhaps some decorator which would use with statemet
        # but we have zero chance of infuencing another test class
        # since we use class-specific name for temporary region

    def assertMultiLineEqual(self, first, second, msg=None):
        r"""Test that the multiline string first is equal to the string second.

        When not equal a diff of the two strings highlighting the differences
        will be included in the error message. This method is used by default
        when comparing strings with assertEqual().

        This method replaces platform dependent newline characters
        by ``\n`` (LF) in both parameters. This is
        different from the same method implemented in Python ``unittest``
        package which preserves the original newline characters.

        This function removes the burden of getting the newline characters
        right on each platform. You can just use ``\n`` everywhere and this
        function will ensure that it does not matter if for example,
        a module generates (as expected) ``\r\n`` (CRLF) newline characters
        on MS Windows.

        .. warning::
            If you need to test the actual newline characters, use the standard
            string comparison and functions such as ``find()``.
        """
        if os.linesep != '\n':
            if os.linesep in first:
                first = first.replace(os.linesep, '\n')
            if os.linesep in second:
                second = second.replace(os.linesep, '\n')
        return super(TestCase, self).assertMultiLineEqual(
            first=first, second=second, msg=msg)

    def assertLooksLike(self, actual, reference, msg=None):
        r"""Test that ``actual`` text is the same as ``reference`` with ellipses.

        If ``actual`` contains platform dependent newline characters,
        these will replaced by ``\n`` which is expected to be in the test data.

        See :func:`check_text_ellipsis` for details of behavior.
        """
        self.assertTrue(isinstance(actual, (str, unicode)), (
                        'actual argument is not a string'))
        self.assertTrue(isinstance(reference, (str, unicode)), (
                        'reference argument is not a string'))
        if os.linesep != '\n' and os.linesep in actual:
            actual = actual.replace(os.linesep, '\n')
        if not check_text_ellipsis(actual=actual, reference=reference):
            # TODO: add support for multiline (first line general, others with details)
            standardMsg = '"%s" does not correspond with "%s"' % (actual,
                                                                  reference)
            self.fail(self._formatMessage(msg, standardMsg))

    # TODO: decide if precision is mandatory
    # (note that we don't need precision for strings and usually for integers)
    # TODO: auto-determine precision based on the map type
    # TODO: we can have also more general function without the subset reference
    # TODO: change name to Module
    def assertModuleKeyValue(self, module, reference, sep,
                             precision, msg=None, **parameters):
        """Test that output of a module is the same as provided subset.

        ::

            self.assertModuleKeyValue('r.info', map='elevation', flags='gr',
                                      reference=dict(min=55.58, max=156.33),
                                      precision=0.01, sep='=')

        ::

            module = SimpleModule('r.info', map='elevation', flags='gr')
            self.assertModuleKeyValue(module,
                                      reference=dict(min=55.58, max=156.33),
                                      precision=0.01, sep='=')

        The output of the module should be key-value pairs (shell script style)
        which is typically obtained using ``-g`` flag.
        """
        if isinstance(reference, str):
            reference = text_to_keyvalue(reference, sep=sep, skip_empty=True)
        module = _module_from_parameters(module, **parameters)
        self.runModule(module, expecting_stdout=True)
        raster_univar = text_to_keyvalue(module.outputs.stdout,
                                         sep=sep, skip_empty=True)
        if not keyvalue_equals(dict_a=reference, dict_b=raster_univar,
                               a_is_subset=True, precision=precision):
            unused, missing, mismatch = diff_keyvalue(dict_a=reference,
                                                      dict_b=raster_univar,
                                                      a_is_subset=True,
                                                      precision=precision)
            # TODO: add region vs map extent and res check in case of error
            if missing:
                raise ValueError("%s output does not contain"
                                 " the following keys"
                                 " provided in reference"
                                 ": %s\n" % (module, ", ".join(missing)))
            if mismatch:
                stdMsg = "%s difference:\n" % module
                stdMsg += "mismatch values"
                stdMsg += " (key, reference, actual): %s\n" % mismatch
                stdMsg += 'command: %s %s' % (module, parameters)
            else:
                # we can probably remove this once we have more tests
                # of keyvalue_equals and diff_keyvalue against each other
                raise RuntimeError("keyvalue_equals() showed difference but"
                                   " diff_keyvalue() did not. This can be"
                                   " a bug in one of them or in the caller"
                                   " (assertModuleKeyValue())")
            self.fail(self._formatMessage(msg, stdMsg))

    def assertRasterFitsUnivar(self, raster, reference,
                               precision=None, msg=None):
        r"""Test that raster map has the values obtained by r.univar module.

        The function does not require all values from r.univar.
        Only the provided values are tested.
        Typical example is checking minimum, maximum and number of NULL cells
        in the map::

            values = 'null_cells=0\nmin=55.5787925720215\nmax=156.329864501953'
            self.assertRasterFitsUnivar(raster='elevation', reference=values)

        Use keyword arguments syntax for all function parameters.

        Does not -e (extended statistics) flag, use `assertModuleKeyValue()`
        for the full interface of arbitrary module.
        """
        self.assertModuleKeyValue(module='r.univar',
                                  map=raster,
                                  separator='=',
                                  flags='g',
                                  reference=reference, msg=msg, sep='=',
                                  precision=precision)

    def assertRasterFitsInfo(self, raster, reference,
                             precision=None, msg=None):
        r"""Test that raster map has the values obtained by r.univar module.

        The function does not require all values from r.univar.
        Only the provided values are tested.
        Typical example is checking minimum, maximum and type of the map::

            minmax = 'min=0\nmax=1451\ndatatype=FCELL'
            self.assertRasterFitsInfo(raster='elevation', reference=minmax)

        Use keyword arguments syntax for all function parameters.

        This function supports values obtained -r (range) and
        -e (extended metadata) flags.
        """
        self.assertModuleKeyValue(module='r.info',
                                  map=raster, flags='gre',
                                  reference=reference, msg=msg, sep='=',
                                  precision=precision)

    def assertRaster3dFitsUnivar(self, raster, reference,
                                 precision=None, msg=None):
        r"""Test that 3D raster map has the values obtained by r3.univar module.

        The function does not require all values from r3.univar.
        Only the provided values are tested.

        Use keyword arguments syntax for all function parameters.

        Function does not use -e (extended statistics) flag,
        use `assertModuleKeyValue()` for the full interface of arbitrary
        module.
        """
        self.assertModuleKeyValue(module='r3.univar',
                                  map=raster,
                                  separator='=',
                                  flags='g',
                                  reference=reference, msg=msg, sep='=',
                                  precision=precision)

    def assertRaster3dFitsInfo(self, raster, reference,
                               precision=None, msg=None):
        r"""Test that raster map has the values obtained by r3.info module.

        The function does not require all values from r3.info.
        Only the provided values are tested.

        Use keyword arguments syntax for all function parameters.

        This function supports values obtained by -g (info) and -r (range).
        """
        self.assertModuleKeyValue(module='r3.info',
                                  map=raster, flags='gr',
                                  reference=reference, msg=msg, sep='=',
                                  precision=precision)

    def assertVectorFitsTopoInfo(self, vector, reference, msg=None):
        r"""Test that raster map has the values obtained by ``v.info`` module.

        This function uses ``-t`` flag of ``v.info`` module to get topology
        info, so the reference dictionary should contain appropriate set or
        subset of values (only the provided values are tested).

        A example of checking number of points::

            topology = dict(points=10938, primitives=10938)
            self.assertVectorFitsTopoInfo(vector='bridges', reference=topology)

        Note that here we are checking also the number of primitives to prove
        that there are no other features besides points.

        No precision is applied (no difference is required). So, this function
        is not suitable for testing items which are floating point number
        (no such items are currently in topological information).

        Use keyword arguments syntax for all function parameters.
        """
        self.assertModuleKeyValue(module='v.info',
                                  map=vector, flags='t',
                                  reference=reference, msg=msg, sep='=',
                                  precision=0)

    def assertVectorFitsRegionInfo(self, vector, reference,
                                   precision, msg=None):
        r"""Test that raster map has the values obtained by ``v.info`` module.

        This function uses ``-g`` flag of ``v.info`` module to get topology
        info, so the reference dictionary should contain appropriate set or
        subset of values (only the provided values are tested).

        Use keyword arguments syntax for all function parameters.
        """
        self.assertModuleKeyValue(module='v.info',
                                  map=vector, flags='g',
                                  reference=reference, msg=msg, sep='=',
                                  precision=precision)

    def assertVectorFitsExtendedInfo(self, vector, reference, msg=None):
        r"""Test that raster map has the values obtained by ``v.info`` module.

        This function uses ``-e`` flag of ``v.info`` module to get topology
        info, so the reference dictionary should contain appropriate set or
        subset of values (only the provided values are tested).

        The most useful items for testing (considering circumstances of test
        invocation) are name, title, level and num_dblinks. (When testing
        storing of ``v.info -e`` metadata, the selection might be different.)

        No precision is applied (no difference is required). So, this function
        is not suitable for testing items which are floating point number.

        Use keyword arguments syntax for all function parameters.
        """
        self.assertModuleKeyValue(module='v.info',
                                  map=vector, flags='e',
                                  reference=reference, msg=msg, sep='=',
                                  precision=0)

    def assertVectorInfoEqualsVectorInfo(self, actual, reference, precision,
                                         msg=None):
        """Test that two vectors are equal according to ``v.info -tg``.

        This function does not test geometry itself just the region of the
        vector map and number of features.
        """
        module = SimpleModule('v.info', flags='t', map=reference)
        self.runModule(module)
        ref_topo = text_to_keyvalue(module.outputs.stdout, sep='=')
        module = SimpleModule('v.info', flags='g', map=reference)
        self.runModule(module)
        ref_info = text_to_keyvalue(module.outputs.stdout, sep='=')
        self.assertVectorFitsTopoInfo(vector=actual, reference=ref_topo,
                                      msg=msg)
        self.assertVectorFitsRegionInfo(vector=actual, reference=ref_info,
                                        precision=precision, msg=msg)

    def assertVectorFitsUnivar(self, map, column, reference, msg=None,
                               layer=None, type=None, where=None,
                               precision=None):
        r"""Test that vector map has the values obtained by v.univar module.

        The function does not require all values from v.univar.
        Only the provided values are tested.
        Typical example is checking minimum and maximum of a column::

            minmax = 'min=0\nmax=1451'
            self.assertVectorFitsUnivar(map='bridges', column='WIDTH',
                                        reference=minmax)

        Use keyword arguments syntax for all function parameters.

        Does not support -d (geometry distances) flag, -e (extended statistics)
        flag and few other, use `assertModuleKeyValue` for the full interface
        of arbitrary module.
        """
        parameters = dict(map=map, column=column, flags='g')
        if layer:
            parameters.update(layer=layer)
        if type:
            parameters.update(type=type)
        if where:
            parameters.update(where=where)
        self.assertModuleKeyValue(module='v.univar',
                                  reference=reference, msg=msg, sep='=',
                                  precision=precision,
                                  **parameters)

    # TODO: use precision?
    # TODO: write a test for this method with r.in.ascii
    def assertRasterMinMax(self, map, refmin, refmax, msg=None):
        """Test that raster map minimum and maximum are within limits.

        Map minimum and maximum is tested against expression::

            refmin <= actualmin and refmax >= actualmax

        Use keyword arguments syntax for all function parameters.

        To check that more statistics have certain values use
        `assertRasterFitsUnivar()` or `assertRasterFitsInfo()`
        """
        stdout = call_module('r.info', map=map, flags='r')
        actual = text_to_keyvalue(stdout, sep='=')
        if refmin > actual['min']:
            stdmsg = ('The actual minimum ({a}) is smaller than the reference'
                      ' one ({r}) for raster map {m}'
                      ' (with maximum {o})'.format(
                          a=actual['min'], r=refmin, m=map, o=actual['max']))
            self.fail(self._formatMessage(msg, stdmsg))
        if refmax < actual['max']:
            stdmsg = ('The actual maximum ({a}) is greater than the reference'
                      ' one ({r}) for raster map {m}'
                      ' (with minimum {o})'.format(
                          a=actual['max'], r=refmax, m=map, o=actual['min']))
            self.fail(self._formatMessage(msg, stdmsg))

    # TODO: use precision?
    # TODO: write a test for this method with r.in.ascii
    # TODO: almost the same as 2D version
    def assertRaster3dMinMax(self, map, refmin, refmax, msg=None):
        """Test that 3D raster map minimum and maximum are within limits.

        Map minimum and maximum is tested against expression::

            refmin <= actualmin and refmax >= actualmax

        Use keyword arguments syntax for all function parameters.

        To check that more statistics have certain values use
        `assertRaster3DFitsUnivar()` or `assertRaster3DFitsInfo()`
        """
        stdout = call_module('r3.info', map=map, flags='r')
        actual = text_to_keyvalue(stdout, sep='=')
        if refmin > actual['min']:
            stdmsg = ('The actual minimum ({a}) is smaller than the reference'
                      ' one ({r}) for 3D raster map {m}'
                      ' (with maximum {o})'.format(
                          a=actual['min'], r=refmin, m=map, o=actual['max']))
            self.fail(self._formatMessage(msg, stdmsg))
        if refmax < actual['max']:
            stdmsg = ('The actual maximum ({a}) is greater than the reference'
                      ' one ({r}) for 3D raster map {m}'
                      ' (with minimum {o})'.format(
                          a=actual['max'], r=refmax, m=map, o=actual['min']))
            self.fail(self._formatMessage(msg, stdmsg))

    def _get_detailed_message_about_no_map(self, name, type):
        msg = ("There is no map <{n}> of type <{t}>"
               " in the current mapset".format(n=name, t=type))
        related = call_module('g.list', type='raster,raster3d,vector',
                              flags='imt', pattern='*' + name + '*')
        if related:
            msg += "\nSee available maps:\n"
            msg += related
        else:
            msg += "\nAnd there are no maps containing the name anywhere\n"
        return msg

    def assertRasterExists(self, name, msg=None):
        """Checks if the raster map exists in current mapset"""
        if not is_map_in_mapset(name, type='raster'):
            stdmsg = self._get_detailed_message_about_no_map(name, 'raster')
            self.fail(self._formatMessage(msg, stdmsg))

    def assertRasterDoesNotExist(self, name, msg=None):
        """Checks if the raster map does not exist in current mapset"""
        if is_map_in_mapset(name, type='raster'):
            stdmsg = self._get_detailed_message_about_no_map(name, 'raster')
            self.fail(self._formatMessage(msg, stdmsg))

    def assertRaster3dExists(self, name, msg=None):
        """Checks if the 3D raster map exists in current mapset"""
        if not is_map_in_mapset(name, type='raster3d'):
            stdmsg = self._get_detailed_message_about_no_map(name, 'raster3d')
            self.fail(self._formatMessage(msg, stdmsg))

    def assertRaster3dDoesNotExist(self, name, msg=None):
        """Checks if the 3D raster map does not exist in current mapset"""
        if is_map_in_mapset(name, type='raster3d'):
            stdmsg = self._get_detailed_message_about_no_map(name, 'raster3d')
            self.fail(self._formatMessage(msg, stdmsg))

    def assertVectorExists(self, name, msg=None):
        """Checks if the vector map exists in current mapset"""
        if not is_map_in_mapset(name, type='vector'):
            stdmsg = self._get_detailed_message_about_no_map(name, 'vector')
            self.fail(self._formatMessage(msg, stdmsg))

    def assertVectorDoesNotExist(self, name, msg=None):
        """Checks if the vector map does not exist in current mapset"""
        if is_map_in_mapset(name, type='vector'):
            stdmsg = self._get_detailed_message_about_no_map(name, 'vector')
            self.fail(self._formatMessage(msg, stdmsg))

    def assertFileExists(self, filename, msg=None,
                         skip_size_check=False, skip_access_check=False):
        """Test the existence of a file.

        .. note:
            By default this also checks if the file size is greater than 0
            since we rarely want a file to be empty. It also checks
            if the file is accessible for reading since we expect that user
            wants to look at created files.
        """
        if not os.path.isfile(filename):
            stdmsg = 'File %s does not exist' % filename
            self.fail(self._formatMessage(msg, stdmsg))
        if not skip_size_check and not os.path.getsize(filename):
            stdmsg = 'File %s is empty' % filename
            self.fail(self._formatMessage(msg, stdmsg))
        if not skip_access_check and not os.access(filename, os.R_OK):
            stdmsg = 'File %s is not accessible for reading' % filename
            self.fail(self._formatMessage(msg, stdmsg))

    def assertFileMd5(self, filename, md5, text=False, msg=None):
        r"""Test that file MD5 sum is equal to the provided sum.

        Usually, this function is used to test binary files or large text files
        which cannot be tested in some other way. Text files can be usually
        tested by some finer method.

        To test text files with this function, you should always use parameter
        *text* set to ``True``. Note that function ``checkers.text_file_md5()``
        offers additional parameters which might be advantageous when testing
        text files.

        The typical workflow is that you create a file in a way you
        trust (that you obtain the right file). Then you compute MD5
        sum of the file. And provide the sum in a test as a string::

            self.assertFileMd5('result.png', md5='807bba4ffa...')

        Use `file_md5()` function from this package::

            file_md5('original_result.png')

        Or in command line, use ``md5sum`` command if available:

        .. code-block:: sh

            md5sum some_file.png

        Finally, you can use Python ``hashlib`` to obtain MD5::

            import hashlib
            hasher = hashlib.md5()
            # expecting the file to fit into memory
            hasher.update(open('original_result.png', 'rb').read())
            hasher.hexdigest()

        .. note:
            For text files, always create MD5 sum using ``\n`` (LF)
            as newline characters for consistency. Also use newline
            at the end of file (as for example, Git or PEP8 requires).
        """
        self.assertFileExists(filename, msg=msg)
        if text:
            actual = text_file_md5(filename)
        else:
            actual = file_md5(filename)
        if not actual == md5:
            standardMsg = ('File <{name}> does not have the right MD5 sum.\n'
                           'Expected is <{expected}>,'
                           ' actual is <{actual}>'.format(
                               name=filename, expected=md5, actual=actual))
            self.fail(self._formatMessage(msg, standardMsg))

    def assertFilesEqualMd5(self, filename, reference, msg=None):
        """Test that files are the same using MD5 sum.

        This functions requires you to provide a file to test and
        a reference file. For both, MD5 sum will be computed and compared with
        each other.
        """
        self.assertFileExists(filename, msg=msg)
        # nothing for ref, missing ref_filename is an error not a test failure
        if not files_equal_md5(filename, reference):
            stdmsg = 'Files %s and %s don\'t have the same MD5 sums' % (filename,
                                                                        reference)
            self.fail(self._formatMessage(msg, stdmsg))

    def _get_unique_name(self, name):
        """Create standardized map or file name which is unique

        If ``readable_names`` attribute is `True`, it uses the *name* string
        to create the unique name. Otherwise, it creates a unique name.
        Even if you expect ``readable_names`` to be `True`, provide *name*
        which is unique

        The *name* parameter should be valid raster name, vector name and file
        name and should be always provided.
        """
        # TODO: possible improvement is to require some descriptive name
        # and ensure uniqueness by add UUID
        if self.readable_names:
            return 'tmp_' + self.id().replace('.', '_') + '_' + name
        else:
            # UUID might be overkill (and expensive) but it's safe and simple
            # alternative is to create hash from the readable name
            return 'tmp_' + str(uuid.uuid4()).replace('-', '')

    def _compute_difference_raster(self, first, second, name_part):
        """Compute difference of two rasters (first - second)

        The name of the new raster is a long name designed to be as unique as
        possible and contains names of two input rasters.

        :param first: raster to subtract from
        :param second: raster used as decrement
        :param name_part: a unique string to be used in the difference name

        :returns: name of a new raster
        """
        diff = self._get_unique_name('compute_difference_raster_' + name_part
                                     + '_' + first + '_minus_' + second)
        expression = '"{diff}" = "{first}" - "{second}"'.format(
            diff=diff,
            first=first,
            second=second
        )
        call_module('r.mapcalc', stdin=expression.encode("utf-8"))
        return diff

    # TODO: name of map generation is repeated three times
    # TODO: this method is almost the same as the one for 2D
    def _compute_difference_raster3d(self, first, second, name_part):
        """Compute difference of two rasters (first - second)

        The name of the new raster is a long name designed to be as unique as
        possible and contains names of two input rasters.

        :param first: raster to subtract from
        :param second: raster used as decrement
        :param name_part: a unique string to be used in the difference name

        :returns: name of a new raster
        """
        diff = self._get_unique_name('compute_difference_raster_' + name_part
                                     + '_' + first + '_minus_' + second)

        call_module('r3.mapcalc',
                    stdin='"{d}" = "{f}" - "{s}"'.format(d=diff,
                                                         f=first,
                                                         s=second))
        return diff

    def _compute_vector_xor(self, ainput, alayer, binput, blayer, name_part):
        """Compute symmetric difference (xor) of two vectors

        :returns: name of a new vector
        """
        diff = self._get_unique_name('compute_difference_vector_' + name_part
                                     + '_' + ainput + '_' + alayer + '_minus_'
                                     + binput + '_' + blayer)
        call_module('v.overlay', operator='xor', ainput=ainput, binput=binput,
                    alayer=alayer, blayer=blayer,
                    output=diff, atype='area', btype='area', olayer='')
        # trying to avoid long reports full of categories by olayer=''
        # olayer   Output layer for new category, ainput and binput
        #     If 0 or not given, the category is not written
        return diff

    # TODO: -z and 3D support
    def _import_ascii_vector(self, filename, name_part):
        """Import a vector stored in GRASS vector ASCII format.

        :returns: name of a new vector
        """
        # hash is the easiest way how to get a valid vector name
        # TODO: introduce some function which will make file valid
        hasher = hashlib.md5()
        hasher.update(encode(filename))
        namehash = hasher.hexdigest()
        vector = self._get_unique_name('import_ascii_vector_' + name_part
                                       + '_' + namehash)
        call_module('v.in.ascii', input=filename,
                    output=vector, format='standard')
        return vector

    # TODO: -z and 3D support
    def _export_ascii_vector(self, vector, name_part, digits):
        """Import a vector stored in GRASS vector ASCII format.

        :returns: name of a new vector
        """
        # TODO: perhaps we can afford just simple file name
        filename = self._get_unique_name('export_ascii_vector_'
                                         + name_part + '_' + vector)
        call_module('v.out.ascii', input=vector,
                    output=filename, format='standard', layer='-1',
                    precision=digits)
        return filename

    def assertRastersNoDifference(self, actual, reference,
                                  precision, statistics=None, msg=None):
        """Test that `actual` raster is not different from `reference` raster

        Method behaves in the same way as `assertRasterFitsUnivar()`
        but works on difference ``reference - actual``.
        If statistics is not given ``dict(min=-precision, max=precision)``
        is used.
        """
        if statistics is None or sorted(statistics.keys()) == ['max', 'min']:
            if statistics is None:
                statistics = dict(min=-precision, max=precision)
            diff = self._compute_difference_raster(reference, actual,
                                                   'assertRastersNoDifference')
            try:
                self.assertModuleKeyValue('r.info', map=diff, flags='r',
                                          sep='=', precision=precision,
                                          reference=statistics, msg=msg)
            finally:
                call_module('g.remove', flags='f', type='raster', name=diff)
        else:
            # general case
            # TODO: we are using r.info min max and r.univar min max interchangeably
            # but they might be different if region is different from map
            # not considered as an huge issue since we expect the tested maps
            # to match with region, however a documentation should containe a notice
            self.assertRastersDifference(actual=actual, reference=reference,
                                         statistics=statistics,
                                         precision=precision, msg=msg)

    def assertRastersDifference(self, actual, reference,
                                statistics, precision, msg=None):
        """Test statistical values of difference of reference and actual rasters

        For cases when you are interested in no or minimal difference,
        use `assertRastersNoDifference()` instead.

        This method should not be used to test r.mapcalc or r.univar.
        """
        diff = self._compute_difference_raster(reference, actual,
                                               'assertRastersDifference')
        try:
            self.assertRasterFitsUnivar(raster=diff, reference=statistics,
                                        precision=precision, msg=msg)
        finally:
            call_module('g.remove', flags='f', type='raster', name=diff)

    def assertRasters3dNoDifference(self, actual, reference,
                                    precision, statistics=None, msg=None):
        """Test that `actual` raster is not different from `reference` raster

        Method behaves in the same way as `assertRasterFitsUnivar()`
        but works on difference ``reference - actual``.
        If statistics is not given ``dict(min=-precision, max=precision)``
        is used.
        """
        if statistics is None or sorted(statistics.keys()) == ['max', 'min']:
            if statistics is None:
                statistics = dict(min=-precision, max=precision)
            diff = self._compute_difference_raster3d(reference, actual,
                                                     'assertRasters3dNoDifference')
            try:
                self.assertModuleKeyValue('r3.info', map=diff, flags='r',
                                          sep='=', precision=precision,
                                          reference=statistics, msg=msg)
            finally:
                call_module('g.remove', flags='f', type='raster_3d', name=diff)
        else:
            # general case
            # TODO: we are using r.info min max and r.univar min max interchangeably
            # but they might be different if region is different from map
            # not considered as an huge issue since we expect the tested maps
            # to match with region, however a documentation should contain a notice
            self.assertRasters3dDifference(actual=actual, reference=reference,
                                           statistics=statistics,
                                           precision=precision, msg=msg)

    def assertRasters3dDifference(self, actual, reference,
                                statistics, precision, msg=None):
        """Test statistical values of difference of reference and actual rasters

        For cases when you are interested in no or minimal difference,
        use `assertRastersNoDifference()` instead.

        This method should not be used to test r3.mapcalc or r3.univar.
        """
        diff = self._compute_difference_raster3d(reference, actual,
                                                 'assertRasters3dDifference')
        try:
            self.assertRaster3dFitsUnivar(raster=diff, reference=statistics,
                                          precision=precision, msg=msg)
        finally:
            call_module('g.remove', flags='f', type='raster_3d', name=diff)

    # TODO: this works only in 2D
    # TODO: write tests
    def assertVectorIsVectorBuffered(self, actual, reference, precision, msg=None):
        """

        This method should not be used to test v.buffer, v.overlay or v.select.
        """
        # TODO: if msg is None: add info specific to this function
        layer = '-1'
        self.assertVectorInfoEqualsVectorInfo(actual=actual,
                                              reference=reference,
                                              precision=precision, msg=msg)
        remove = []
        buffered = reference + '_buffered'  # TODO: more unique name
        intersection = reference + '_intersection'  # TODO: more unique name
        self.runModule('v.buffer', input=reference, layer=layer,
                       output=buffered, distance=precision)
        remove.append(buffered)
        try:
            self.runModule('v.overlay', operator='and', ainput=actual,
                           binput=reference,
                           alayer=layer, blayer=layer,
                           output=intersection, atype='area', btype='area',
                           olayer='')
            remove.append(intersection)
            # TODO: this would use some refactoring
            # perhaps different functions or more low level functions would
            # be more appropriate
            module = SimpleModule('v.info', flags='t', map=reference)
            self.runModule(module)
            ref_topo = text_to_keyvalue(module.outputs.stdout, sep='=')
            self.assertVectorFitsTopoInfo(vector=intersection,
                                          reference=ref_topo,
                                          msg=msg)
            module = SimpleModule('v.info', flags='g', map=reference)
            self.runModule(module)
            ref_info = text_to_keyvalue(module.outputs.stdout, sep='=')
            self.assertVectorFitsRegionInfo(vector=intersection,
                                            reference=ref_info,
                                            msg=msg, precision=precision)
        finally:
            call_module('g.remove', flags='f', type='vector', name=remove)

    # TODO: write tests
    def assertVectorsNoAreaDifference(self, actual, reference, precision,
                                      layer=1, msg=None):
        """Test statistical values of difference of reference and actual rasters

        Works only for areas.

        Use keyword arguments syntax for all function parameters.

        This method should not be used to test v.overlay or v.select.
        """
        diff = self._compute_xor_vectors(ainput=reference, binput=actual,
                                         alayer=layer, blayer=layer,
                                         name_part='assertVectorsNoDifference')
        try:
            module = SimpleModule('v.to.db', map=diff,
                                  flags='pc', separator='=')
            self.runModule(module)
            # the output of v.to.db -pc sep== should look like:
            # ...
            # 43=98606087.5818323
            # 44=727592.902311112
            # total area=2219442027.22035
            total_area = module.outputs.stdout.splitlines()[-1].split('=')[-1]
            if total_area > precision:
                stdmsg = ("Area of difference of vectors <{va}> and <{vr}>"
                          " should be 0"
                          " in the given precision ({p}) not {a}").format(
                    va=actual, vr=reference, p=precision, a=total_area)
                self.fail(self._formatMessage(msg, stdmsg))
        finally:
            call_module('g.remove', flags='f', type='vector', name=diff)

    # TODO: here we have to have significant digits which is not consistent
    # TODO: documentation for all new asserts
    # TODO: same can be created for raster and 3D raster
    def assertVectorEqualsVector(self, actual, reference, digits, precision, msg=None):
        """Test that two vectors are equal.

        .. note:
            This test should not be used to test ``v.in.ascii`` and
            ``v.out.ascii`` modules.

        .. warning:
            ASCII files for vectors are loaded into memory, so this
            function works well only for "not too big" vector maps.
        """
        # both vectors to ascii
        # text diff of two ascii files
        # may also do other comparisons on vectors themselves (asserts)
        self.assertVectorInfoEqualsVectorInfo(actual=actual, reference=reference, precision=precision, msg=msg)
        factual = self._export_ascii_vector(vector=actual,
                                            name_part='assertVectorEqualsVector_actual',
                                            digits=digits)
        freference = self._export_ascii_vector(vector=reference,
                                               name_part='assertVectorEqualsVector_reference',
                                               digits=digits)
        self.assertVectorAsciiEqualsVectorAscii(actual=factual,
                                                reference=freference,
                                                remove_files=True,
                                                msg=msg)

    def assertVectorEqualsAscii(self, actual, reference, digits, precision, msg=None):
        """Test that vector is equal to the vector stored in GRASS ASCII file.

        .. note:
            This test should not be used to test ``v.in.ascii`` and
            ``v.out.ascii`` modules.

        .. warning:
            ASCII files for vectors are loaded into memory, so this
            function works well only for "not too big" vector maps.
        """
        # vector to ascii
        # text diff of two ascii files
        # it may actually import the file and do other asserts
        factual = self._export_ascii_vector(vector=actual,
                                            name_part='assertVectorEqualsAscii_actual',
                                            digits=digits)
        vreference = None
        try:
            vreference = self._import_ascii_vector(filename=reference,
                                               name_part='assertVectorEqualsAscii_reference')
            self.assertVectorInfoEqualsVectorInfo(actual=actual,
                                                  reference=vreference,
                                                  precision=precision, msg=msg)
            self.assertVectorAsciiEqualsVectorAscii(actual=factual,
                                                    reference=reference,
                                                    remove_files=False,
                                                    msg=msg)
        finally:
            # TODO: manage using cleanup settings
            # we rely on fail method to either raise or return (soon)
            os.remove(factual)
            if vreference:
                self.runModule('g.remove', flags='f', type='vector', name=vreference)

    # TODO: we expect v.out.ascii to give the same order all the time, is that OK?
    def assertVectorAsciiEqualsVectorAscii(self, actual, reference,
                                           remove_files=False, msg=None):
        """Test that two GRASS ASCII vector files are equal.

        .. note:
            This test should not be used to test ``v.in.ascii`` and
            ``v.out.ascii`` modules.

        .. warning:
            ASCII files for vectors are loaded into memory, so this
            function works well only for "not too big" vector maps.
        """
        import difflib
        # 'U' taken from difflib documentation
        fromlines = open(actual, 'U').readlines()
        tolines = open(reference, 'U').readlines()
        context_lines = 3  # number of context lines
        # TODO: filenames are set to "actual" and "reference", isn't it too general?
        # it is even more useful if map names or file names are some generated
        # with hash or some other unreadable things
        # other styles of diffs are available too
        # but unified is a good choice if you are used to svn or git
        # workaround for missing -h (do not print header) flag in v.out.ascii
        num_lines_of_header = 10
        diff = difflib.unified_diff(fromlines[num_lines_of_header:],
                                    tolines[num_lines_of_header:],
                                    'reference', 'actual', n=context_lines)
        # TODO: this should be solved according to cleanup policy
        # but the parameter should be kept if it is an existing file
        # or using this method by itself
        if remove_files:
            os.remove(actual)
            os.remove(reference)
        stdmsg = ("There is a difference between vectors when compared as"
                  " ASCII files.\n")

        output = StringIO()
        # TODO: there is a diff size constant which we can use
        # we are setting it unlimited but we can just set it large
        maxlines = 100
        i = 0
        for line in diff:
            if i >= maxlines:
                break
            output.write(line)
            i += 1
        stdmsg += output.getvalue()
        output.close()
        # it seems that there is not better way of asking whether there was
        # a difference (always a iterator object is returned)
        if i > 0:
            # do HTML diff only if there is not too many lines
            # TODO: this might be tough to do with some more sophisticated way of reports
            if self.html_reports and i < maxlines:
                # TODO: this might be here and somehow stored as file or done in reporter again if right information is stored
                # i.e., files not deleted or the whole strings passed
                # alternative is make_table() which is the same but creates just a table not a whole document
                # TODO: all HTML files might be collected by the main reporter
                # TODO: standardize the format of name of HTML file
                # for one test id there is only one possible file of this name
                htmldiff_file_name = self.id() + '_ascii_diff' + '.html'
                self.supplementary_files.append(htmldiff_file_name)
                htmldiff = difflib.HtmlDiff().make_file(fromlines, tolines,
                                                        'reference', 'actual',
                                                        context=True,
                                                        numlines=context_lines)
                htmldiff_file = open(htmldiff_file_name, 'w')
                for line in htmldiff:
                    htmldiff_file.write(line)
                htmldiff_file.close()

            self.fail(self._formatMessage(msg, stdmsg))

    @classmethod
    def runModule(cls, module, expecting_stdout=False, **kwargs):
        """Run PyGRASS module.

        Runs the module and raises an exception if the module ends with
        non-zero return code. Usually, this is the same as testing the
        return code and raising exception but by using this method,
        you give testing framework more control over the execution,
        error handling and storing of output.

        In terms of testing framework, this function causes a common error,
        not a test failure.

        :raises CalledModuleError: if the module failed
        """
        module = _module_from_parameters(module, **kwargs)
        _check_module_run_parameters(module)
        try:
            module.run()
        except CalledModuleError:
            # here exception raised by run() with finish_=True would be
            # almost enough but we want some additional info to be included
            # in the test report
            errors = module.outputs.stderr
            # provide diagnostic at least in English locale
            # TODO: standardized error code would be handy here
            import re
            if re.search('Raster map.*not found', errors, flags=re.DOTALL):
                errors += "\nSee available raster maps:\n"
                errors += call_module('g.list', type='raster')
            if re.search('Vector map.*not found', errors, flags=re.DOTALL):
                errors += "\nSee available vector maps:\n"
                errors += call_module('g.list', type='vector')
            # TODO: message format, parameters
            raise CalledModuleError(module.popen.returncode, module.name,
                                    module.get_python(),
                                    errors=errors)
        # TODO: use this also in assert and apply when appropriate
        if expecting_stdout and not module.outputs.stdout.strip():

            if module.outputs.stderr:
                errors = " The errors are:\n" + module.outputs.stderr
            else:
                errors = " There were no error messages."
            if module.outputs.stdout:
                # this is not appropriate for translation but we don't want
                # and don't need testing to be translated
                got = "only whitespace."
            else:
                got = "nothing."
            raise RuntimeError("Module call " + module.get_python() +
                               " ended successfully but we were expecting"
                               " output and got " + got + errors)
    # TODO: we can also comapre time to some expected but that's tricky
    # maybe we should measure time but the real benchmarks with stdin/stdout
    # should be done by some other function
    # TODO: this should be the function used for valgrind or profiling or debug
    # TODO: it asserts the rc but it does much more, so testModule?
    # TODO: do we need special function for testing module failures or just add parameter returncode=0?
    # TODO: consider not allowing to call this method more than once
    # the original idea was to run this method just once for test method
    # but for "integration" tests  (script-like tests with more than one module)
    # it would be better to be able to use this multiple times
    # TODO: enable merging streams?
    def assertModule(self, module, msg=None, **kwargs):
        """Run PyGRASS module in controlled way and assert non-zero return code.

        You should use this method to invoke module you are testing.
        By using this method, you give testing framework more control over
        the execution, error handling and storing of output.

        It will not print module stdout and stderr, instead it will always
        store them for further examination. Streams are stored separately.

        This method is not suitable for testing error states of the module.
        If you want to test behavior which involves non-zero return codes
        and examine stderr in test, use `assertModuleFail()` method.

        Runs the module and causes test failure if module ends with
        non-zero return code.
        """
        module = _module_from_parameters(module, **kwargs)
        _check_module_run_parameters(module)
        if not shutil_which(module.name):
            stdmsg = "Cannot find the module '{0}'".format(module.name)
            self.fail(self._formatMessage(msg, stdmsg))
        try:
            module.run()
            self.grass_modules.append(module.name)
        except CalledModuleError:
            print(text_to_string(module.outputs.stdout))
            print(text_to_string(module.outputs.stderr))
            # TODO: message format
            # TODO: stderr?
            stdmsg = ('Running <{m.name}> module ended'
                      ' with non-zero return code ({m.popen.returncode})\n'
                      'Called: {code}\n'
                      'See the following errors:\n'
                      '{errors}'.format(
                          m=module, code=module.get_python(),
                          errors=module.outputs.stderr
                      ))
            self.fail(self._formatMessage(msg, stdmsg))
        print(text_to_string(module.outputs.stdout))
        print(text_to_string(module.outputs.stderr))
        # log these to final report
        # TODO: always or only if the calling test method failed?
        # in any case, this must be done before self.fail()
        # module.outputs['stdout'].value
        # module.outputs['stderr'].value

    # TODO: should we merge stderr to stdout in this case?
    def assertModuleFail(self, module, msg=None, **kwargs):
        """Test that module fails with a non-zero return code.

        Works like `assertModule()` but expects module to fail.
        """
        module = _module_from_parameters(module, **kwargs)
        _check_module_run_parameters(module)
        # note that we cannot use finally because we do not leave except
        try:
            module.run()
            self.grass_modules.append(module.name)
        except CalledModuleError:
            print(text_to_string(module.outputs.stdout))
            print(text_to_string(module.outputs.stderr))
        else:
            print(text_to_string(module.outputs.stdout))
            print(text_to_string(module.outputs.stderr))
            stdmsg = ('Running <%s> ended with zero (successful) return code'
                      ' when expecting module to fail' % module.get_python())
            self.fail(self._formatMessage(msg, stdmsg))


# TODO: add tests and documentation to methods which are using this function
# some test and documentation add to assertModuleKeyValue
def _module_from_parameters(module, **kwargs):
    if kwargs:
        if not isinstance(module, str):
            raise ValueError('module can be only string or PyGRASS Module')
        if isinstance(module, Module):
            raise ValueError('module can be only string if other'
                             ' parameters are given')
            # allow passing all parameters in one dictionary called parameters
        if list(kwargs.keys()) == ['parameters']:
            kwargs = kwargs['parameters']
        module = SimpleModule(module, **kwargs)
    return module


def _check_module_run_parameters(module):
    # in this case module already run and we would start it again
    if module.run_:
        raise ValueError('Do not run the module manually, set run_=False')
    if not module.finish_:
        raise ValueError('This function will always finish module run,'
                         ' set finish_=None or finish_=True.')
    # we expect most of the usages with stdout=PIPE
    # TODO: in any case capture PIPE always?
    if module.stdout_ is None:
        module.stdout_ = subprocess.PIPE
    elif module.stdout_ != subprocess.PIPE:
        raise ValueError('stdout_ can be only PIPE or None')
    if module.stderr_ is None:
        module.stderr_ = subprocess.PIPE
    elif module.stderr_ != subprocess.PIPE:
        raise ValueError('stderr_ can be only PIPE or None')
        # because we want to capture it
