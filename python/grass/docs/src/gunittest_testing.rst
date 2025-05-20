Testing GRASS GIS source code and modules
=========================================

If you are already familiar with the basic concepts
of GRASS testing framework, you might want to skip to one of:

* :ref:`test-module` section
* :ref:`test-c` section
* :ref:`test-python` section
* :ref:`test-doctest` section
* :class:`~gunittest.case.TestCase` class
* :ref:`running-tests-report` section


Introduction
------------

For the testing in GRASS GIS, we are using a `gunittest` package and
we usually refer to the system of writing and running tests
as to a *GRASS testing framework*.

The framework is based on Python `unittest`_ package with a large number
of GRASS-specific improvements, extensions and changes. These include
things such as creation of GRASS-aware HTML test reports,
or running of test in the way that process terminations potentially
caused by C library functions does not influence the main testing process.

Some tests will run without any data but many tests require
the small (basic) version of GRASS GIS sample Location for North Carolina
(see `GRASS GIS sample data`).

Basic example
-------------

If you are writing a test of a GRASS module,
create a Python script with the content derived from the example below.
When using existing existing maps, suppose you are in North Carolina SPM
GRASS sample location.

The file can contain one or more test case classes. Each class
can contain one or more test methods (functions).
Here we create one test case class with one test method.
The other two methods are class methods ensuring the right environment
for all test methods inside a test case class.
When a test file becomes part of source code (which is the usual case)
it must be placed into a directory named ``testsuite``.

.. code-block:: python

    from grass.gunittest.case import TestCase
    from grass.gunittest.main import test


    # test case class must be derived from grass.gunittest.TestCase
    class TestSlopeAspect(TestCase):

        @classmethod
        def setUpClass(cls):
            """Ensures expected computational region"""
            # to not override mapset's region (which might be used by other tests)
            cls.use_temp_region()
            # cls.runModule or self.runModule is used for general module calls
            cls.runModule('g.region', raster='elevation')
            # note that the region set by default for NC location is the same as
            # the elevation raster map, this is an example shows what to do
            # in the general case

        @classmethod
        def tearDownClass(cls):
            cls.del_temp_region()

        # test method must start with test_
        def test_limits(self):
            """Test that slope and aspect are in expected limits"""
            # we don't have to delete (g.remove) the maps
            # but we need to use unique names within one test file
            slope = 'limits_slope'
            aspect = 'limits_aspect'
            # self.assertModule is used to call module which we test
            # we expect module to finish successfully
            self.assertModule('r.slope.aspect', elevation='elevation',
                              slope=slope, aspect=aspect)
            # function tests if map's min and max are within expected interval
            self.assertRasterMinMax(map=slope, refmin=0, refmax=90,
                                    msg="Slope in degrees must be between 0 and 90")
            self.assertRasterMinMax(map=aspect, refmin=0, refmax=360,
                                    msg="Aspect in degrees must be between 0 and 360")


    if __name__ == '__main__':
        test()

In the example we have used only two assert methods, one to check that
module runs and end successfully and the other to test that map values are
within an expect interval. There is a much larger selection of assert methods
in :class:`~gunittest.case.TestCase` class documentation
and also in Python `unittest`_ package documentation.

To run the test, run GRASS GIS, use NC SPM sample location and create
a separate mapset (name it ``test`` for example). Then go to the directory
with the test file and run it:

.. code-block:: sh

    python some_test_file.py

The output goes to the terminal in this case. Read further to see
also more advanced ways of invoking the tests.

We have shown a test of a GRASS module using NC sample location.
However, tests can be written also for C and Python library and also
for internal functions in modules. See the rests of this document
for a complete guide.


Building blocks and terminology
-------------------------------

.. note::
    Some parts of the terminology should be revised to ensure understanding and
    acceptance.

test function and test method
    A *test function* is a test of one particular feature or a test of
    one particular result.
    A *test function* is referred to as *test method*, *individual test*
    or just *test*.

assert function and assert method
    An *assert function* (or *assert method*) refers to a function
    which checks that some predicate is fulfilled. For example,
    predicate can be that two raster maps does not differ from each
    other or that module run ends with successfully.

