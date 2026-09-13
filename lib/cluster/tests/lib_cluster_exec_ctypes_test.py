"""Tests for the full clustering workflow ctypes binding

I_cluster_exec() drives the whole algorithm end to end (initial means,
assignment, iterative re-assignment, merging non-distinct classes, and
finally building the output signatures). It calls G_debug() internally,
so it needs the same libgis_without_session fixture as
lib_cluster_algorithm_ctypes_test.py; see conftest.py for why.

The sample data is two-band for the reason given in
lib_cluster_algorithm_ctypes_test.py: a single band cannot tell a
correct per-band loop apart from one that only ever handles band 0.
"""

from contextlib import contextmanager
from ctypes import CFUNCTYPE, byref, c_double, c_int

import pytest

import grass.lib.cluster as libcluster
import grass.lib.gis as libgis

# Two groups far enough apart that the clustering result is unambiguous,
# with the second band on a different scale from the first.
GROUP_A = [(0.0, 0.0), (1.0, 1.0), (2.0, 2.0), (-1.0, -1.0), (0.5, 0.5)]
GROUP_B = [
    (100.0, 200.0),
    (101.0, 201.0),
    (99.0, 199.0),
    (100.5, 200.5),
    (99.5, 199.5),
]

# The default handler for G_warning() looks up GISBASE to decide where to
# log the message, which is a fatal error when it is not set, and on some
# platforms (observed: glibc, not musl) that kills the process silently
# rather than just losing the message. Routing messages to a handler of
# our own avoids that lookup entirely, following the same
# G_set_error_routine() pattern used in gui/wxpython/vdigit/wxdisplay.py
# and gui/wxpython/nviz/wxnviz.py, and in lib/arraystats' tests.
ERROR_ROUTINE = CFUNCTYPE(libgis.UNCHECKED(c_int), libgis.String, c_int)


@contextmanager
def swallow_grass_messages():
    @ERROR_ROUTINE
    def handler(message, is_fatal):
        return 1  # non-zero: caller (this test) has handled it

    libgis.G_set_error_routine(handler)
    try:
        yield
    finally:
        libgis.G_unset_error_routine()


def cluster_with_points(points, nbands=2):
    cluster = libcluster.Cluster()
    libcluster.I_cluster_begin(byref(cluster), nbands)
    for values in points:
        assert (
            libcluster.I_cluster_point(byref(cluster), (c_double * nbands)(*values))
            == 0
        )
    return cluster


def band_mean(points, band):
    return sum(point[band] for point in points) / len(points)


def test_exec_separates_two_well_separated_groups(libgis_without_session) -> None:
    """A checkpoint callback lets a caller (e.g. i.cluster's progress
    reporting) observe each phase of the algorithm as it runs; it is
    exercised here as well as the final result"""
    cluster = cluster_with_points(GROUP_A + GROUP_B)

    phases_seen = []
    checkpoint_type = libcluster.I_cluster_exec.argtypes[6]

    @checkpoint_type
    def checkpoint(_cluster_ptr, phase):
        phases_seen.append(phase)
        return 0

    interrupted = c_int(0)
    ret = libcluster.I_cluster_exec(
        byref(cluster), 2, 20, 98.0, 0.5, 2, checkpoint, byref(interrupted)
    )

    assert ret == 0
    assert cluster.nclasses == 2
    class_field = getattr(cluster, "class")
    classes = [class_field[p] for p in range(cluster.npoints)]
    # The two groups are far enough apart that every point in GROUP_A must
    # land in one class and every point in GROUP_B in the other, whichever
    # of the two class numbers the algorithm happens to assign to each.
    assert len(set(classes[: len(GROUP_A)])) == 1
    assert len(set(classes[len(GROUP_A) :])) == 1
    assert classes[0] != classes[-1]
    # Phase 1: initial means. Phase 2: initial assignment. Phase 3: each
    # re-assignment pass. Phase 4 (a merge) never runs here, since the two
    # groups are already distinct on the first pass.
    assert phases_seen == [1, 2, 3]

    # The signature of each class is the per-band mean of its own group.
    assert cluster.S.nsigs == 2
    signatures = sorted(
        (cluster.S.sig[c].mean[0], cluster.S.sig[c].mean[1]) for c in range(2)
    )
    assert signatures == pytest.approx(
        [
            (band_mean(GROUP_A, 0), band_mean(GROUP_A, 1)),
            (band_mean(GROUP_B, 0), band_mean(GROUP_B, 1)),
        ]
    )
    for c in range(2):
        assert cluster.S.sig[c].npoints == len(GROUP_A)
        assert cluster.S.sig[c].status == 1


def test_exec_rejects_too_few_points(libgis_without_session) -> None:
    """I_cluster_exec() checks the point count before doing anything else,
    so a null checkpoint callback is safe here: it is never reached

    This is the one case in this file that reaches I_cluster_exec()'s
    G_warning() call, so it needs swallow_grass_messages(); see that
    function's comment for why.
    """
    cluster = cluster_with_points([(1.0, 1.0)])
    interrupted = c_int(0)
    checkpoint_type = libcluster.I_cluster_exec.argtypes[6]
    no_checkpoint = checkpoint_type()  # a null function pointer

    with swallow_grass_messages():
        ret = libcluster.I_cluster_exec(
            byref(cluster), 2, 20, 98.0, 0.5, 2, no_checkpoint, byref(interrupted)
        )

    assert ret == 1
