"""Tests of i.spectral"""

import ast

from grass.tools import Tools


def test_text_output_writes_band_values_per_pick(
    xy_raster_dataset_session_mapset, tmp_path
):
    """The -t text output lists each pick's band values, numbered per pick.

    Regression test: write2textf iterated with `for row in enumerate(what)`, so
    it wrote the `(index, row)` tuple verbatim instead of the band values. Two
    coordinate pairs also check the per-pick numbering.
    """
    tools = Tools(session=xy_raster_dataset_session_mapset)
    tools.g_region(n=10, s=0, w=0, e=10, res=1)
    tools.r_mapcalc(expression="b1 = 10")
    tools.r_mapcalc(expression="b2 = 20")
    tools.r_mapcalc(expression="b3 = 30")
    output = tmp_path / "spectrum.txt"
    tools.i_spectral(
        raster="b1,b2,b3",
        coordinates=(5, 5, 2, 2),
        output=str(output),
        flags="t",
    )
    lines = output.read_text(encoding="utf-8").splitlines()
    assert len(lines) == 2
    for expected_index, line in enumerate(lines, start=1):
        index_str, _, values_str = line.partition(", ")
        assert int(index_str) == expected_index
        assert ast.literal_eval(values_str) == [10.0, 20.0, 30.0]