test case
    The test methods testing one particular topic or feature are in one
    test case class.

    From another point of view, one test case class contains all tests
    which require the same preparation and cleanup steps.
    In other words, a *test case* class contains all tests which are
    using the same *test fixture*.

    There is also a general :class:`~gunittest.case.TestCase` class which
    all concrete test case classes should inherit from to get all
    GRASS-specific testing functionality and also to be found
    by the testing framework.

test suite
    A *test suite*, or also *testsuite*, is a set of tests focused on one
    topic, functionality or unit (similarly to test case).
    In GRASS GIS, it is a set of files in one ``testsuite`` directory.
    The test files in one ``testsuite``
    directory are expected to test what is in the parent directory
    of a given ``testsuite`` directory. This is used to organize
    tests in the source code and also to generate test reports.

    The term *test suite* may also refer to ``TestSuite`` class
    which is part of Python `unittest`_ test invocation mechanism
    used by `gunittest` internally.

    More generally, a *test suite* is a group of test cases or any tests
    (test methods, test cases and other test suites) in one or more files.

test file
    A *test file* is a Python script executable as a standalone process.
    It does not set up any special environment and runs where it was invoked.
    The testing framework does not rely on the file to end in a standard
    way which means that if one file ends with segmentation fault
    the testing framework can continue in testing of other test files.
    Test files are central part `gunittest` system and are also the biggest
    difference from Python `unittest`_. Test file name should be unique
    but does not have to contain all parent directory names, for example
    it can consist from a simplified name of a module plus a word or two
    describing which functionality is tested. The name should not contain
    dots (except for the ``.py`` suffix).

    Alternatively, a test file could be called *test script* or
    *test module* (both in Python and GRASS sense) but note that
    none of these is used.

test runner and test invoker
    Both *test runner* and *test invoker* refer to classes, functions or
    scripts used to run (invoke) tests or test files. One of the terms may
    fade of in the future (probably *invoke* because it is not used by
    Python `unittest`_).

test fixture (test set up and tear down)
    The preparation of the test is called *setup* or *set up* and the cleaning
    after the test is called *teardown* or *tear down*. A *test fixture* refers
    to these two steps and also to the environment where the test or tests
    are executed.

    Each test case class can define ``setUp``, ``setUpClass``, ``tearDown``
    and ``tearDownClass`` methods to implement preparation and cleanup
    steps for tests it contains. The methods ending with ``Class`` are
    class methods (in Python terminology) and should be defined using
    ``@classmethod`` decorator and with ``cls`` as first argument. These
    methods are executed once for the whole class while the methods
    without ``Class`` are executed for each test method.

    In GRASS GIS, the preparation may, but does not have to, contain imports
    of maps, using temporary region, setting computational region,
    or generating random maps. The cleanup step should remove temporary
    region as well as remove all created maps and files.

test report
    A *test report* is a document or set of documents with results of
    all executed tests together with additional information such as output
    of test.

    Note that also *test result* is used also used in similar context
    because the class responsible for representing or creating the report
    in Python `unittest`_ package is called ``TestResult``.

test failure and test error
    A *test failure* occurs when a assert fails, e.g. value of
    a parameter given to ``assertTrue()`` function is ``False``.
    A *test error* occurs when something what is not tested fails,
    i.e. when exception is risen for example preparation code or
    a test method itself.

.. _test-general:

Testing with gunittest package in general
-----------------------------------------

The tests should be in files in a ``testsuite`` directory which is a subdirectory
of the directory with tested files (module, package, library). Each test file
(testing file) can have can have several test cases (testing classes).
All test file names should have pattern ``test*.py`` or ``*.py``
if another naming convention seems more appropriate.

GRASS GIS `gunittest` package and testing framework is similar to the standard
Python ``unittest`` package, so the ways to build tests are very similar.
Test methods are in a test test case class and each test method tests one
think using one or more assert methods.

::

    from grass.gunittest.case import TestCase
    from grass.gunittest.main import test


    class TestPython(TestCase):

        def test_counting(self):
            """Test that Python can count to two"""
            self.assertEqual(1 + 1, 2)


    if __name__ == '__main__':
        test()

