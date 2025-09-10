import os
from pathlib import Path

import pytest

import grass.script as gs
from grass.script import MaskManager
from grass.tools import Tools
from grass.exceptions import CalledModuleError


def test_reclass_as_mask_correct_state(tmp_path):
    """Check that r.mapcalc-generated raster works as mask with reclass"""
    gs.create_project(tmp_path / "project")
    with gs.setup.init(tmp_path / "project", env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=20, s=0, e=20, w=0, rows=20, cols=20)
        tools.r_mapcalc(expression="data = row() + col()")
        tools.r_mapcalc(expression="raster_mask = if(row() < 10, 1, null())")
        with MaskManager(env=session.env):
            tools.r_mask(raster="raster_mask")
            tools.r_univar(input="data")
            tools.r_mask(flags="r")


def test_reclass_as_mask_broken_data(tmp_path):
    """Check that reading reclass map with broken data shows descriptive error"""
    gs.create_project(tmp_path / "project")
    with gs.setup.init(tmp_path / "project", env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=20, s=0, e=20, w=0, rows=20, cols=20)
        tools.r_mapcalc(expression="data = row() + col()")
        tools.r_mapcalc(expression="raster_mask = if(row() < 10, 1, null())")
        with MaskManager(env=session.env) as mask:
            tools.r_mask(raster="raster_mask")
            cellhd_file = Path(
                tools.g_findfile(file=mask.mask_name, element="cellhd", format="json")[
                    "file"
                ]
            )
            content = cellhd_file.read_text(encoding="utf-8")
            bad_value = "bad value"
            content = content.replace("\n1\n", f"\n{bad_value}\n")
            cellhd_file.write_text(content, encoding="utf-8")
            print(content)
            with pytest.raises(CalledModuleError, match=bad_value):
                tools.r_univar(map="data")
