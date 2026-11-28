# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the vertex snapping of v.in.ogr

v.in.ogr snaps the vertices of the imported boundaries to each other when the
snap option is used. The vertices which are close to each other are found with
a k-d tree, so these tests check that the imported geometry is the expected
one.
"""

import os
import re

import pytest

import grass.script as gs

# the CRS of the test data, the project is created from the data file so that
# the CRS of the data and the CRS of the project are the same
CRS = "EPSG:32633"

# two squares which share an edge
CLEAN_POLYGONS = [
    "POLYGON ((0 0, 0 10, 10 10, 10 0, 0 0))",
    "POLYGON ((0 0, 10 0, 10 -10, 0 -10, 0 0))",
]

# the same two squares, but the shared edge of the second square is shifted
DIRTY_POLYGONS = [
    "POLYGON ((0 0, 0 10, 10 10, 10 0, 0 0))",
    "POLYGON ((0.5 0.5, 10.5 0.5, 10.5 -10, 0.5 -10, 0.5 0.5))",
]


def write_dataset(tmp_path, name, polygons):
    """Write the polygons to an OGR dataset and return the VRT file

    The dataset is a CSV file with the geometries in WKT, referenced by a VRT
    file which sets the geometry type and the CRS of the layer.
    """
    csv_path = tmp_path / f"{name}.csv"
    csv_path.write_text("WKT\n" + "".join(f'"{polygon}"\n' for polygon in polygons))
    vrt_path = tmp_path / f"{name}.vrt"
    vrt_path.write_text(
        f"""<OGRVRTDataSource>
  <OGRVRTLayer name="{name}">
    <SrcDataSource relativeToVRT="1">{csv_path.name}</SrcDataSource>
    <SrcLayer>{name}</SrcLayer>
    <GeometryType>wkbPolygon</GeometryType>
    <LayerSRS>{CRS}</LayerSRS>
    <GeometryField encoding="WKT" field="WKT"/>
  </OGRVRTLayer>
</OGRVRTDataSource>
"""
    )
    return vrt_path


def import_dataset(session, vrt_path, snap):
    """Import the dataset with snapping"""
    gs.run_command(
        "v.in.ogr",
        input=str(vrt_path),
        output="imported",
        snap=snap,
        env=session.env,
    )


def read_areas(session):
    """Read the areas of the imported map as a list of sets of vertices"""
    wkt = gs.read_command(
        "v.out.ascii", input="imported", format="wkt", env=session.env
    )
    areas = []
    for line in wkt.splitlines():
        if not line.startswith("POLYGON"):
            continue
        numbers = [float(number) for number in re.findall(r"[-+0-9.eE]+", line)]
        vertices = zip(numbers[0::2], numbers[1::2], strict=False)
        areas.append({(round(x, 6), round(y, 6)) for x, y in vertices})
    return areas


@pytest.fixture
def session(tmp_path):
    """A session in a project with the CRS of the test data"""
    vrt_path = write_dataset(tmp_path, "polygons", CLEAN_POLYGONS)
    project = tmp_path / "project"
    gs.create_project(project, filename=vrt_path)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session


def test_vertices_are_not_moved_when_they_are_far_apart(session, tmp_path):
    """Vertices which are further apart than the threshold are not moved"""
    vrt_path = write_dataset(tmp_path, "clean", CLEAN_POLYGONS)
    import_dataset(session, vrt_path, snap=0.001)

    areas = read_areas(session)
    assert len(areas) == 2
    expected = [
        {(0.0, 0.0), (0.0, 10.0), (10.0, 10.0), (10.0, 0.0)},
        {(0.0, 0.0), (10.0, 0.0), (10.0, -10.0), (0.0, -10.0)},
    ]
    assert areas == expected


def test_vertices_within_threshold_are_snapped(session, tmp_path):
    """Vertices which are closer to each other than the threshold are snapped
    to the same position"""
    vrt_path = write_dataset(tmp_path, "dirty", DIRTY_POLYGONS)
    import_dataset(session, vrt_path, snap=1.0)

    areas = read_areas(session)
    assert len(areas) == 2
    # the first square is not changed
    assert areas[0] == {(0.0, 0.0), (0.0, 10.0), (10.0, 10.0), (10.0, 0.0)}
    # the shared edge of the second square is snapped to the edge of the
    # first square, the other vertices are not moved
    assert areas[1] == {(0.0, 0.0), (10.0, 0.0), (10.5, -10.0), (0.5, -10.0)}


def test_vertices_are_not_snapped_without_the_snap_option(session, tmp_path):
    """Without the snap option the vertices are not moved"""
    vrt_path = write_dataset(tmp_path, "clean", CLEAN_POLYGONS)
    import_dataset(session, vrt_path, snap=-1)

    areas = read_areas(session)
    assert len(areas) == 2
    assert areas[0] == {(0.0, 0.0), (0.0, 10.0), (10.0, 10.0), (10.0, 0.0)}
    assert areas[1] == {(0.0, 0.0), (10.0, 0.0), (10.0, -10.0), (0.0, -10.0)}