Each test file should be able to run by itself accept certain set of command
line parameters (currently none). This is done using
``if __name__ == '__main__'`` and  ``gunittest.test()`` function.

To run a test file, start GRASS session in the location and mapset suitable for
testing (typically, NC sample location) and go to the test file's directory
(it will be usually some ``testsuite`` directory in the source code)
and run it as a Python script::

    python test_something.py

When running individual test files, it is advisable to be in a separate
mapset, so for example when using NC sample location, you should use
a new mapset of arbitrary name but not one of the predefined mapsets).

To run all tests in the source tree, you have to be in the source code
directory where you want to find tests, also you need to be inside
a GRASS session and use command similar to this one::

    python -m grass.gunittest.main --location nc_spm_grass7 --location-type nc

All test files in all ``testsuite`` directories will be executed and
a report will be created in a newly created ``testreport`` directory.
Open the file ``testreport/index.html`` to browse though the results.
Note that again you need to be in GRASS session to run the tests in this way.

The ``--location-type`` parameter serves to filter tests according to data
they can run successfully with. It is ignored for tests which does not have
this specified.

In this case each running test file gets its own mapset and
current working directory but all run are in one location.

.. warning::
    The current location is ignored but you should not run tests
    in the location which is precious to you for the case that something fails
    and current location is used for tests.

When your are writing tests you can rely on having maps which are present
in the NC sample location, or you can generate random maps. You can also
import your data which you store inside ``data`` directory inside the
given ``testsuite`` directory (for maps, ASCII formats are usually used).
If you can create tests independent on location projection and location data
it is much better then relying on given data but it is not at all required
and all approaches are encouraged.

Whenever possible it is advantageous to use available assert methods.
GRASS-specific assert methods are in :class:`gunittest.case.TestCase` class.
For general assert methods refer to Python `unittest`_ package documentation.
Both are used in the same way; they are methods of a given test case class.
In cases (which should be rare) when no assert methods fits the purpose,
you can use your own checking finalized with a call of ``assertTrue()``
or ``assertFalse()`` method with the ``msg`` parameter parameter set
to an informative message.

When you are using multiple assert methods in one test method, you must
carefully consider what assert methods are testing and in which order
you should put them. Consider the following example::

    # incorrect order
    def test_map_in_list_wrong(self):
        maps = get_list_of_maps()
        self.assertIn('elevation', maps)
        # there is no point in testing that
        # if list (or string) was empty or None execution of test ended
        # at the line with assertIn
        self.assertTrue(maps)

    # correct order
    def test_map_in_list_correct(self):
        maps = get_list_of_maps()
        # see if list (or string) is not empty (or None)
        self.assertTrue(maps)
        # and then see if the list fulfills more advanced conditions
        self.assertIn('elevation', maps)

If you are not sure when you would use multiple asserts consider the case
when using only ``assertIn()`` function::

    def test_map_in_list_short(self):
        maps = get_list_of_maps()
        self.assertIn('elevation', maps)

If the list (or string) is empty, the test fails and the message says
something about ``elevation''`` not being in the ``maps`` but
it might be much more useful if it would tell us that the list ``maps``
does not contain any items. In case of ``maps`` being ``None``, the situation
is more complicated since we using ``assertIn`` with ``None`` will
cause test error (not only failure). We must consider what is
expected behavior of ``get_list_of_maps()`` function and what
we are actually testing. For example, if we would be testing function
interface, we probably should test separately different possibilities
using ``assertIsNotNone()`` and then ``assertTrue()`` and then anything else.

Another reason for using multiple assert methods is that we may want to
test different qualities of a result. Following the previous example,
we can test that a list contains some map and does not contain some other.
If you are testing a lot of things and they don't have any clear order
or dependencies, it might be more advantageous to split
testing into several testing methods and do the preparation (creating a list
in our example) in ``setUpClass()`` or ``setUp()`` method.


.. _test-module:

Tests of GRASS modules
----------------------

This is applicable for both GRASS modules written in C or C++ and
GRASS modules written in Python since we are testing the whole module
(which is invoked as a subprocess).

