"""Tests of r.in.pdal COPC support and the point_table_capacity option"""

import pytest

from grass.exceptions import CalledModuleError
from grass.tools import Tools


def test_copc_import(session, point_cloud_files):
    """SRS of a COPC file is detected and all points are binned"""
    tools = Tools(session=session)
    tools.g_region(n=18, s=0, e=18, w=0, res=6)
    tools.r_in_pdal(input=point_cloud_files["copc"], output="copc_n", method="n")
    stats = tools.r_univar(map="copc_n", flags="g").keyval
    assert stats["n"] == 9
    assert stats["min"] == 36
    assert stats["max"] == 36


def test_copc_mean_z(session, point_cloud_files):
    """Z values pass through COPC binning"""
    tools = Tools(session=session)
    tools.g_region(n=18, s=0, e=18, w=0, res=6)
    tools.r_in_pdal(input=point_cloud_files["copc"], output="copc_mean", method="mean")
    stats = tools.r_univar(map="copc_mean", flags="g").keyval
    assert stats["min"] == pytest.approx(6)
    assert stats["max"] == pytest.approx(30)
    assert stats["sum"] == pytest.approx(162)


def test_copc_subregion_matches_las(session, point_cloud_files):
    """Import into a region smaller than the point cloud extent.

    The COPC reader prunes by the region bounds, so the result must be
    identical to importing the same points from a plain LAS file.
    """
    tools = Tools(session=session)
    tools.g_region(n=12, s=0, e=12, w=0, res=6)
    tools.r_in_pdal(input=point_cloud_files["copc"], output="copc_sub", method="n")
    tools.r_in_pdal(input=point_cloud_files["las"], output="las_sub", method="n")
    tools.r_mapcalc(expression="sub_diff = abs(copc_sub - las_sub)")
    stats = tools.r_univar(map="sub_diff", flags="g").keyval
    assert stats["n"] == 4
    assert stats["max"] == 0
    stats = tools.r_univar(map="copc_sub", flags="g").keyval
    assert stats["sum"] == 144


def test_copc_full_extent(session, point_cloud_files):
    """The -e flag imports every point of a COPC file.

    The region is the extent of the data, so the reader is not given
    bounds to prune by and nothing may be dropped at the edges.
    """
    tools = Tools(session=session)
    tools.r_in_pdal(
        input=point_cloud_files["copc"],
        output="copc_full",
        method="n",
        resolution=6,
        flags="e",
    )
    tools.g_region(raster="copc_full")
    stats = tools.r_univar(map="copc_full", flags="g").keyval
    assert stats["sum"] == 324


def test_point_table_capacity(session, point_cloud_files):
    """Point table capacity does not change the result"""
    tools = Tools(session=session)
    tools.g_region(n=18, s=0, e=18, w=0, res=6)
    tools.r_in_pdal(input=point_cloud_files["las"], output="cap_default", method="n")
    tools.r_in_pdal(
        input=point_cloud_files["las"],
        output="cap_one",
        method="n",
        point_table_capacity=1,
    )
    tools.r_mapcalc(expression="cap_diff = abs(cap_default - cap_one)")
    stats = tools.r_univar(map="cap_diff", flags="g").keyval
    assert stats["max"] == 0


def test_point_table_capacity_rejects_zero(session, point_cloud_files):
    """Zero capacity is rejected by the parser"""
    tools = Tools(session=session)
    tools.g_region(n=18, s=0, e=18, w=0, res=6)
    with pytest.raises(CalledModuleError):
        tools.r_in_pdal(
            input=point_cloud_files["las"],
            output="cap_zero",
            method="n",
            point_table_capacity=0,
        )
