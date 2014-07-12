Testing GRASS GIS source code and modules
=========================================

Introduction
------------

For the testing we will be using system based on Python `unittest`_ package.
The system is not finished yet.

For now, the best way is to use just Python unittest package as is.
Additionally, it is possible to use Python `doctest`_ package
which is compatible with unittest at certain level.
Both packages are part of the standard  Python distribution.

The content of this document may become part of submitting files and
the documentation of testing framework classes and scripts.


Testing with gunittest package in general
-----------------------------------------

The tests should be in files in a ``testsuite`` directory which is a subdirectory
of the directory with tested files (module, package, library). Each testing file
(test file) can have can have several testing classes (test cases).
All test files names should have pattern ``test*.py``.

GRASS GIS `gunittest` package and testing framework is similar to the standard
Python ``unittest`` package, so the ways to build tests are very similar.

::

    from grass.guinttest import TestCase


    class TestPython(TestCase):

        def test_counting(self):
            """Test that Python can count to two"""
            self.assertEqual(1 + 1, 2)


    if __name__ == '__main__':
        from grass.guinttest import test
        test()

Each test file should be able to run by itself and accept certain set of command
line parameters. This is ensured using `gunittest.test()`.

To run (invoke) all tests in the source tree run::

    python -m grass.gunittest.main [gisdbase] location test_data_category

All test files in all ``testsuite`` directories will be executed and
a report will be created in a newly created ``testreport`` directory.
Open the file ``testreport/index.html`` to browse though the results.
You need to be in GRASS session to run the tests.

The test_data_category parameter serves to filter tests accoring to data
they can run successfully with. It is ignored for tests which does not have
this specified.

Each running test file gets its own mapset and current working directory
but all run are in one location.

.. warning::
    The current location is ignored but you should run not invoke tests
    in the location which is precious to you for the case that something fails.

To run individual tests file you should be in GRASS session in GRASS NC sample
location in a mapset of arbitrary name (except for the predefined mapsets).

Your tests can rely on maps which are present in the GRASS NC sample location.
But if you can provide tests which are independent on location it is better.

Read the documentation of Python ``unittest`` package for a list of assert
methods which you can use to test your results. For test of more complex
GRASS-specific results refer to :class:`gunittest.case.TestCase` class
documentation.


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

Especially if a module module has a lot of different parameters allowed
in different combinations, you should test the if the wrong ones are really
disallowed and proper error messages are provided (in addition, you can
test things such as creation and removal of maps in error states).

::

    from grass.gunittest import SimpleModule

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


Tests of C and C++ code
-----------------------

Tests of Python code
--------------------

Use `gunittest` for this purpose in the same way as `unittest`_ would be used.


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


Data
----

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
    Moreover, the set can be extened by GRASS locations which are using
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

.. note::
    gunittest is under development but, so some things can change, however
    this should not stop you from writing tests since the actual differences
    in your code will be only subtle.

.. note::
    gunittest is not part of GRASS GIS source code yet, it is available
    separately. If you don't want to deal with some other code now,
    just write tests based only on Python ``unittest``. This will limit
    your possibilities of convenient testing but should not stop you
    from writing tests, especially if you will write tests of Python functions,
    and C functions exposed to Python through ctypes API. (Note that it might
    be a good idea to write tests for library function you rely on in your
    GRASS module).


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


Analyzing quality of source code
--------------------------------

Besides testing, you can also use some tools to check the quality of your code
according to various standards and occurrence of certain code patterns.

For C/C++ code use third party solution `Coverity Scan`_ where GRASS GIS
is registered as project number `1038`_. Also you can use `Cppcheck`_
which will show a lot of errors which compilers do not check.
In any case, set your compiler to high error and warning levels,
check them and fix them in your code.

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


.. _unittest: https://docs.python.org/2/library/unittest.html
.. _doctest: https://docs.python.org/2/library/doctest.html
.. _Coverity Scan: https://scan.coverity.com/
.. _1038: https://scan.coverity.com/projects/1038
.. _Cppcheck: http://cppcheck.sourceforge.net/
.. _sandbox: https://svn.osgeo.org/grass/sandbox/wenzeslaus/grass_py_static_check.py
