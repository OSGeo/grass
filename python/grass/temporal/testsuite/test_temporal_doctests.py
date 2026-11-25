"""
Temporal framework doctests
"""

import doctest

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


def _doctest_suite(module):
    # The docstrings use names from the grass.temporal namespace, such as init().
    return doctest.DocTestSuite(module, globs={**vars(tgis), **vars(module)})


def load_tests(loader, tests, ignore):
    grass.gunittest.utils.do_doctest_gettext_workaround()
    tests.addTests(_doctest_suite(tgis.abstract_dataset))
    tests.addTests(_doctest_suite(tgis.abstract_map_dataset))
    tests.addTests(_doctest_suite(tgis.abstract_space_time_dataset))
    tests.addTests(_doctest_suite(tgis.base))
    tests.addTests(_doctest_suite(tgis.core))
    tests.addTests(_doctest_suite(tgis.datetime_math))
    # Unexpected error here
    # tests.addTests(_doctest_suite(tgis.list_stds))
    tests.addTests(_doctest_suite(tgis.metadata))
    tests.addTests(_doctest_suite(tgis.register))
    tests.addTests(_doctest_suite(tgis.space_time_datasets))
    tests.addTests(_doctest_suite(tgis.spatial_extent))
    tests.addTests(_doctest_suite(tgis.spatial_topology_dataset_connector))
    tests.addTests(_doctest_suite(tgis.spatio_temporal_relationships))
    tests.addTests(_doctest_suite(tgis.temporal_extent))
    tests.addTests(_doctest_suite(tgis.temporal_granularity))
    tests.addTests(_doctest_suite(tgis.temporal_topology_dataset_connector))
    # Algebra is still very experimental
    tests.addTests(_doctest_suite(tgis.temporal_algebra))
    tests.addTests(_doctest_suite(tgis.temporal_raster3d_algebra))
    tests.addTests(_doctest_suite(tgis.temporal_raster_algebra))
    tests.addTests(_doctest_suite(tgis.temporal_raster_base_algebra))
    tests.addTests(_doctest_suite(tgis.temporal_operator))
    tests.addTests(_doctest_suite(tgis.temporal_vector_algebra))
    tests.addTests(_doctest_suite(tgis.c_libraries_interface))
    return tests


if __name__ == "__main__":
    grass.gunittest.main.test()
