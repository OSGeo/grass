"""
Temporal framework doctests
"""

import doctest
import sys

import grass.gunittest.case
import grass.gunittest.main
import grass.gunittest.utils
import grass.temporal as tgis

doctest.DocFileCase = type(
    "DocFileCase", (grass.gunittest.case.TestCase,), dict(doctest.DocFileCase.__dict__)
)
doctest.SkipDocTestCase = type(
    "SkipDocTestCase",
    (grass.gunittest.case.TestCase,),
    dict(doctest.SkipDocTestCase.__dict__),
)


def load_tests(loader, tests, ignore):
    grass.gunittest.utils.do_doctest_gettext_workaround()
    tests.addTests(doctest.DocTestSuite(tgis.abstract_dataset))
    tests.addTests(doctest.DocTestSuite(tgis.abstract_map_dataset))
    tests.addTests(doctest.DocTestSuite(tgis.abstract_space_time_dataset))
    tests.addTests(doctest.DocTestSuite(tgis.base))
    tests.addTests(doctest.DocTestSuite(tgis.core))
    tests.addTests(doctest.DocTestSuite(tgis.datetime_math))
    # Unexpected error here
    # tests.addTests(doctest.DocTestSuite(tgis.list_stds))
    tests.addTests(doctest.DocTestSuite(tgis.metadata))
    tests.addTests(doctest.DocTestSuite(tgis.register))
    tests.addTests(doctest.DocTestSuite(tgis.space_time_datasets))
    tests.addTests(doctest.DocTestSuite(tgis.spatial_extent))
    tests.addTests(doctest.DocTestSuite(tgis.spatial_topology_dataset_connector))
    tests.addTests(doctest.DocTestSuite(tgis.spatio_temporal_relationships))
    tests.addTests(doctest.DocTestSuite(tgis.temporal_extent))
    tests.addTests(doctest.DocTestSuite(tgis.temporal_granularity))
    tests.addTests(doctest.DocTestSuite(tgis.temporal_topology_dataset_connector))
    # Algebra is still very experimental
    tests.addTests(doctest.DocTestSuite(tgis.temporal_algebra))
    tests.addTests(doctest.DocTestSuite(tgis.temporal_raster3d_algebra))
    tests.addTests(doctest.DocTestSuite(tgis.temporal_raster_algebra))
    tests.addTests(doctest.DocTestSuite(tgis.temporal_raster_base_algebra))
    tests.addTests(doctest.DocTestSuite(tgis.temporal_operator))
    tests.addTests(doctest.DocTestSuite(tgis.temporal_vector_algebra))
    tests.addTests(doctest.DocTestSuite(tgis.c_libraries_interface))
    return tests


if __name__ == "__main__":
    # Temporary deactivated for Python 3 cause it stalls
    if sys.version_info[0] >= 3:
        pass
    else:
        grass.gunittest.main.test()
