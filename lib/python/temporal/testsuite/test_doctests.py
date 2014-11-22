# -*- coding: utf-8 -*-
"""
Temporal framework doctests
"""

import doctest
import grass.temporal
import grass.gunittest
import grass.gunittest.utils
import os

doctest.DocFileCase = type('DocFileCase',
                           (grass.gunittest.TestCase,),
                           dict(doctest.DocFileCase.__dict__))
doctest.SkipDocTestCase = type('SkipDocTestCase',
                               (grass.gunittest.TestCase,),
                               dict(doctest.SkipDocTestCase.__dict__))

def load_tests(loader, tests, ignore):
    grass.gunittest.utils.do_doctest_gettext_workaround()
    tests.addTests(doctest.DocTestSuite(grass.temporal.abstract_dataset))
    tests.addTests(doctest.DocTestSuite(grass.temporal.abstract_map_dataset))
    tests.addTests(doctest.DocTestSuite(grass.temporal.abstract_space_time_dataset))
    tests.addTests(doctest.DocTestSuite(grass.temporal.base))
    tests.addTests(doctest.DocTestSuite(grass.temporal.c_libraries_interface))
    # Unexpected error here
    #tests.addTests(doctest.DocTestSuite(grass.temporal.core))
    tests.addTests(doctest.DocTestSuite(grass.temporal.datetime_math))
    # Unexpected error here
    #tests.addTests(doctest.DocTestSuite(grass.temporal.list_stds))
    tests.addTests(doctest.DocTestSuite(grass.temporal.metadata))
    tests.addTests(doctest.DocTestSuite(grass.temporal.register))
    tests.addTests(doctest.DocTestSuite(grass.temporal.space_time_datasets))
    tests.addTests(doctest.DocTestSuite(grass.temporal.spatial_extent))
    tests.addTests(doctest.DocTestSuite(grass.temporal.spatial_topology_dataset_connector))
    tests.addTests(doctest.DocTestSuite(grass.temporal.spatio_temporal_relationships))
    tests.addTests(doctest.DocTestSuite(grass.temporal.temporal_extent))
    tests.addTests(doctest.DocTestSuite(grass.temporal.temporal_granularity))
    tests.addTests(doctest.DocTestSuite(grass.temporal.temporal_topology_dataset_connector))
    # Algebra is still very experimental
    tests.addTests(doctest.DocTestSuite(grass.temporal.temporal_algebra))
    tests.addTests(doctest.DocTestSuite(grass.temporal.temporal_raster3d_algebra))
    tests.addTests(doctest.DocTestSuite(grass.temporal.temporal_raster_algebra))
    tests.addTests(doctest.DocTestSuite(grass.temporal.temporal_raster_base_algebra))
    tests.addTests(doctest.DocTestSuite(grass.temporal.temporal_operator))
    tests.addTests(doctest.DocTestSuite(grass.temporal.temporal_vector_algebra))
    return tests

if __name__ == '__main__':
    grass.gunittest.test()
