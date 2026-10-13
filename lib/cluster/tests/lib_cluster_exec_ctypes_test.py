"""Tests for the full clustering workflow ctypes binding

I_cluster_exec() drives the whole algorithm end to end: initial means,
assignment, iterative re-assignment, merging classes that are not
distinct enough, and finally building the output signatures. It calls
G_debug() internally, so it needs the same libgis_without_session
fixture as lib_cluster_algorithm_ctypes_test.py; see conftest.py for why.

Before any of that, the function quietly replaces out-of-range arguments
with defaults of its own, and most of the tests below pin those down.
None of the corrected values can be read back off the struct, so each is
checked by the effect it has on the result, alongside a contrasting
in-range value that produces a different one.

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

# One tight group plus a single distant point. The outlier ends up alone
# in a class of its own, which is what the final minimum-size pass then
# has to decide about.
GROUP_WITH_OUTLIER = [
    (0.0, 0.0),
    (1.0, 1.0),
    (2.0, 2.0),
    (3.0, 3.0),
    (4.0, 4.0),
    (100.0, 100.0),
]

# A continuum rather than distinct groups. Where the class boundaries
# fall is arbitrary enough that the first re-assignment pass still moves
# a point, leaving the run short of a 98% stable result on that pass.
EVENLY_SPACED = [(float(i), float(i) * 2.0) for i in range(12)]

# Eight of the nine points of a 3x3 grid. Classes carved out of it
# overlap heavily, so the separation between them comes to about 0.35:
# below the 0.5 default, but above a smaller threshold passed explicitly.
UNIT_GRID = [
    (0.0, 0.0),
    (1.0, 0.0),
    (2.0, 0.0),
    (0.0, 1.0),
    (1.0, 1.0),
    (2.0, 1.0),
    (0.0, 2.0),
    (1.0, 2.0),
]

# I_cluster_exec() reports progress through this callback, which is also
# the only way to reach into the middle of a run. Phase 1 is the initial
# means, phase 2 the initial assignment, phase 3 each re-assignment pass,
# and phase 4 a merge of two classes that were not distinct enough.
CHECKPOINT = libcluster.I_cluster_exec.argtypes[6]

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


def exec_with(
    points,
    maxclass=2,
    iterations=20,
    convergence=98.0,
    separation=0.5,
    min_class_size=2,
    interrupt_at_phase=None,
):
    """Cluster points with I_cluster_exec(), recording the phases it reports

    Every parameter defaults to an in-range value, so a test only has to
    name the one it is about. Passing interrupt_at_phase raises the
    interrupt flag from inside the checkpoint callback, which is the only
    point at which a caller could realistically raise it mid-run.

    Returns the return value, the Cluster struct and the phases seen.
    """
    cluster = cluster_with_points(points)
    phases = []
    interrupted = c_int(0)

    @CHECKPOINT
    def checkpoint(_cluster, phase):
        phases.append(phase)
        if phase == interrupt_at_phase:
            interrupted.value = 1
        return 0

    ret = libcluster.I_cluster_exec(
        byref(cluster),
        maxclass,
        iterations,
        convergence,
        separation,
        min_class_size,
        checkpoint,
        byref(interrupted),
    )
    return ret, cluster, phases


def classes_of(cluster):
    """Each point's class, with -1 for points whose class was eliminated"""
    class_field = getattr(cluster, "class")
    return [class_field[p] for p in range(cluster.npoints)]


def counts_of(cluster):
    return [cluster.count[c] for c in range(cluster.nclasses)]


def band_mean(points, band):
    return sum(point[band] for point in points) / len(points)


def test_exec_separates_two_well_separated_groups(libgis_without_session) -> None:
    """The checkpoint callback lets a caller (e.g. i.cluster's progress
    reporting) observe each phase as it runs; it is exercised here as
    well as the final result"""
    ret, cluster, phases = exec_with(GROUP_A + GROUP_B)

    assert ret == 0
    assert cluster.nclasses == 2
    classes = classes_of(cluster)
    # The two groups are far enough apart that every point in GROUP_A must
    # land in one class and every point in GROUP_B in the other, whichever
    # of the two class numbers the algorithm happens to assign to each.
    assert len(set(classes[: len(GROUP_A)])) == 1
    assert len(set(classes[len(GROUP_A) :])) == 1
    assert classes[0] != classes[-1]
    # No phase 4, since the two groups are already distinct on the first
    # pass and so nothing is ever merged.
    assert phases == [1, 2, 3]

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
    no_checkpoint = CHECKPOINT()  # a null function pointer

    with swallow_grass_messages():
        ret = libcluster.I_cluster_exec(
            byref(cluster), 2, 20, 98.0, 0.5, 2, no_checkpoint, byref(interrupted)
        )

    assert ret == 1


def test_exec_turns_a_negative_maxclass_into_a_single_class(
    libgis_without_session,
) -> None:
    """A negative class count becomes 1 rather than being rejected, so
    the two groups come back merged into one class"""
    ret, cluster, _ = exec_with(GROUP_A + GROUP_B, maxclass=-1)

    assert ret == 0
    assert cluster.nclasses == 1
    assert classes_of(cluster) == [0] * len(GROUP_A + GROUP_B)
    assert counts_of(cluster) == [len(GROUP_A + GROUP_B)]
    assert cluster.S.nsigs == 1