::

    def test_elevation(self):
        self.assertModule('r.info', map='elevation', flags='g')
        ...

Use method ``assertRasterMinMax()`` to test that a result is within
expected range. This is a very general test which checks the basic
correctness of the result and can be used with different maps
in different locations.

::

    def test_slope_limits(self):
        slope = 'limits_slope'
        self.assertModule('r.slope.aspect', elevation='elevation',
                          slope=slope)
        self.assertRasterMinMax(map=slope, refmin=0, refmax=90,
                                msg="Slope in degrees must be between 0 and 90")

.. todo::
    Add example of assertions of key-value results.

Especially if a module has a lot of different parameters allowed
in different combinations, you should test the if the wrong ones are really
disallowed and proper error messages are provided (in addition, you can
test things such as creation and removal of maps in error states).

::

    from grass.gunittest.gmodules import SimpleModule

    class TestRInfoParameterHandling(TestCase):
        """Test r.info handling of wrong input of parameters."""

        def test_rinfo_wrong_map(self):
            """Test input of map which does not exist."""
            map_name = 'does_not_exist'
            # create a module instance suitable for testing
            rinfo = SimpleModule('r.info', map=map_name, flags='g')
            # test that module fails (ends with non-zero return code)
            self.assertModuleFail(rinfo)
            # test that error output is not empty
            self.assertTrue(rinfo.outputs.stderr)
            # test that the right map is mentioned in the error message
            self.assertIn(map_name, stderr)

In some cases it might be advantageous to create a module instance
in `setUp()` method and then modify it in test methods.

.. note:
    Test should be (natural) language, i.e. locale, independent
    to allow testing the functionality under different locale settings.
    So, if you are testing content of messages (which should be usually
    translated), use `assertIn()` method (regular expression might be
    applicable in some cases but in most cases `in` is exactly the
    operation needed).

.. _test-c:

Tests of C and C++ code
-----------------------

There are basically two possibilities how to test C and C++ code.
If you are testing GRASS library code the functions which are part of API
these functions are exposed through Python ``ctypes`` and thus can be tested
in Python. See section :ref:`test-python` for reference.

However, more advantageous and more preferable (although sometimes
more complicated) solution is to write a special program, preferably
GRASS module (i.e., using ``G_parser``). The dedicated program can
provide more direct interface to C and C++ functions used by
a GRASS module then the module and can also serve for doing benchmarks
which are not part of the testing.
This can approach can be applied to both

See the example in ``lib/raster3d`` GRASS source code directory
to create a proper Makefiles. A ``main()`` function should be written
in the same way as for a standard module.

Having a GRASS module for the purpose of testing you can write test
as if it would be standard GRASS module.


.. _test-python:

Tests of Python code
--------------------

For testing of Python code contained in some package, use
`gunittest` in the same way as `unittest`_ would be used.
This basically means that if you will write tests of Python functions
and C functions exposed to Python
through ``ctypes`` API, you might want to focus more on `unittest`_
documentation since you will perhaps need the more standard
assert functions rather then the GRASS-specific ones.


.. _test-doctest:

Testing Python code with doctest
--------------------------------

.. note::
    The primary use of ``doctest`` is to ensure that the documentation
    for functions and classes is valid. Additionally, it can increase
    the number of tests when executed together with other tests.

In Python, the easiest thing to test are functions which performs some
computations or string manipulations, i.e. they have some numbers or strings
on the input and some other numbers or strings on the output.

At the beginning you can use doctest for this purpose. The syntax is as follows::

    def sum_list(list_to_sum):
        """Here is some documentation in docstring.

        And here is the test::

        >>> sum_list([2, 5, 3])
        10
        """

In case of GRASS modules which are Python scripts, you can add something like
this to your script::

    if __name__ == "__main__":
        if len(sys.argv) == 2 and sys.argv[1] == '--doctest':
            import doctest
            doctest.testmod()
        else:
           main()

No output means that everything was successful. Note that you cannot use all
the ways of running doctest since doctest will fail don the module file due
to the dot or dots in the file name. Moreover, it is sometimes required that
the file is accessible through sys.path which is not true for case of GRASS modules.

