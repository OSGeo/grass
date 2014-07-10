# -*- coding: utf-8 -*-

"""!@package grass.gunittest.case

@brief GRASS Python testing framework test case

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Vaclav Petras
"""

import os
import subprocess
import unittest
from unittest.util import safe_repr

from grass.pygrass.modules import Module
from grass.exceptions import CalledModuleError

from .gmodules import call_module
from .checkers import (check_text_ellipsis,
                       text_to_keyvalue, keyvalue_equals, diff_keyvalue,
                       file_md5, files_equal_md5)


class TestCase(unittest.TestCase):
    # we dissable R0904 for all TestCase classes because their purpose is to
    # provide a lot of assert methods
    # pylint: disable=R0904
    """

    Always use keyword arguments for all parameters other than first two. For
    the first two, it is recommended to use keyword arguments but not required.
    """
    longMessage = True  # to get both standard and custom message
    maxDiff = None  # we can afford long diffs
    _temp_region = None  # to control the temporary region

    def __init__(self, methodName):
        super(TestCase, self).__init__(methodName)

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
        # we use just the class name since we rely on the invokation system
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
                               "does not corresond to currently set"
                               " WIND_OVERRIDE ({c})",
                               n=cls._temp_region, c=name)
        call_module("g.remove", quiet=True, region=name)
        # TODO: we don't know if user calls this
        # so perhaps some decorator which would use with statemet
        # but we have zero chance of infuencing another test class
        # since we use class-specific name for temporary region

    def assertLooksLike(self, actual, reference, msg=None):
        """Test that ``actual`` text is the same as ``referece`` with ellipses.

        See :func:`check_text_ellipsis` for details of behavior.
        """
        self.assertTrue(isinstance(actual, basestring), (
                        'actual argument is not a string'))
        self.assertTrue(isinstance(reference, basestring), (
                        'reference argument is not a string'))
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
    def assertCommandKeyValue(self, module, reference, sep,
                              precision, msg=None, **parameters):
        """Test that output of a module is the same as provided subset.

        ::

            self.assertCommandKeyValue('r.info', map='elevation', flags='gr',
                                       reference=dict(min=55.58, max=156.33),
                                       precision=0.01, sep='=')

        ::

            module = SimpleModule('r.info', map='elevation', flags='gr')
            self.assertCommandKeyValue(module,
                                       reference=dict(min=55.58, max=156.33),
                                       precision=0.01, sep='=')

        The output of the module should be key-value pairs (shell script style)
        which is typically obtained using ``-g`` flag.
        """
        if isinstance(reference, basestring):
            reference = text_to_keyvalue(reference, sep=sep, skip_empty=True)
        module = _module_from_parameters(module, **parameters)
        self.runModule(module)
        raster_univar = text_to_keyvalue(module.outputs.stdout,
                                         sep=sep, skip_empty=True)
        if not keyvalue_equals(dict_a=reference, dict_b=raster_univar,
                                a_is_subset=True, precision=precision):
            unused, missing, mismatch = diff_keyvalue(dict_a=reference,
                                                      dict_b=raster_univar,
                                                      a_is_subset=True,
                                                      precision=precision)
            if missing:
                raise ValueError("%s output does not contain"
                                 " the following keys"
                                 " provided in reference"
                                 ": %s\n" % (module, ", ".join(missing)))
            if mismatch:
                stdMsg = "%s difference:\n" % module
                stdMsg += "mismatch values"
                stdMsg += "(key, reference, actual): %s\n" % mismatch
                stdMsg += 'command: %s %s' % (module, parameters)
            else:
                # we can probably remove this once we have more tests
                # of keyvalue_equals and diff_keyvalue against each other
                raise RuntimeError("keyvalue_equals() showed difference but"
                                   " diff_keyvalue() did not. This can be"
                                   " a bug in one of them or in the caller"
                                   " (assertCommandKeyValue())")
            self.fail(self._formatMessage(msg, stdMsg))

    def assertRasterFitsUnivar(self, raster, reference,
                               precision=None, msg=None):
        r"""Test that raster map has the values obtained by r.univar module.

        The function does not require all values from r.univar.
        Only the provided values are tested.
        Typical example is checking minimum, maximum and number of NULL cells
        in the map::

            values = 'null_cells=0\nmin=55.5787925720215\nmax=156.329864501953'
            self.assertRasterFitsUnivar(map='elevation', reference=values)

        Use keyword arguments syntax for all function parameters.

        Does not -e (extended statistics) flag, use `assertCommandKeyValue()`
        for the full interface of arbitrary module.
        """
        self.assertCommandKeyValue(module='r.univar',
                                   map=raster,
                                   separator='=',
                                   flags='g',
                                   reference=reference, msg=msg, sep='=',
                                   precision=precision)

    def assertRasterFitsInfo(self, raster, reference,
                             precision=None, msg=None):
        r"""Test that raster map has the values obtained by v.univar module.

        The function does not require all values from v.univar.
        Only the provided values are tested.
        Typical example is checking minimum, maximum and type of the map::

            minmax = 'min=0\nmax=1451\ndatatype=FCELL'
            self.assertRasterFitsInfo(map='elevation', reference=values)

        Use keyword arguments syntax for all function parameters.

        This function supports values obtained -r (range) and
        -e (extended metadata) flags.
        """
        self.assertCommandKeyValue(module='r.info',
                                   map=raster, flags='gre',
                                   reference=reference, msg=msg, sep='=',
                                   precision=precision)

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
        flag and few other, use `assertCommandKeyValue` for the full interface
        of arbitrary module.
        """
        parameters = dict(map=map, column=column, flags='g')
        if layer:
            parameters.update(layer=layer)
        if type:
            parameters.update(type=type)
        if where:
            parameters.update(where=where)
        self.assertCommandKeyValue(module='v.univar',
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

    def assertFileExists(self, filename, msg=None,
                         skip_size_check=False, skip_access_check=False):
        """Test the existence of a file.

        .. note:
            By default this also checks if the file size is greater than 0
            since we rarely want a file to be empty. And it also checks
            if the file is access for reading.
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

    def assertFileMd5(self, filename, md5, msg=None):
        """Test that file MD5 sum is equal to the provided sum.

        The typical workflow is that you create a file in a way you
        trust (that you obtain the right file). Then you compute MD5
        sum of the file. And provide the sum in a test as a string::

            self.assertFileMd5('result.txt', md5='807bba4ffa...')

        Use `file_md5()` function from this package::

            file_md5('original_result.txt')

        Or in command line, use ``md5sum`` command if available:

        .. code-block:: sh
            md5sum some_file.txt

        Finaly, you can use Python ``hashlib`` to obtain MD5::

            import hashlib
            hasher = hashlib.md5()
            # expecting the file to fit into memory
            hasher.update(open('original_result.txt', 'rb').read())
            hasher.hexdigest()
        """
        self.assertFileExists(filename, msg=msg)
        if not file_md5(filename) == md5:
            standardMsg = 'File %s does not have the right MD5 sum' % filename
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

    def _compute_difference_raster(self, first, second, name_part):
        """Compute difference of two rasters (first - second)

        The name of the new raster is a long name designed to be as unique as
        possible and contains names of two input rasters.

        :param first: raster to subtract from
        :param second: raster used as decrement
        :param name_part: a unique string to be used in the difference name

        :returns: name of a new raster
        """
        diff = ('tmp_' + self.id() + '_compute_difference_raster_'
                + name_part + '_' + first + '_minus_' + second)
        call_module('r.mapcalc',
                    stdin='"{d}" = "{f}" - "{s}"'.format(d=diff,
                                                         f=first,
                                                         s=second))
        return diff

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
                self.assertCommandKeyValue('r.info', map=diff, flags='r',
                                           sep='=', precision=precision,
                                           reference=statistics, msg=msg)
            finally:
                call_module('g.remove', rast=diff)
        # general case
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
            call_module('g.remove', rast=diff)

    @classmethod
    def runModule(cls, module, **kwargs):
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
        module.run()
        if module.popen.returncode:
            errors = module.outputs['stderr'].value
            # provide diagnostic at least in English locale
            # TODO: standardized error code would be handy here
            import re
            if re.search('Raster map.*not found', errors, flags=re.DOTALL):
                errors += "\nSee available raster maps:\n"
                errors += call_module('g.list', type='rast')
            if re.search('Vector map.*not found', errors, flags=re.DOTALL):
                errors += "\nSee available vector maps:\n"
                errors += call_module('g.list', type='vect')
            # TODO: message format, parameters
            raise CalledModuleError(module.popen.returncode, module.name,
                                    module.get_python(),
                                    errors=errors)

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

        # TODO: merge stderr to stdout? if caller gives PIPE, for sure not
        if module.run_:
            raise ValueError('Do not run the module manually, set run_=False')
        if not module.finish_:
            raise ValueError('This function will always finish module run,'
                             ' set finish_=None or finish_=True.')
        if module.stdout_ is None:
            module.stdout_ = subprocess.PIPE
        elif module.stdout_ != subprocess.PIPE:
            raise ValueError('stdout_ can be only PIPE or None')
            # because we want to capture it
        if module.stderr_ is None:
            module.stderr_ = subprocess.PIPE
        elif module.stderr_ != subprocess.PIPE:
            raise ValueError('stderr_ can be only PIPE or None')
            # because we want to capture it

        module.run()
        print module.outputs['stdout'].value
        print module.outputs['stderr'].value
        if module.popen.returncode:
            # TODO: message format
            # TODO: stderr?
            stdmsg = ('Running <{m.name}> module ended'
                      ' with non-zero return code ({m.popen.returncode})\n'
                      'Called: {code}\n'
                      'See the folowing errors:\n'
                      '{errors}'.format(
                          m=module, code=module.get_python(),
                          errors=module.outputs["stderr"].value
                      ))
            self.fail(self._formatMessage(msg, stdmsg))

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

        if module.run_:
            raise ValueError('Do not run the module manually, set run_=False')
        if not module.finish_:
            raise ValueError('This function will always finish module run,'
                             ' set finish_=None or finish_=True.')
        if module.stdout_ is None:
            module.stdout_ = subprocess.PIPE
        elif module.stdout_ != subprocess.PIPE:
            raise ValueError('stdout_ can be only PIPE or None')
            # because we want to capture it
        if module.stderr_ is None:
            module.stderr_ = subprocess.PIPE
        elif module.stderr_ != subprocess.PIPE:
            raise ValueError('stderr_ can be only PIPE or None')
            # because we want to capture it

        module.run()
        print module.outputs['stdout'].value
        print module.outputs['stderr'].value
        if not module.popen.returncode:
            stdmsg = ('Running <%s> ended with zero (successful) return code'
                      ' when expecting module to fail' % module.get_python())
            self.fail(self._formatMessage(msg, stdmsg))


# TODO: add tests and documentation to methods which are using this function
# some test and documentation add to assertCommandKeyValue
def _module_from_parameters(module, **kwargs):
    if kwargs:
        if not isinstance(module, basestring):
            raise ValueError('module can be only string or PyGRASS Module')
        if isinstance(module, Module):
            raise ValueError('module can be only string if other'
                             ' parameters are given')
            # allow to pass all parameters in one dictionary called parameters
        if kwargs.keys() == ['parameters']:
            kwargs = kwargs['parameters']
        module = Module(module, run_=False, **kwargs)
    return module