def test_exec_raises_a_non_positive_min_class_size_to_seventeen(
    libgis_without_session,
) -> None:
    """A minimum class size of 0 is replaced by 17, not treated as "no
    minimum"

    Both groups here hold five points, so the substituted minimum wipes
    out every class and leaves no signatures at all, while a minimum of 5
    keeps both. That difference is the only way to see which value was
    actually used, since the corrected one is never stored.
    """
    ret, cluster, _ = exec_with(GROUP_A + GROUP_B, min_class_size=0)

    assert ret == 0
    assert cluster.nclasses == 0
    assert classes_of(cluster) == [-1] * len(GROUP_A + GROUP_B)
    assert cluster.S.nsigs == 0

    _, kept, _ = exec_with(GROUP_A + GROUP_B, min_class_size=5)
    assert kept.nclasses == 2
    assert counts_of(kept) == [5, 5]


def test_exec_raises_a_min_class_size_of_one_to_two(libgis_without_session) -> None:
    """A minimum class size of 1 is raised to 2, so a class holding a
    single point is still discarded

    Taken literally, a minimum of 1 would keep the outlier's one-point
    class; it is dropped instead, which is what shows the value was
    raised.
    """
    ret, cluster, _ = exec_with(GROUP_WITH_OUTLIER, min_class_size=1)

    assert ret == 0
    assert cluster.nclasses == 1
    assert counts_of(cluster) == [5]
    assert classes_of(cluster) == [0, 0, 0, 0, 0, -1]


def test_exec_replaces_non_positive_iterations_with_twenty(
    libgis_without_session,
) -> None:
    """Asking for no iterations runs twenty of them rather than none

    The convergence threshold is set beyond 100% so that it can never be
    met, which leaves the iteration limit as the only thing that can end
    the loop and makes the substituted value countable.
    """
    ret, cluster, phases = exec_with(GROUP_A + GROUP_B, iterations=0, convergence=200.0)

    assert ret == 0
    assert phases.count(3) == 20
    assert cluster.iteration == 20

    _, capped, capped_phases = exec_with(
        GROUP_A + GROUP_B, iterations=3, convergence=200.0
    )
    assert capped_phases.count(3) == 3
    assert capped.iteration == 3


def test_exec_replaces_a_non_positive_convergence_with_ninety_eight(
    libgis_without_session,
) -> None:
    """A convergence of 0 becomes 98%, so the run keeps iterating rather
    than stopping as soon as any point at all is stable

    The first re-assignment pass over this data leaves it short of 98%
    stable, so the substituted threshold forces a second pass, while a
    threshold the first pass already clears stops after one.
    """
    ret, cluster, phases = exec_with(EVENLY_SPACED, maxclass=4, convergence=0.0)

    assert ret == 0
    assert phases.count(3) == 2
    assert cluster.iteration == 2

    _, explicit, explicit_phases = exec_with(
        EVENLY_SPACED, maxclass=4, convergence=98.0
    )
    assert explicit_phases.count(3) == 2
    assert explicit.iteration == 2

    _, lenient, lenient_phases = exec_with(EVENLY_SPACED, maxclass=4, convergence=50.0)
    assert lenient_phases.count(3) == 1
    assert lenient.iteration == 1


def test_exec_replaces_a_negative_separation_with_half(
    libgis_without_session,
) -> None:
    """A negative separation becomes 0.5, which is enough to merge the
    two classes this grid is split into

    The merge is also the only case in this file that reaches phase 4.
    The two classes are about 0.35 apart, so the substituted 0.5 merges
    them exactly as an explicit 0.5 does, while a smaller threshold
    passed on purpose leaves them alone.
    """
    ret, cluster, phases = exec_with(UNIT_GRID, separation=-1.0)

    assert ret == 0
    assert phases == [1, 2, 3, 4, 3]
    assert cluster.nclasses == 1

    _, explicit, explicit_phases = exec_with(UNIT_GRID, separation=0.5)
    assert explicit_phases == [1, 2, 3, 4, 3]
    assert explicit.nclasses == 1

    _, distinct, distinct_phases = exec_with(UNIT_GRID, separation=0.1)
    assert distinct_phases == [1, 2, 3]
    assert distinct.nclasses == 2


def test_exec_reports_an_interrupt_during_the_initial_assignment(
    libgis_without_session,
) -> None:
    """Raising the flag from the phase 1 checkpoint stops the run inside
    I_cluster_assign(), before any point has been assigned"""
    ret, _, phases = exec_with(GROUP_A + GROUP_B, interrupt_at_phase=1)

    assert ret == -2
    assert phases == [1]


def test_exec_reports_an_interrupt_before_the_reassignment_loop(
    libgis_without_session,
) -> None:
    """Raising the flag from the phase 2 checkpoint stops the run at the
    top of the re-assignment loop, which is a separate check from the one
    the initial assignment goes through"""
    ret, _, phases = exec_with(GROUP_A + GROUP_B, interrupt_at_phase=2)

    assert ret == -2
    assert phases == [1, 2]
