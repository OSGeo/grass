"""Test grass.jupyter.Profile"""

import pytest

from grass.tools import Tools

import grass.jupyter as gj

LINE = [(0.5, 0.5), (1.5, 0.5), (1.5, 4.5)]


def test_vertices(session_with_data):
    """Every coordinate pair becomes a vertex with a cumulative distance"""
    profile = gj.Profile("data", LINE, env=session_with_data.env)
    assert len(profile.vertices) == len(LINE)
    assert [vertex["distance"] for vertex in profile.vertices] == pytest.approx(
        [0, 1, 5]
    )
    assert (profile.vertices[-1]["easting"], profile.vertices[-1]["northing"]) == (
        1.5,
        4.5,
    )


def test_data_rows(session_with_data):
    """Samples run from the first to the last vertex with increasing distance"""
    profile = gj.Profile("data", LINE, env=session_with_data.env)
    rows = profile.data["data"]
    assert set(rows[0].keys()) == {"easting", "northing", "distance", "value"}
    assert rows[0] == {"easting": 0.5, "northing": 0.5, "distance": 0, "value": 5}
    # One sample per cell along 5 units plus the last vertex.
    assert len(rows) == 6
    distances = [row["distance"] for row in rows]
    assert distances == sorted(distances)
    assert rows[-1]["distance"] == pytest.approx(profile.vertices[-1]["distance"])
    # data = row() * col(), so the last vertex in row 1, column 2 has value 2.
    assert rows[-1]["value"] == 2


def test_multiple_rasters_aligned(session_with_data):
    """Rasters are sampled at the same distances"""
    tools = Tools(session=session_with_data)
    tools.r_mapcalc(expression="data2 = 2 * data")
    profile = gj.Profile(["data", "data2"], LINE, env=session_with_data.env)
    assert profile.rasters == ["data", "data2"]
    rows = profile.data["data"]
    rows2 = profile.data["data2"]
    assert [row["distance"] for row in rows] == [row["distance"] for row in rows2]
    assert [2 * row["value"] for row in rows] == [row["value"] for row in rows2]


def test_null_values(session_with_data):
    """Null cells become None and do not break plotting"""
    pytest.importorskip("matplotlib", reason="matplotlib package not available")
    tools = Tools(session=session_with_data)
    tools.r_mapcalc(expression="nulls = if(col() == 2, null(), data)")
    profile = gj.Profile("nulls", LINE, env=session_with_data.env)
    values = [row["value"] for row in profile.data["nulls"]]
    assert values[0] == 5
    assert values[1:] == [None] * 5
    ax = profile.plot()
    assert len(ax.get_lines()) >= 1


def test_resolution(session_with_data):
    """Halving the resolution doubles the number of samples along the line"""
    profile = gj.Profile("data", LINE, env=session_with_data.env)
    fine = gj.Profile("data", LINE, env=session_with_data.env, resolution=0.5)
    # Both include the extra sample at the last vertex.
    assert len(fine.data["data"]) - 1 == 2 * (len(profile.data["data"]) - 1)


def test_invalid_input(session_with_data):
    """Too few coordinates or no raster are rejected"""
    with pytest.raises(ValueError, match="two coordinate pairs"):
        gj.Profile("data", [(0.5, 0.5)], env=session_with_data.env)
    with pytest.raises(ValueError, match="one raster"):
        gj.Profile([], LINE, env=session_with_data.env)


def test_to_dataframe(session_with_data):
    """DataFrame has one row per sample with the raster name"""
    pd = pytest.importorskip("pandas", reason="pandas package not available")
    profile = gj.Profile("data", LINE, env=session_with_data.env)
    df = profile.to_dataframe()
    assert isinstance(df, pd.DataFrame)
    assert list(df.columns) == ["raster", "easting", "northing", "distance", "value"]
    assert len(df) == len(profile.data["data"])
    assert set(df["raster"]) == {"data"}


def test_figure_and_save(session_with_data, tmp_path):
    """Figure has a map and a chart panel and can be saved"""
    pytest.importorskip("matplotlib", reason="matplotlib package not available")
    profile = gj.Profile("data", LINE, env=session_with_data.env)
    fig = profile.figure()
    assert len(fig.axes) == 2
    filename = tmp_path / "profile.png"
    profile.save(filename)
    assert filename.exists()
    assert filename.stat().st_size > 0