However, do not use use doctest for tests of edge cases, for tests which require
generate complex data first, etc. In these cases use `gunittest`.


.. _test-as-scripts:

Tests as general scripts
------------------------

GRASS testing framework supports also general Python or Shell scripts
to be used as test files. This is strongly discouraged because it
is not using standard ``gnunittest`` assert functions which only leads
to reimplementing the functionality, relying on a person examining the data,
or improper tests such as mere testing
if the module executed without an error without looking at the actual results.
Moreover, the testing framework does not have a control over what is
executed and how which limits potential usage and features of testing
framework. Doing this also prevents testing framework from creating a
detailed report and thus better understanding of what is tested and what is
failing. Shell scripts are also harder to execute on MS Windows where the
interpreter might not be available or might not be on path.

The testing framework uses Shell interpreter with ``-e`` flag when executing
the tests, so the tests does not have to use ``set -e`` and can rely on it being
set from outside. The flag ensures that if some command fails, i.e. ends with
non-zero return code (exit status), the execution of the script ends too.
The testing framework also uses ``-x`` flag to print the executed commands
which usually makes examining of the test output easier.

If multiple test files are executed using ``grass.gunittest.main`` module,
the testing framework creates a temporary Mapset for the general Python and
Shell scripts in the same way as it does for ``gunittest``-based test files.
When the tests are executed separately, the clean up in current Mapset
and current working directory must be ensured by the user or the script itself
(which is generally true for all test files).

.. warning::
    This is a bad practice which prevents creation of detailed reports and
    usage of advanced ``gunittest`` features, so you should avoid it
    whenever possible.


Data
----

.. note::
    Both the section and the practice itself are under development.

Most of the tests requires some input data. However, it is good to write
a test in the way that it is independent on the available data.
In case of GRASS, we have we can have tests of functions where
some numbers or strings are input and some numbers or string are output.
These tests does not require any data to be provided since the numbers
can be part of the test. Then we have another category of tests, typically
tests of GRASS modules, which require some maps to be on the input
and thus the output (and test) depends on the specific data.
Again, it it best to have tests which does not require any special data
or generally environment settings (e.g. geographic projection)
but it is much easier to write good tests with a given set of data.
So, an compromises must be made and tests of different types should be written.

In the GRASS testing framework, each test file should be marked according to
category it belongs to. Each category corresponds to GRASS location or locations
where the test file can run successfully.

Universal tests
    First category is *universal*. The tests in this category use some some
    hard coded constants, generated data, random data, or their own imported
    data as in input to function and GRASS modules. All the tests, input data
    and reference results should be projection independent. These tests will
    runs always regardless of available locations.

Standard names tests
    Second category are tests using *standard names*. Tests rely on a
    certain set of maps with particular names to be present in the location.
    Moreover, the tests can rely also on the (semantic) meaning of the
    names, i.e. raster map named elevation will always contain some kind of
    digital elevation model of some area, so raster map elevation can be
    used to compute aspect. In other words, these tests should be able to
    (successfully) run in any location with a maps named in the same way as
    in the standard testing location(s).

Standard data tests
    Third category of tests rely on *standard data*. These tests expect that the
    GRASS location they run in not only contains the maps with particular names
    as in the *standard names* but the tests rely also on the data being the
    same as in the standard testing location(s). However, the (geographic)
    projection or data storage can be different. This is expected to be the
    most common case but it is much better if the tests is one of the previous
    categories (*universal* or *standard names*). If it is possible the
    functions or modules with tests in this category should have also tests
    which will fit into one of the previous categories, even though these
    additional tests will not be as precise as the other tests.

Location specific tests
    Finally, there are tests which requires certain concrete location. There
    is (or will be) a set of standard testing locations each will have the same
    data (maps) but the projections and data storage types will be different.
    The suggested locations are: NC sample location in SPM projection,
    NC in SPF, NC in LL, NC in XY, and perhaps NC in UTM, and NC in some
    custom projection (in case of strange not-fitting projection, there is
    a danger that the results of analyses can differer significantly).
    Moreover, the set can be extended by GRASS locations which are using
    different storage backends, e.g. PostGIS for vectors and PostgreSQL for
    temporal database. Tests can specify one or preferably more of these
    standard locations.

