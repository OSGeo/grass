"""r.mapcalc tests for division by zero and math functions outside their domain"""

import pytest

from grass.tools import Tools


def cell_values(tools, name):
    """Return distinct cell values of a raster as text with * for NULL"""
    return set(tools.r_stats(input=name, flags="1").text.split())


@pytest.mark.parametrize("nprocs", [1, 4])
@pytest.mark.parametrize(
    "expression",
    [
        "5 % 0",
        "5 / 0",
        "float(5) % float(0)",
        "float(5) / float(0)",
        "5.0 % 0.0",
        "5.0 / 0.0",
        "sqrt(-1)",
        "log(0)",
        "log(-1)",
        "log(2, 0)",
        "log(2, -2)",
        "acos(2)",
        "asin(-2)",
        "exp(-8, 0.5)",
        "pow(-8.0, 0.5)",
        "pow(float(-8), float(0.5))",
    ],
)
def test_null_result(session_in_mapset, expression, nprocs):
    """Undefined results are NULL and the tool succeeds"""
    tools = Tools(session=session_in_mapset)
    tools.r_mapcalc(expression=f"result = {expression}", nprocs=nprocs)
    assert cell_values(tools, "result") == {"*"}


@pytest.mark.parametrize("nprocs", [1, 4])
@pytest.mark.parametrize("operator", ["%", "/"])
def test_zero_in_some_cells(session_in_mapset, operator, nprocs):
    """Only cells with zero divisor are NULL"""
    tools = Tools(session=session_in_mapset)
    # The divisor is zero in the third of the five rows.
    tools.r_mapcalc(
        expression=f"result = 7 {operator} (rows_raster - 3)", nprocs=nprocs
    )
    stats = tools.r_univar(map="result", format="json")
    assert stats["null_cells"] == 6
    assert stats["n"] == 24


@pytest.mark.parametrize("nprocs", [1, 4])
@pytest.mark.parametrize(
    ("expression", "expected"),
    [
        ("7 % 3", 1),
        ("-7 % 3", -1),
        ("7.5 % 2", 1.5),
        ("7 / 2", 3),
        ("7.0 / 2", 3.5),
        ("float(7) / float(2)", 3.5),
        ("sqrt(9)", 3),
        ("log(100, 10)", 2),
        ("exp(0)", 1),
        ("exp(2, 3)", 8),
        ("exp(-8, 3)", -512),
        ("pow(2, 3)", 8),
        ("pow(2.0, 0.5)", 2**0.5),
        ("pow(float(2), float(3))", 8),
        ("sin(30)", 0.5),
        ("cos(60)", 0.5),
        ("tan(45)", 1),
        ("asin(0.5)", 30),
        ("acos(0.5)", 60),
        ("atan(1)", 45),
        ("atan(0, 1)", 90),
        ("atan(0, -1)", 270),
    ],
)
def test_defined_result(session_in_mapset, expression, expected, nprocs):
    """Defined results are computed as usual"""
    tools = Tools(session=session_in_mapset)
    tools.r_mapcalc(expression=f"result = {expression}", nprocs=nprocs)
    stats = tools.r_univar(map="result", format="json")
    assert stats["null_cells"] == 0
    assert stats["min"] == pytest.approx(expected)
    assert stats["max"] == pytest.approx(expected)


@pytest.mark.parametrize("nprocs", [1, 4])
@pytest.mark.parametrize(
    "expression",
    ["exp(1000)", "pow(10.0, 400)", "pow(float(10), float(100))"],
)
def test_overflow_is_infinity(session_in_mapset, expression, nprocs):
    """Results too large for the type are infinity, not NULL"""
    tools = Tools(session=session_in_mapset)
    tools.r_mapcalc(expression=f"result = {expression}", nprocs=nprocs)
    assert cell_values(tools, "result") == {"inf"}
