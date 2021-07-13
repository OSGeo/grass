"""
Tests checkers
"""

import doctest

import grass.gunittest.case
import grass.gunittest.main
import grass.gunittest.utils

from grass.pygrass import gis
from grass.pygrass.gis import region


# doctest does not allow changing the base classes of test case, skip test case
# and test suite, so we need to create a new type which inherits from our class
# and contains doctest's methods
# the alternative is to copy 500 from doctest and change what is needed
# (this might be necessary anyway because of the reports and stdout and stderr)
doctest.DocFileCase = type(
    "DocFileCase", (grass.gunittest.case.TestCase,), dict(doctest.DocFileCase.__dict__)
)
doctest.SkipDocTestCase = type(
    "SkipDocTestCase",
    (grass.gunittest.case.TestCase,),
    dict(doctest.SkipDocTestCase.__dict__),
)


def load_tests(loader, tests, ignore):
    # TODO: this must be somewhere when doctest is called, not here
    # TODO: ultimate solution is not to use _ as a buildin in lib/python
    # for now it is the only place where it works
    grass.gunittest.utils.do_doctest_gettext_workaround()

    from grass.pygrass import utils
    from grass.script.core import run_command

    utils.create_test_vector_map(gis.test_vector_name)
    utils.create_test_vector_map(gis.region.test_vector_name)
    run_command("g.region", n=50, s=0, e=60, w=0, res=1)
    run_command(
        "r.mapcalc", expression="%s = 1" % (gis.test_raster_name), overwrite=True
    )
    run_command(
        "r.mapcalc", expression="%s = 1" % (gis.region.test_raster_name), overwrite=True
    )
    run_command("g.region", n=40, s=0, e=40, w=0, res=2)

    # this should be called at some top level
    tests.addTests(doctest.DocTestSuite(gis))
    tests.addTests(doctest.DocTestSuite(region))
    return tests


if __name__ == "__main__":
    grass.gunittest.main.test()
