"""Tests for the point-collection ctypes bindings

I_cluster_begin(), I_cluster_point() and the point-set variants only
allocate and fill the Cluster struct's own arrays; none of them call
G_debug(), so unlike most of the clustering algorithm itself they need
nothing from libgis' session environment and no fixture (see
conftest.py and lib_cluster_algorithm_ctypes_test.py).
"""

from ctypes import byref, c_double

import grass.lib.cluster as libcluster
import grass.lib.raster as libraster


def new_cluster(nbands):
    """A freshly initialized Cluster struct for the given band count"""
    cluster = libcluster.Cluster()
    assert libcluster.I_cluster_begin(byref(cluster), nbands) == 0
    return cluster


def point(*values):
    """Build a ctypes double array from per-band values"""
    return (c_double * len(values))(*values)


def null_point(nbands):
    """A point whose bands are all set to the DCELL NULL value"""
    values = point(*([0.0] * nbands))
    libraster.Rast_set_d_null_value(values, nbands)
    return values


def test_begin_rejects_a_non_positive_band_count() -> None:
    cluster = libcluster.Cluster()
    assert libcluster.I_cluster_begin(byref(cluster), 0) == 1


def test_point_accumulates_values_and_running_sums() -> None:
    cluster = new_cluster(2)

    assert libcluster.I_cluster_point(byref(cluster), point(1.0, 2.0)) == 0
    assert libcluster.I_cluster_point(byref(cluster), point(3.0, 4.0)) == 0

    assert cluster.npoints == 2
    assert [cluster.points[0][p] for p in range(2)] == [1.0, 3.0]
    assert [cluster.points[1][p] for p in range(2)] == [2.0, 4.0]
    # band_sum/band_sum2 are running totals kept for I_cluster_means(), not
    # recomputed from the points array, so they are checked here too.
    assert [cluster.band_sum[b] for b in range(2)] == [4.0, 6.0]
    assert [cluster.band_sum2[b] for b in range(2)] == [10.0, 20.0]


def test_point_rejects_a_point_with_a_null_band() -> None:
    """A point with any band set to NULL is rejected outright, not just
    skipped in that one band, since a partial pixel cannot be clustered"""
    cluster = new_cluster(2)

    assert libcluster.I_cluster_point(byref(cluster), null_point(2)) == 1
    assert cluster.npoints == 0


def test_point_set_batch_workflow_drops_all_zero_points() -> None:
    """I_cluster_begin_point_set()/I_cluster_point_part()/
    I_cluster_end_point_set() is the batch equivalent of repeated
    I_cluster_point() calls, used when bands are filled one at a time
    rather than one point at a time. A point whose every band is exactly
    zero (as opposed to NULL) is silently dropped, matching the "point
    contains no data" convention used elsewhere in this library.
    """
    cluster = new_cluster(2)
    values = [(1.0, 10.0), (0.0, 0.0), (2.0, 20.0)]

    assert libcluster.I_cluster_begin_point_set(byref(cluster), len(values)) == 0
    for n, bands in enumerate(values):
        for band, value in enumerate(bands):
            assert (
                libcluster.I_cluster_point_part(
                    byref(cluster), c_double(value), band, n
                )
                == 0
            )
    kept = libcluster.I_cluster_end_point_set(byref(cluster), len(values))

    assert kept == 2
    assert cluster.npoints == 2
    assert [cluster.points[0][p] for p in range(2)] == [1.0, 2.0]
    assert [cluster.points[1][p] for p in range(2)] == [10.0, 20.0]


def test_point_part_rejects_a_null_value() -> None:
    cluster = new_cluster(1)
    libcluster.I_cluster_begin_point_set(byref(cluster), 1)

    null_value = c_double()
    libraster.Rast_set_d_null_value(byref(null_value), 1)
    assert libcluster.I_cluster_point_part(byref(cluster), null_value, 0, 0) == 1


def test_clear_resets_the_structure() -> None:
    cluster = new_cluster(2)
    libcluster.I_cluster_point(byref(cluster), point(1.0, 2.0))

    assert libcluster.I_cluster_clear(byref(cluster)) == 0

    assert cluster.nbands == 0
    assert not cluster.points
    assert not cluster.band_sum
    assert not cluster.band_sum2
