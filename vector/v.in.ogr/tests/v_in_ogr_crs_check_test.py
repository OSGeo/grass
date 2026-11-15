# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the CRS check with compound and geographic 3D CRSs

v.external shares the CRS check code with v.in.ogr, so both are tested.
"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools, ToolError

# NAD83(2011) / North Carolina (ftUS), also with NAVD88 height (ftUS)
HORIZONTAL_CRS = "EPSG:6543"
COMPOUND_CRS = "EPSG:6543+6360"
# WGS 84 in 2D and 3D
GEOGRAPHIC_2D_CRS = "EPSG:4326"
GEOGRAPHIC_3D_CRS = "EPSG:4979"
CRSS_WITH_VERTICAL = {COMPOUND_CRS, GEOGRAPHIC_3D_CRS}
UNRELATED_CRS = "EPSG:32617"


def write_points(path, crs):
    """Write an OGR VRT with two points read from a CSV file next to it

    The coordinates are valid in both projected and geographic CRSs.
    """
    csv_path = path.with_suffix(".csv")
    csv_path.write_text("x,y\n-79,36\n-78.9,35.9\n")
    path.write_text(
        f"""<OGRVRTDataSource>
  <OGRVRTLayer name="points">
    <SrcDataSource relativeToVRT="1">{csv_path.name}</SrcDataSource>
    <SrcLayer>{csv_path.stem}</SrcLayer>
    <GeometryType>wkbPoint</GeometryType>
    <LayerSRS>{crs}</LayerSRS>
    <GeometryField encoding="PointFromColumns" x="x" y="y"/>
  </OGRVRTLayer>
</OGRVRTDataSource>
"""
    )
    return path


@pytest.fixture(params=["v_in_ogr", "v_external"])
def import_vector(request):
    """Function which imports (or links) a vector file into a session

    Returns the v.info of the imported vector and the messages of the import.
    """

    def run(session, path):
        tools = Tools(session=session, consistent_return_value=True)
        result = getattr(tools, request.param)(input=str(path), output="imported")
        info = tools.v_info(map="imported", flags="t", format="json")
        return info, result.stderr

    return run


@pytest.mark.parametrize(
    ("project_crs", "file_crs"),
    [
        (HORIZONTAL_CRS, COMPOUND_CRS),
        (COMPOUND_CRS, COMPOUND_CRS),
        (COMPOUND_CRS, HORIZONTAL_CRS),
        (GEOGRAPHIC_2D_CRS, GEOGRAPHIC_3D_CRS),
        (GEOGRAPHIC_3D_CRS, GEOGRAPHIC_3D_CRS),
        (GEOGRAPHIC_3D_CRS, GEOGRAPHIC_2D_CRS),
    ],
)
def test_same_horizontal_crs_matches(tmp_path, project_crs, file_crs, import_vector):
    """A file imports when its horizontal CRS matches the project

    The user is told when the vertical component of the file CRS is ignored.
    """
    file = write_points(tmp_path / "file.vrt", file_crs)
    project = tmp_path / "project"
    gs.create_project(project, filename=write_points(tmp_path / "crs.vrt", project_crs))
    with gs.setup.init(project, env=os.environ.copy()) as session:
        info, messages = import_vector(session, file)
    assert info["points"] == 2
    assert ("vertical" in messages.lower()) == (file_crs in CRSS_WITH_VERTICAL)


def test_different_horizontal_crs_is_rejected(tmp_path, import_vector):
    """Ignoring the vertical CRS does not make different horizontal CRSs match"""
    unrelated_file = write_points(tmp_path / "unrelated.vrt", UNRELATED_CRS)
    project = tmp_path / "project"
    gs.create_project(
        project, filename=write_points(tmp_path / "crs.vrt", COMPOUND_CRS)
    )
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        pytest.raises(ToolError, match="does not appear to match"),
    ):
        import_vector(session, unrelated_file)
