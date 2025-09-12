import os
import stat

import pytest

from grass.exceptions import CalledModuleError
from grass.tools import Tools


@pytest.mark.parametrize("compression_flag", ["", "c"])
@pytest.mark.parametrize(
    "suffix",
    [".pack", ".rpack", ".r_pack", ".grass_raster", ".grr", ".arbitrary_suffix_1"],
)
def test_pack_different_suffixes(
    xy_raster_dataset_session_for_module, tmp_path, suffix, compression_flag
):
    """Check that non-zero output is created with different suffixes

    The compression should not change the allowed suffixes.
    """
    tools = Tools(session=xy_raster_dataset_session_for_module)
    output = (tmp_path / "output_1").with_suffix(suffix)
    tools.r_pack(input="rows_raster", output=output, flags=compression_flag)
    assert output.exists()
    assert output.is_file()
    assert output.stat().st_size > 0


def test_input_does_not_exist(xy_empty_dataset_session, tmp_path):
    """Check we raise when input does not exists"""
    tools = Tools(session=xy_empty_dataset_session)
    output = tmp_path / "output_1.rpack"
    with pytest.raises(CalledModuleError, match="does_not_exist"):
        tools.r_pack(input="does_not_exist", output=output)


def test_output_exists_without_overwrite(
    xy_raster_dataset_session_for_module, tmp_path
):
    """Check behavior when output already exists"""
    tools = Tools(session=xy_raster_dataset_session_for_module)
    output = tmp_path / "output_1.rpack"
    assert not output.exists()
    tools.r_pack(input="rows_raster", output=output)
    assert output.exists()
    with pytest.raises(CalledModuleError, match=rf"(?s){output}.*[Oo]verwrite"):
        # Same call repeated again.
        tools.r_pack(input="rows_raster", output=output)
    assert output.exists()


def test_output_exists_with_overwrite(xy_raster_dataset_session_for_module, tmp_path):
    """Check behavior when output already exists and overwrite is set"""
    tools = Tools(session=xy_raster_dataset_session_for_module)
    output = tmp_path / "output_1.rpack"
    assert not output.exists()
    tools.r_pack(input="rows_raster", output=output)
    assert output.exists()
    # Same call repeated again but with overwrite.
    tools.r_pack(input="rows_raster", output=output, overwrite=True)
    assert output.exists()


def test_output_dir_does_not_exist(xy_raster_dataset_session_for_module, tmp_path):
    """Check behavior when directory for output does not exist"""
    tools = Tools(session=xy_raster_dataset_session_for_module)
    output = tmp_path / "does_not_exist" / "output_1.rpack"
    assert not output.exists()
    assert not output.parent.exists()
    with pytest.raises(
        CalledModuleError, match=rf"(?s)'{output.parent}'.*does not exist"
    ):
        tools.r_pack(input="rows_raster", output=output)


def test_output_dir_is_file(xy_raster_dataset_session_for_module, tmp_path):
    """Check behavior when what should be a directory is a file"""
    tools = Tools(session=xy_raster_dataset_session_for_module)
    parent = tmp_path / "file_as_dir"
    parent.touch()
    output = parent / "output_1.rpack"
    assert not output.exists()
    assert output.parent.exists()
    assert output.parent.is_file()
    with pytest.raises(
        CalledModuleError, match=rf"(?s)'{output.parent}'.*is not.*directory"
    ):
        tools.r_pack(input="rows_raster", output=output)


def test_output_dir_is_not_writable(xy_raster_dataset_session_for_module, tmp_path):
    """Check behavior when directory is not writable"""
    tools = Tools(session=xy_raster_dataset_session_for_module)
    parent = tmp_path / "parent_dir"
    parent.mkdir()
    parent.chmod(stat.S_IREAD)
    output = parent / "output_1.rpack"
    assert not os.path.exists(output)  # output.exists() gives permission denied
    assert output.parent.exists()
    assert output.parent.is_dir()
    with pytest.raises(
        CalledModuleError, match=rf"(?s)'{output.parent}'.*is not.*writable"
    ):
        tools.r_pack(input="rows_raster", output=output)


@pytest.mark.parametrize("compression_flag", ["", "c"])
def test_round_trip_with_r_unpack(
    xy_raster_dataset_session_mapset, tmp_path, compression_flag
):
    """Check that we get the same values with r.unpack"""
    tools = Tools(session=xy_raster_dataset_session_mapset)
    output = tmp_path / "output_1.rpack"
    tools.r_pack(input="rows_raster", output=output, flags=compression_flag)
    imported = "imported"
    tools.r_unpack(input=output, output=imported)
    tools.r_mapcalc(expression="diff = rows_raster - imported")
    stats = tools.r_univar(map="diff", format="json")
    assert stats["n"] > 1
    assert stats["sum"] == 0
    assert stats["min"] == 0
    assert stats["max"] == 0
