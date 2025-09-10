from pathlib import Path

import pytest

from grass.script import MaskManager
from grass.tools import Tools
from grass.exceptions import CalledModuleError


def test_reclass_as_mask_correct_state(session):
    """Check that r.mapcalc-generated raster works as mask with reclass"""
    tools = Tools(session=session)
    with MaskManager(env=session.env):
        tools.r_mask(raster="raster_mask")
        tools.r_univar(map="data")
        tools.r_mask(flags="r")


@pytest.mark.parametrize("bad_value", ["bad value", "nan", "inf", "#1", "name: bad"])
def test_reclass_as_mask_broken_data(session, bad_value):
    """Check that reading reclass map with broken data shows descriptive error"""
    tools = Tools(session=session)
    with MaskManager(env=session.env) as mask:
        tools.r_mask(raster="raster_mask")
        cellhd_file = Path(
            tools.g_findfile(file=mask.mask_name, element="cellhd", format="json")[
                "file"
            ]
        )
        content = cellhd_file.read_text(encoding="utf-8")
        content = content.replace("\n1\n", f"\n{bad_value}\n")
        cellhd_file.write_text(content, encoding="utf-8")
        with pytest.raises(CalledModuleError, match=bad_value):
            tools.r_univar(map="data")


def test_reclass_as_mask_repeated_hash_value_line(session):
    """Check that reading reclass map with an extra hash line"""
    tools = Tools(session=session)
    with MaskManager(env=session.env) as mask:
        tools.r_mask(raster="raster_mask")
        cellhd_file = Path(
            tools.g_findfile(file=mask.mask_name, element="cellhd", format="json")[
                "file"
            ]
        )
        content = cellhd_file.read_text(encoding="utf-8")
        content = content.replace("\n#1\n", "\n#1\n#42\n")
        cellhd_file.write_text(content, encoding="utf-8")
        with pytest.raises(CalledModuleError, match=r"(?s)#42.*min: 1"):
            tools.r_univar(map="data")


def test_reclass_as_mask_intermixed_hash_value_line(session):
    """Check that reading reclass map with an intermixed hash line"""
    tools = Tools(session=session)
    with MaskManager(env=session.env) as mask:
        tools.r_mask(raster="raster_mask")
        cellhd_file = Path(
            tools.g_findfile(file=mask.mask_name, element="cellhd", format="json")[
                "file"
            ]
        )
        content = cellhd_file.read_text(encoding="utf-8")
        content = content.replace("\n#1\n", "\n1\n2\n#1\n")
        cellhd_file.write_text(content, encoding="utf-8")
        with pytest.raises(CalledModuleError, match=r"(?s)not read yet.*#1"):
            tools.r_univar(map="data")
