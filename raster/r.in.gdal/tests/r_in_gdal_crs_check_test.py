# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the CRS check with compound and geographic 3D CRSs

r.external shares the CRS check code with r.in.gdal, so both are tested.
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


def write_raster(path, crs):
    """Write a 10x10 VRT raster with the given CRS (GDAL fills it with zeros)

    The extent is valid in both projected and geographic CRSs.
    """
    path.write_text(
        f"""<VRTDataset rasterXSize="10" rasterYSize="10">
  <SRS>{crs}</SRS>
  <GeoTransform>-79, 0.01, 0, 36, 0, -0.01</GeoTransform>
  <VRTRasterBand dataType="Float32" band="1"/>
</VRTDataset>
"""
    )
    return path


@pytest.fixture(params=["r_in_gdal", "r_external"])
def import_raster(request):
    """Function which imports (or links) a raster file into a session

    Returns the r.info of the imported raster and the messages of the import.
    """

    def run(session, path):
        tools = Tools(session=session, consistent_return_value=True)
        result = getattr(tools, request.param)(input=str(path), output="imported")
        return tools.r_info(map="imported", format="json"), result.stderr

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
def test_same_horizontal_crs_matches(tmp_path, project_crs, file_crs, import_raster):
    """A file imports when its horizontal CRS matches the project

    The user is told when the vertical component of the file CRS is ignored.
    """
    file = write_raster(tmp_path / "file.vrt", file_crs)
    project = tmp_path / "project"
    gs.create_project(project, filename=write_raster(tmp_path / "crs.vrt", project_crs))
    with gs.setup.init(project, env=os.environ.copy()) as session:
        info, messages = import_raster(session, file)
    assert info["rows"] == 10
    assert ("vertical" in messages.lower()) == (file_crs in CRSS_WITH_VERTICAL)


def test_different_horizontal_crs_is_rejected(tmp_path, import_raster):
    """Ignoring the vertical CRS does not make different horizontal CRSs match"""
    unrelated_file = write_raster(tmp_path / "unrelated.vrt", UNRELATED_CRS)
    project = tmp_path / "project"
    gs.create_project(
        project, filename=write_raster(tmp_path / "crs.vrt", COMPOUND_CRS)
    )
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        pytest.raises(ToolError, match="does not appear to match"),
    ):
        import_raster(session, unrelated_file)
