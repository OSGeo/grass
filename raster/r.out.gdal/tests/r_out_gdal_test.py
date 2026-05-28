def test_cog(tmp_path, session_tools):
    """Check that COG is generated"""
    output_file = tmp_path / "test_raster.tif"
    session_tools.r_out_gdal(
        input="rows_raster",
        output=output_file,
        format="COG",
        createopt="COMPRESS=LZW",
        overviews=2,
    )
    assert output_file.exists()
    assert output_file.is_file()
