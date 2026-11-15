"""Tests for the clustering algorithm step ctypes bindings

Most of these functions call G_debug() internally, which needs an
answer for the session's DEBUG variable even though none of them read or
write a project or mapset; see conftest.py's libgis_without_session
fixture for how that is arranged.

The sample data is deliberately two-band rather than one-band: every one
of these functions loops over bands, and with a single band a loop that
only ever handled band 0 would still produce correct results.
"""

from ctypes import byref, c_double, c_int

import pytest

import grass.lib.cluster as libcluster

# Five points whose second band is ten times the first, so that values
# for the two bands stay easy to check by hand but remain distinguishable
# from each other.
SAMPLE_POINTS = [(1.0, 10.0), (2.0, 20.0), (3.0, 30.0), (4.0, 40.0), (5.0, 50.0)]


def cluster_with_points(points, nclasses, nbands=2):
    """A Cluster struct loaded with points and allocated for nclasses

    Mirrors the first few steps of I_cluster_exec(): collect the points,
    then allocate the per-class arrays that the rest of the algorithm's
    steps read and write.
    """
    cluster = libcluster.Cluster()
    libcluster.I_cluster_begin(byref(cluster), nbands)
    for values in points:
        assert (
            libcluster.I_cluster_point(byref(cluster), (c_double * nbands)(*values))
            == 0
        )
    cluster.nclasses = nclasses
    assert libcluster.I_cluster_exec_allocate(byref(cluster)) == 1
    return cluster


def classify(cluster):
    """Run I_cluster_means() then I_cluster_assign(), as I_cluster_exec()
    does for its first pass, and return the resulting per-point classes"""
    libcluster.I_cluster_means(byref(cluster))
    interrupted = c_int(0)
    libcluster.I_cluster_assign(byref(cluster), byref(interrupted))
    class_field = getattr(cluster, "class")
    return [class_field[p] for p in range(cluster.npoints)]


def per_class(array, band, nclasses):
    """Read one band's row out of a band-by-class array"""
    return [array[band][c] for c in range(nclasses)]


def test_means_spreads_initial_class_means_by_standard_deviation(
    libgis_without_session,
) -> None:
    """I_cluster_means() has no classified points to average yet, so it
    seeds each class mean at an even spread around the overall mean of
    each band, scaled by that band's standard deviation"""
    cluster = cluster_with_points(SAMPLE_POINTS, 3)

    libcluster.I_cluster_means(byref(cluster))

    assert per_class(cluster.mean, 0, 3) == pytest.approx(
        [1.4188611699158102, 3.0, 4.58113883008419]
    )
    assert per_class(cluster.mean, 1, 3) == pytest.approx(
        [14.188611699158104, 30.0, 45.811388300841898]
    )


def test_assign_puts_each_point_in_its_nearest_class(libgis_without_session) -> None:
    cluster = cluster_with_points(SAMPLE_POINTS, 3)

    classes = classify(cluster)

    assert classes == [0, 0, 1, 2, 2]
    assert [cluster.count[c] for c in range(3)] == [2, 1, 2]
    assert per_class(cluster.sum, 0, 3) == [3.0, 3.0, 9.0]
    assert per_class(cluster.sum, 1, 3) == [30.0, 30.0, 90.0]


def test_reassign_leaves_already_nearest_points_alone(libgis_without_session) -> None:
    """I_cluster_reassign() is the iterated form of I_cluster_assign():
    it compares each point against the class sums rather than the seeded
    means, and reports how many points changed class. Points that are
    already in their nearest class stay put and are not counted.
    """
    cluster = cluster_with_points(SAMPLE_POINTS, 3)
    classify(cluster)
    interrupted = c_int(0)

    changes = libcluster.I_cluster_reassign(byref(cluster), byref(interrupted))

    assert changes == 0
    class_field = getattr(cluster, "class")
    assert [class_field[p] for p in range(cluster.npoints)] == [0, 0, 1, 2, 2]
    assert [cluster.count[c] for c in range(3)] == [2, 1, 2]


def test_reassign_moves_a_point_to_a_nearer_class(libgis_without_session) -> None:
    """A point is moved when another class centroid is closer across all
    bands together, and the counts and sums of both classes are updated

    The single point (1, 9) sits nearer to class 0 on band 0 alone
    (distance 1 against 4), but nearer to class 1 once band 1 is included
    (5 against 82), so this also pins down that the comparison really
    spans every band rather than just the first.
    """
    cluster = cluster_with_points([(1.0, 9.0)], 2)
    # Hand-set the two class centroids: class 0 at (0, 0) and class 1 at
    # (3, 10), each standing in for 2 points, since I_cluster_reassign()
    # reads the class sums and counts rather than the means.
    cluster.count[0] = 2
    cluster.sum[0][0] = 0.0
    cluster.sum[1][0] = 0.0
    cluster.count[1] = 2
    cluster.sum[0][1] = 6.0
    cluster.sum[1][1] = 20.0
    class_field = getattr(cluster, "class")
    class_field[0] = 0
    interrupted = c_int(0)

    changes = libcluster.I_cluster_reassign(byref(cluster), byref(interrupted))

    assert changes == 1
    assert class_field[0] == 1
    assert [cluster.count[c] for c in range(2)] == [1, 3]
    assert per_class(cluster.sum, 0, 2) == [-1.0, 7.0]
    assert per_class(cluster.sum, 1, 2) == [-9.0, 29.0]


