import numpy as np

import grass.jupyter as gj
from grass.tools import Tools


def test_numpy_array_as_raster_use_region(session, tmp_path):
    xx, yy = np.meshgrid(np.linspace(0, 1, 10), np.linspace(-1, 1, 20))
    data = xx * yy

    # Set the region to match the array dimensions.
    # The region will be used later and needs to match.
    tools = Tools(session=session)
    rows = data.shape[0]
    cols = data.shape[1]
    tools.g_region(s=0, n=rows, w=0, e=cols, res=1)

    map2d = gj.Map(session=session, use_region=True)
    map2d.d_rast(map=data)
    image = tmp_path / "test.png"
    map2d.save(image)
    assert image.exists()
    assert image.stat().st_size


def test_numpy_array_as_raster_auto_region(session, tmp_path):
    xx, yy = np.meshgrid(np.linspace(0, 1, 10), np.linspace(-1, 1, 20))
    data = xx * yy

    # Set the region to match the array dimensions and resolution.
    # The auto-region mechanism will not find a raster, so it will
    # not set a region.
    tools = Tools(session=session)
    rows = data.shape[0]
    cols = data.shape[1]
    tools.g_region(s=0, n=rows, w=0, e=cols, res=1)

    map2d = gj.Map(session=session)
    map2d.d_rast(map=data)
    image = tmp_path / "test.png"
    map2d.save(image)
    assert image.exists()
    assert image.stat().st_size


def test_numpy_multiple_arrays_as_rasters(session, tmp_path):
    xx, yy = np.meshgrid(np.linspace(0, 1, 10), np.linspace(-1, 1, 20))
    data = xx * yy

    tools = Tools(session=session)
    rows = data.shape[0]
    cols = data.shape[1]
    tools.g_region(s=0, n=rows, w=0, e=cols, res=1)

    data2 = np.tri(rows, cols, -2)

    map2d = gj.Map(session=session, use_region=True)
    map2d.d_shade(color=data, shade=data2)
    image = tmp_path / "test.png"
    map2d.save(image)
    assert image.exists()
    assert image.stat().st_size
