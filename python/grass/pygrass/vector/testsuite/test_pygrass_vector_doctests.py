# -*- coding: utf-8 -*-
"""
Tests checkers
"""

import doctest

import grass.gunittest.case
import grass.gunittest.main
import grass.gunittest.utils

import grass.pygrass.vector as gvector
import grass.pygrass.utils as gutils


# doctest does not allow changing the base classes of test case, skip test case
# and test suite, so we need to create a new type which inherits from our class
# and contains doctest's methods
# the alternative is to copy 500 from doctest and change what is needed
# (this might be necessary anyway because of the reports and stdout and stderr)
doctest.DocFileCase = type('DocFileCase',
                           (grass.gunittest.case.TestCase,),
                           dict(doctest.DocFileCase.__dict__))
doctest.SkipDocTestCase = type('SkipDocTestCase',
                               (grass.gunittest.case.TestCase,),
                               dict(doctest.SkipDocTestCase.__dict__))


def load_tests(loader, tests, ignore):
    # TODO: this must be somewhere when doctest is called, not here
    # TODO: ultimate solution is not to use _ as a buildin in lib/python
    # for now it is the only place where it works
    grass.gunittest.utils.do_doctest_gettext_workaround()

    from grass.pygrass import utils
    utils.create_test_vector_map(gvector.test_vector_name)
    utils.create_test_vector_map(gvector.abstract.test_vector_name)
    utils.create_test_vector_map(gvector.geometry.test_vector_name)
    utils.create_test_vector_map(gvector.find.test_vector_name)
    utils.create_test_vector_map(gvector.table.test_vector_name)

    # this should be called at some top level
    tests.addTests(doctest.DocTestSuite(gvector))
    tests.addTests(doctest.DocTestSuite(gvector.abstract))
    tests.addTests(doctest.DocTestSuite(gvector.basic))
    tests.addTests(doctest.DocTestSuite(gvector.find))
    tests.addTests(doctest.DocTestSuite(gvector.geometry))
    tests.addTests(doctest.DocTestSuite(gvector.sql))
    tests.addTests(doctest.DocTestSuite(gvector.table))
    return tests


if __name__ == '__main__':
    grass.gunittest.main.test()
