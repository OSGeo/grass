# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the k-d tree implementation in lib/btree2

The k-d tree is a pure in-memory structure, so no GRASS session is needed here.
The library is loaded directly with ctypes, the structure definitions below
must match the ones in lib/btree2/kdtree.h.
"""

import ctypes
import math
import os
import shutil
import subprocess
from pathlib import Path

import numpy as np
import pytest


class KDNode(ctypes.Structure):
    """Node of the k-d tree, see struct kdnode in kdtree.h"""


KDNode._fields_ = [
    ("dim", ctypes.c_ubyte),
    ("depth", ctypes.c_ubyte),
    ("balance", ctypes.c_ubyte),
    ("c", ctypes.POINTER(ctypes.c_double)),
    ("uid", ctypes.c_int),
    ("child", ctypes.POINTER(KDNode) * 2),
]


class KDTree(ctypes.Structure):
    """k-d tree, see struct kdtree in kdtree.h"""


KDTree._fields_ = [
    ("ndims", ctypes.c_ubyte),
    ("nextdim", ctypes.POINTER(ctypes.c_ubyte)),
    ("csize", ctypes.c_int),
    ("btol", ctypes.c_int),
    ("count", ctypes.c_size_t),
    ("root", ctypes.POINTER(KDNode)),
    ("nodes", ctypes.POINTER(KDNode)),
    ("pool", ctypes.POINTER(ctypes.c_double)),
    ("nnodes", ctypes.c_size_t),
]


class KDTrav(ctypes.Structure):
    """k-d tree traversal, see struct kdtrav in kdtree.h"""


KDTrav._fields_ = [
    ("tree", ctypes.POINTER(KDTree)),
    ("curr_node", ctypes.POINTER(KDNode)),
    ("up", ctypes.POINTER(KDNode) * 256),
    ("top", ctypes.c_int),
    ("first", ctypes.c_int),
]


def library_directories():
    """Directories which may contain the GRASS libraries"""
    directories = []
    grass = shutil.which("grass")
    if grass:
        try:
            result = subprocess.run(
                [grass, "--config", "path"],
                capture_output=True,
                check=True,
                text=True,
            )
        except (OSError, subprocess.SubprocessError):
            pass
        else:
            directories.append(Path(result.stdout.strip()) / "lib")
    for entry in os.environ.get("LD_LIBRARY_PATH", "").split(os.pathsep):
        if entry:
            directories.append(Path(entry))
    return directories


def find_library(name):
    """Find a GRASS shared library, None if it is not found"""
    for directory in library_directories():
        if not directory.is_dir():
            continue
        for pattern in (f"{name}*.so", f"{name}*.dylib", f"{name}*.dll"):
            for path in sorted(directory.glob(pattern)):
                return path
    return None


# G_GISRC_MODE_MEMORY from grass/gis.h
GISRC_MODE_MEMORY = 1


class KDTreeLib:
    """Access to the k-d tree functions of lib/btree2"""

    def __init__(self, path, gis_path):
        self.lib = ctypes.CDLL(str(path))
        self.gis = ctypes.CDLL(str(gis_path)) if gis_path else None
        if self.gis:
            self.gis.G_free.argtypes = [ctypes.c_void_p]
            self.gis.G_free.restype = None
            self.gis.G_set_gisrc_mode.argtypes = [ctypes.c_int]
            self.gis.G_set_gisrc_mode.restype = None
            self.gis.G_get_gisrc_mode.argtypes = []
            self.gis.G_get_gisrc_mode.restype = ctypes.c_int

        self.lib.kdtree_create.argtypes = [
            ctypes.c_char,
            ctypes.POINTER(ctypes.c_int),
        ]
        self.lib.kdtree_create.restype = ctypes.POINTER(KDTree)

        self.lib.kdtree_create_from_points.argtypes = [
            ctypes.c_char,
            ctypes.POINTER(ctypes.c_double),
            ctypes.POINTER(ctypes.c_int),
            ctypes.c_int,
            ctypes.c_int,
        ]
        self.lib.kdtree_create_from_points.restype = ctypes.POINTER(KDTree)

        self.lib.kdtree_destroy.argtypes = [ctypes.POINTER(KDTree)]
        self.lib.kdtree_destroy.restype = None

        self.lib.kdtree_insert.argtypes = [
            ctypes.POINTER(KDTree),
            ctypes.POINTER(ctypes.c_double),
            ctypes.c_int,
            ctypes.c_int,
        ]
        self.lib.kdtree_insert.restype = ctypes.c_int

        self.lib.kdtree_remove.argtypes = [
            ctypes.POINTER(KDTree),
            ctypes.POINTER(ctypes.c_double),
            ctypes.c_int,
        ]
        self.lib.kdtree_remove.restype = ctypes.c_int

        self.lib.kdtree_knn.argtypes = [
            ctypes.POINTER(KDTree),
            ctypes.POINTER(ctypes.c_double),
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_double),
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int),
        ]
        self.lib.kdtree_knn.restype = ctypes.c_int

        self.lib.kdtree_dnn.argtypes = [
            ctypes.POINTER(KDTree),
            ctypes.POINTER(ctypes.c_double),
            ctypes.POINTER(ctypes.POINTER(ctypes.c_int)),
            ctypes.POINTER(ctypes.POINTER(ctypes.c_double)),
            ctypes.c_double,
            ctypes.POINTER(ctypes.c_int),
        ]
        self.lib.kdtree_dnn.restype = ctypes.c_int

        self.lib.kdtree_init_trav.argtypes = [
            ctypes.POINTER(KDTrav),
            ctypes.POINTER(KDTree),
        ]
        self.lib.kdtree_init_trav.restype = ctypes.c_int

        self.lib.kdtree_traverse.argtypes = [
            ctypes.POINTER(KDTrav),
            ctypes.POINTER(ctypes.c_double),
            ctypes.POINTER(ctypes.c_int),
        ]
        self.lib.kdtree_traverse.restype = ctypes.c_int

    def create(self, ndims):
        """Create an empty tree"""
        return self.lib.kdtree_create(ndims, None)

    def create_from_points(self, ndims, points, uids, dc=0):
        """Create a tree from an array of points

        Returns the tree and the array with the uids of the points which are
        in the tree.
        """
        count = len(uids)
        coords = (ctypes.c_double * (count * ndims))(*np.asarray(points).ravel())
        ids = (ctypes.c_int * count)(*uids)
        tree = self.lib.kdtree_create_from_points(ndims, coords, ids, count, dc)
        return tree, ids

    def destroy(self, tree):
        """Destroy a tree"""
        self.lib.kdtree_destroy(tree)

    def insert(self, tree, point, uid, dc=0):
        """Insert a point, returns True if the point was added"""
        coords = (ctypes.c_double * len(point))(*point)
        return bool(self.lib.kdtree_insert(tree, coords, uid, dc))

    def remove(self, tree, point, uid):
        """Remove a point, returns True if the point was removed"""
        coords = (ctypes.c_double * len(point))(*point)
        return bool(self.lib.kdtree_remove(tree, coords, uid))

    def knn(self, tree, point, k, skip=None):
        """k nearest neighbors, returns the uids and the squared distances"""
        coords = (ctypes.c_double * len(point))(*point)
        uids = (ctypes.c_int * k)()
        dists = (ctypes.c_double * k)()
        skipped = ctypes.byref(ctypes.c_int(skip)) if skip is not None else None
        found = self.lib.kdtree_knn(tree, coords, uids, dists, k, skipped)
        return list(uids[:found]), list(dists[:found])

    def dnn(self, tree, point, maxdist, skip=None):
        """All neighbors within maxdist, returns the uids and the distances"""
        coords = (ctypes.c_double * len(point))(*point)
        uids = ctypes.POINTER(ctypes.c_int)()
        dists = ctypes.POINTER(ctypes.c_double)()
        skipped = ctypes.byref(ctypes.c_int(skip)) if skip is not None else None
        found = self.lib.kdtree_dnn(
            tree,
            coords,
            ctypes.byref(uids),
            ctypes.byref(dists),
            maxdist,
            skipped,
        )
        result_uids = [uids[i] for i in range(found)]
        result_dists = [dists[i] for i in range(found)]
        if self.gis:
            self.gis.G_free(uids)
            self.gis.G_free(dists)
        return result_uids, result_dists

    def traverse(self, tree):
        """All points of the tree as a list of (uid, coordinates)"""
        trav = KDTrav()
        self.lib.kdtree_init_trav(ctypes.byref(trav), tree)
        coords = (ctypes.c_double * tree.contents.ndims)()
        uid = ctypes.c_int()
        points = []
        while self.lib.kdtree_traverse(ctypes.byref(trav), coords, ctypes.byref(uid)):
            points.append((uid.value, [coords[i] for i in range(len(coords))]))
        return points

    def depth(self, tree):
        """Depth of the tree"""
        return tree.contents.root.contents.depth


@pytest.fixture(scope="module")
def kdtree():
    """The k-d tree library"""
    path = find_library("libgrass_btree2")
    if path is None:
        pytest.skip("libgrass_btree2 not found")
    return KDTreeLib(path, find_library("libgrass_gis"))


@pytest.fixture(scope="module", autouse=True)
def gisrc_memory_mode(kdtree):
    """Let the k-d tree functions run without a GRASS session

    Balancing a tree calls G_debug(), which reads the DEBUG variable out of
    the session's GISRC file. With no session there is no file to read, so the
    lookup ends in G_fatal_error() and exits the process, taking the pytest
    run down with it. The k-d tree never touches a project or a mapset, so
    instead of setting up a session for its own sake, GISRC is switched to
    memory mode: libgis then keeps those variables in memory and never looks
    for the file.

    The mode is process-global in the C library, so it is restored here.
    """
    if kdtree.gis is None:
        yield
        return
    previous_mode = kdtree.gis.G_get_gisrc_mode()
    kdtree.gis.G_set_gisrc_mode(GISRC_MODE_MEMORY)
    try:
        yield
    finally:
        kdtree.gis.G_set_gisrc_mode(previous_mode)


def random_points(count, ndims=2, seed=42):
    """Random points with distinct coordinates"""
    rng = np.random.default_rng(seed)
    return rng.random((count, ndims)) * 1000.0


def squared_distances(points, query):
    """Squared distances of all points to the query point"""
    return ((np.asarray(points) - np.asarray(query)) ** 2).sum(axis=1)


def sorted_by_distance(uids, dists):
    """The uids and the distances, sorted by the distance"""
    pairs = sorted(zip(dists, uids, strict=True))

    return [uid for _, uid in pairs], [dist for dist, _ in pairs]


def test_bulk_tree_has_the_same_points_as_the_dynamic_tree(kdtree):
    """The tree built from an array of points contains the same points as the
    tree built by inserting the points one by one"""
    points = random_points(500, seed=1)
    uids = list(range(1000, 1500))

    dynamic = kdtree.create(2)
    bulk, bulk_uids = kdtree.create_from_points(2, points, uids)
    try:
        for uid, point in zip(uids, points, strict=True):
            assert kdtree.insert(dynamic, point, uid)

        assert bulk.contents.count == dynamic.contents.count
        assert list(bulk_uids[: bulk.contents.count]) == uids
        assert sorted(kdtree.traverse(bulk)) == sorted(kdtree.traverse(dynamic))
    finally:
        kdtree.destroy(dynamic)
        kdtree.destroy(bulk)


def test_bulk_tree_gives_the_same_neighbors_as_the_dynamic_tree(kdtree):
    """The tree built from an array of points gives the same nearest
    neighbors as the tree built by inserting the points one by one"""
    points = random_points(500, seed=2)
    uids = list(range(500))

    dynamic = kdtree.create(2)
    bulk, _ = kdtree.create_from_points(2, points, uids)
    try:
        for uid, point in zip(uids, points, strict=True):
            kdtree.insert(dynamic, point, uid)

        for query in points[::37]:
            bulk_uids, bulk_dists = sorted_by_distance(*kdtree.knn(bulk, query, 5))
            dynamic_uids, dynamic_dists = sorted_by_distance(
                *kdtree.knn(dynamic, query, 5)
            )
            # the distances can differ in the last bit because the tree
            # search computes them in a different order depending on the
            # shape of the tree
            assert bulk_uids == dynamic_uids
            assert bulk_dists == pytest.approx(dynamic_dists)

            bulk_uids, bulk_dists = sorted_by_distance(*kdtree.dnn(bulk, query, 50.0))
            dynamic_uids, dynamic_dists = sorted_by_distance(
                *kdtree.dnn(dynamic, query, 50.0)
            )
            assert bulk_uids == dynamic_uids
            assert bulk_dists == pytest.approx(dynamic_dists)
    finally:
        kdtree.destroy(dynamic)
        kdtree.destroy(bulk)


def test_bulk_tree_neighbors_match_the_brute_force(kdtree):
    """The nearest neighbors of the tree built from an array of points are the
    same as the ones found by checking all points"""
    points = random_points(200, seed=3)
    uids = list(range(200))

    bulk, _ = kdtree.create_from_points(2, points, uids)
    try:
        for query in points[::23]:
            dists = squared_distances(points, query)
            order = np.argsort(dists, kind="stable")

            found_uids, found_dists = kdtree.knn(bulk, query, 5)
            assert found_dists == pytest.approx(dists[order[:5]])
            assert found_uids == [uids[i] for i in order[:5]]

            radius = 30.0
            neighbors = [i for i in order if dists[i] <= radius * radius]
            found_uids, found_dists = kdtree.dnn(bulk, query, radius)
            assert found_dists == pytest.approx(dists[neighbors])
            assert found_uids == [uids[i] for i in neighbors]
    finally:
        kdtree.destroy(bulk)


def test_bulk_tree_is_balanced(kdtree):
    """The tree built from an array of points is balanced"""
    for count in (1, 2, 3, 4, 5, 6, 7, 8, 100, 1000):
        points = random_points(count, seed=count)
        uids = list(range(count))
        bulk, _ = kdtree.create_from_points(2, points, uids)
        try:
            assert bulk.contents.count == count
            assert kdtree.depth(bulk) == math.floor(math.log2(count))
        finally:
            kdtree.destroy(bulk)


def test_bulk_tree_removes_duplicate_coordinates_like_inserting(kdtree):
    """Points with identical coordinates are only added once, the first one wins"""
    points = np.array(
        [
            [0.0, 0.0],
            [1.0, 1.0],
            [0.0, 0.0],
            [2.0, 2.0],
            [1.0, 1.0],
        ]
    )
    uids = [10, 11, 12, 13, 14]

    dynamic = kdtree.create(2)
    bulk, bulk_uids = kdtree.create_from_points(2, points, uids, dc=0)
    try:
        inserted = [
            kdtree.insert(dynamic, point, uid, dc=0)
            for point, uid in zip(points, uids, strict=True)
        ]
        assert inserted == [True, True, False, True, False]
        assert bulk.contents.count == dynamic.contents.count == 3
        assert list(bulk_uids[: bulk.contents.count]) == [10, 11, 13]
    finally:
        kdtree.destroy(dynamic)
        kdtree.destroy(bulk)


def test_bulk_tree_keeps_duplicate_coordinates_with_different_uid(kdtree):
    """Points with identical coordinates but a different uid are all added"""
    points = np.array(
        [
            [0.0, 0.0],
            [1.0, 1.0],
            [0.0, 0.0],
            [1.0, 1.0],
        ]
    )
    uids = [10, 11, 12, 13]

    dynamic = kdtree.create(2)
    bulk, bulk_uids = kdtree.create_from_points(2, points, uids, dc=1)
    try:
        inserted = [
            kdtree.insert(dynamic, point, uid, dc=1)
            for point, uid in zip(points, uids, strict=True)
        ]
        assert inserted == [True, True, True, True]
        assert bulk.contents.count == dynamic.contents.count == 4
        assert list(bulk_uids[: bulk.contents.count]) == uids
    finally:
        kdtree.destroy(dynamic)
        kdtree.destroy(bulk)


def test_bulk_tree_removes_duplicate_points_with_the_same_uid(kdtree):
    """Points with identical coordinates and an identical uid are only added
    once"""
    points = np.array([[0.0, 0.0], [1.0, 1.0], [0.0, 0.0]])
    uids = [10, 11, 10]

    dynamic = kdtree.create(2)
    bulk, bulk_uids = kdtree.create_from_points(2, points, uids, dc=1)
    try:
        inserted = [
            kdtree.insert(dynamic, point, uid, dc=1)
            for point, uid in zip(points, uids, strict=True)
        ]
        assert inserted == [True, True, False]
        assert bulk.contents.count == dynamic.contents.count == 2
        assert list(bulk_uids[: bulk.contents.count]) == [10, 11]
    finally:
        kdtree.destroy(dynamic)
        kdtree.destroy(bulk)


def test_remove_point_from_bulk_tree(kdtree):
    """A removed point is not in the tree anymore, the other points are"""
    points = random_points(100, seed=4)
    uids = list(range(100))

    bulk, _ = kdtree.create_from_points(2, points, uids)
    try:
        removed = 42
        assert kdtree.remove(bulk, points[removed], removed)
        assert bulk.contents.count == 99
        assert {uid for uid, _ in kdtree.traverse(bulk)} == set(uids) - {removed}

        found_uids, _ = kdtree.dnn(bulk, points[removed], 0.0)
        assert removed not in found_uids
    finally:
        kdtree.destroy(bulk)


def test_insert_point_into_bulk_tree(kdtree):
    """A point can be inserted into a tree which was built from an array of
    points"""
    points = random_points(100, seed=5)
    uids = list(range(100))

    bulk, _ = kdtree.create_from_points(2, points, uids)
    try:
        new_point = np.array([500.0, 500.0])
        assert kdtree.insert(bulk, new_point, 100)
        assert bulk.contents.count == 101
        assert {uid for uid, _ in kdtree.traverse(bulk)} == set(uids) | {100}

        # the new point is found by the searches
        found_uids, found_dists = kdtree.knn(bulk, new_point, 1)
        assert found_uids == [100]
        assert found_dists == pytest.approx([0.0])
    finally:
        kdtree.destroy(bulk)