Specialized location tests
    Additionally, an specialized location with a collection of strange,
    incorrect, or generally extreme data will be provided. In theory, more
    than one location like this can be created if the data cannot be
    together in one location or if the location itself is somehow special,
    e.g. because of projection.

Each category, or perhaps each location (will) have a set of external data
available for import or other purposes. The standardization of this data
is in question and thus this may be specific to each location or this
can be a separate resource common to all tests using one of the standardized
locations, or alternatively this data can be associated with the location
with special data.

.. note::
    The more general category you choose for your tests the more testing data
    can applied to your tests and the more different circumstances can be tried
    with your tests.


Data specific to one test
-------------------------

If the data required by the test are not part of standard location
and cannot be part of the test file itself, this data should be stored
in files in ``data`` subdirectory of ``testsuite`` directory.
The test should access the data using a relative path from its location,
i.e. all data will be accessed using ``data/...``. This ``data`` directory
might be used directly when running test file directly in the directory
in the source code or might be copied to the test current working directory
when running tests by the main test invoking tool.


Tests creating separate Mapsets, Locations and GRASS Databases
--------------------------------------------------------------

If test is creating a custom Mapset or Mapsets, it can create them in
the current Location or create a custom GRASS Database in the current
directory. In any case, test has to take care of cleaning up (deleting)
the created directories and it has to use names which will be unique
enough (name of the test case class or the file is probably a good choice
but completely unique identifier is probably much better).

If test needs custom Location or it tests something related to GRASS Database,
it must always create a new GRASS Database in the current directory.

In any case, the author must try the tests cautiously and several times
in the same Location to see if everything works as expected. Testing
framework is using Mapsets to separate the tests and the functions
does not explicitly check for the case where a test is using different
Mapset then the one which has been given to it by the framework.


Analyzing quality of source code
--------------------------------

Besides testing, you can also use some tools to check the quality of your code
according to various standards and occurrence of certain code patterns.

For C/C++ code we additionally use the third party solution `Coverity Scan`_
where GRASS GIS is registered as project number `1038`_. Also you can use
`Cppcheck`_ which will show a lot of errors which compilers do not check.
In any case, set your compiler to high error and warning levels,
check them and fix them in your code. Furthermore, continuous integrations is
used to check if the source code can still be compiled after submitting changes
to the repository.

For Python, we recommend pylint and then for style issues pep8 tool
(and perhaps also pep257 tool). However, there is more tools available
you can use them together with the recommend ones.

To provide a way to evaluate the Python source code in the whole GRASS source
tree there is a Python script ``grass_py_static_check.py`` which uses
pylint and pep8 with GRASS-specific settings. Run the tool in GRASS session
in the source code root directory. A HTML report will be created in
``pylint_report`` directory.

::

    grass_py_static_check.py

.. note::
    ``grass_py_static_check.py`` is available in `sandbox`_.

Additionally, if you are invoking your Python code manually using python command,
e.g. when testing, use parameters::

    python -Qwarn -tt -3 some_module.py

This will warn you about usage of old division semantics for integers
and about incompatibilities with Python 3 (if you are using Python 2)
which 2to3 tool cannot fix. Finally, it will issue errors if are using tabs
for indentation inconsistently (note that you should not use tabs for
indentation at all).


Further reading
---------------

.. toctree::
   :maxdepth: 2

   gunittest
   gunittest_running_tests


.. _unittest: https://docs.python.org/2/library/unittest.html
.. _doctest: https://docs.python.org/2/library/doctest.html
.. _Coverity Scan: https://scan.coverity.com/
.. _1038: https://scan.coverity.com/projects/1038
.. _Cppcheck: http://cppcheck.sourceforge.net/
.. _sandbox: https://svn.osgeo.org/grass/sandbox/wenzeslaus/grass_py_static_check.py
.. _GRASS GIS sample data: https://grass.osgeo.org/download/data/ and http://fatra.cnr.ncsu.edu/data/ (nc_spm_full_v2alpha)
