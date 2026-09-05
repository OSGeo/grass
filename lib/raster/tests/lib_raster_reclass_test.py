import os
from io import StringIO
from pathlib import Path

import pytest

from grass.exceptions import CalledModuleError
from grass.experimental import MapsetSession
from grass.script import MaskManager
from grass.tools import Tools


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


def get_filename_length_limit(path):
    """Get maximum filename length for the given path"""
    try:
        # Expected to work on unix
        return os.pathconf(path, "PC_NAME_MAX")
    except (AttributeError, ValueError, OSError):
        # On Windows, there is no os.pathconf
        # and the other exceptions are from function doc.
        # Getting actual value for Windows would be complicated,
        # so we go with a low value rather than skipping the test.
        return 50


def test_long_names(session, tmp_path):
    """Check that long names are handled correctly"""
    # Using 255 because that's GNAME_MAX is 256 minus the terminator.
    name_size = min(get_filename_length_limit(tmp_path), 255)
    mapset_name = "m" * name_size
    long_name = "l" * name_size
    name = "r" * name_size
    with (
        MapsetSession(name=mapset_name, create=True, env=session.env) as mapset_session,
        Tools(session=mapset_session) as tools,
    ):
        # This one is the one to actually break when the names are too long for the
        # internal buffer - the raster being used in r.reclass has a long name.
        tools.r_mapcalc(expression=f"{long_name} = raster_mask")
        tools.r_reclass(input=long_name, output=name, rules=StringIO("1 = 5\n"))
        tools.r_univar(map=name)

        tools.r_reclass(input=name, output="short_name", rules=StringIO("1 = 5\n"))
        tools.r_univar(map="short_name")

        with MaskManager(env=mapset_session.env) as mask:
            tools.r_mask(raster=long_name, env=mask.env)
            tools.r_univar(map=name, env=mask.env)

        with MaskManager(env=mapset_session.env, mask_name="n" * name_size) as mask:
            tools.r_mask(raster=long_name, env=mask.env)
            tools.r_univar(map=name, env=mask.env)