def test_sum2_accumulates_the_sum_of_squares_per_class(libgis_without_session) -> None:
    """Unlike GASTATS's variance-style statistics, I_cluster_sum2() stores
    the plain sum of squares per class; I_cluster_separation() derives
    variance from it itself"""
    cluster = cluster_with_points(SAMPLE_POINTS, 3)
    classify(cluster)

    libcluster.I_cluster_sum2(byref(cluster))

    assert per_class(cluster.sum2, 0, 3) == [5.0, 9.0, 41.0]
    assert per_class(cluster.sum2, 1, 3) == [500.0, 900.0, 4100.0]


def test_separation_returns_the_far_sentinel_for_a_tiny_class(
    libgis_without_session,
) -> None:
    """A class with fewer than 2 points has no variance to compare, so
    I_cluster_separation() reports it as maximally separated (-1.0)
    rather than dividing by zero"""
    cluster = cluster_with_points(SAMPLE_POINTS, 3)
    classify(cluster)
    libcluster.I_cluster_sum2(byref(cluster))

    # Class 1 holds only the single point (3, 30), see the assign test.
    assert libcluster.I_cluster_separation(byref(cluster), 0, 1) == -1.0


def test_separation_measures_how_distinguishable_two_classes_are(
    libgis_without_session,
) -> None:
    cluster = cluster_with_points(SAMPLE_POINTS, 3)
    classify(cluster)
    libcluster.I_cluster_sum2(byref(cluster))

    # Classes 0 and 2 hold two points each, and the separation sums the
    # contribution of both bands.
    separation = libcluster.I_cluster_separation(byref(cluster), 0, 2)

    assert separation == pytest.approx(1.2247448713915889)


def test_distinct_finds_the_closest_pair_below_the_separation_threshold(
    libgis_without_session,
) -> None:
    cluster = cluster_with_points(SAMPLE_POINTS, 3)
    classify(cluster)
    libcluster.I_cluster_sum2(byref(cluster))

    # A generous threshold finds classes 0 and 2 as the closest pair with
    # at least 2 points each (class 1 is skipped: it only has one point).
    assert libcluster.I_cluster_distinct(byref(cluster), 100.0) == 0
    assert (cluster.merge1, cluster.merge2) == (0, 2)

    # A threshold tighter than any actual separation finds nothing to merge.
    assert libcluster.I_cluster_distinct(byref(cluster), 0.0001) == 1


def test_nclasses_counts_classes_at_or_above_a_minimum_size(
    libgis_without_session,
) -> None:
    cluster = cluster_with_points(SAMPLE_POINTS, 3)
    classify(cluster)

    # Classes 0 and 2 have 2 points each; class 1 has only 1.
    assert libcluster.I_cluster_nclasses(byref(cluster), 2) == 2


def test_merge_combines_two_classes(libgis_without_session) -> None:
    cluster = cluster_with_points(SAMPLE_POINTS, 3)
    classify(cluster)
    libcluster.I_cluster_sum2(byref(cluster))
    libcluster.I_cluster_distinct(byref(cluster), 100.0)  # sets merge1/merge2 to 0, 2

    libcluster.I_cluster_merge(byref(cluster))

    class_field = getattr(cluster, "class")
    # Every point that was in class 2 now reports class 0; class 2's count
    # and per-band sums move into class 0 and are zeroed at their old slot.
    assert [class_field[p] for p in range(cluster.npoints)] == [0, 0, 1, 0, 0]
    assert [cluster.count[c] for c in range(3)] == [4, 1, 0]
    assert per_class(cluster.sum, 0, 3) == [12.0, 3.0, 0.0]
    assert per_class(cluster.sum, 1, 3) == [120.0, 30.0, 0.0]


def test_reclass_compacts_out_a_class_below_the_minimum_size(
    libgis_without_session,
) -> None:
    """I_cluster_reclass() removes classes below minsize and shifts the
    remaining ones down to fill the resulting gaps, remapping every
    point's class along the way"""
    cluster = cluster_with_points([(0.0, 0.0)] * 3, 3)
    # Hand-set an empty class 1 between two populated ones, rather than
    # reaching this state through classification, to isolate reclass()
    # from the rest of the algorithm.
    for class_index, (count, band0, band1) in enumerate(
        [(5, 50.0, 500.0), (0, 0.0, 0.0), (3, 30.0, 300.0)]
    ):
        cluster.count[class_index] = count
        cluster.sum[0][class_index] = band0
        cluster.sum[1][class_index] = band1
    class_field = getattr(cluster, "class")
    class_field[0] = 0
    class_field[1] = 0
    class_field[2] = 2

    ret = libcluster.I_cluster_reclass(byref(cluster), 1)

    assert ret == 0
    assert cluster.nclasses == 2
    assert [class_field[p] for p in range(3)] == [0, 0, 1]
    assert [cluster.count[c] for c in range(2)] == [5, 3]
    assert per_class(cluster.sum, 0, 2) == [50.0, 30.0]
    assert per_class(cluster.sum, 1, 2) == [500.0, 300.0]


def test_reclass_returns_1_when_every_class_already_meets_the_minimum(
    libgis_without_session,
) -> None:
    cluster = cluster_with_points([(0.0, 0.0)], 2)
    cluster.count[0] = 5
    cluster.count[1] = 3

    assert libcluster.I_cluster_reclass(byref(cluster), 1) == 1
    assert cluster.nclasses == 2
