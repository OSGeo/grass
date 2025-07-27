import numpy as np

import grass.script as gs
from grass.tools import Tools


def test_numpy_one_input(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used"""
    tools = Tools(session=xy_dataset_session)
    tools.r_slope_aspect(elevation=np.ones((1, 1)), slope="slope")
    assert tools.r_info(map="slope", format="json")["datatype"] == "FCELL"


# NumPy syntax for outputs
# While inputs are straightforward, there is several possible ways how to handle
# syntax for outputs.
# Output is the type of function for creating NumPy arrays, return value is now the arrays:
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope=np.ndarray, aspect=np.array)
# Output is explicitly requested:
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope="slope", aspect="aspect", force_numpy_for_output=True)
# Output is explicitly requested at the object level:
# Tools(force_numpy_for_output=True).r_slope_aspect(elevation=np.ones((1, 1)), slope="slope", aspect="aspect")
# Output is always array or arrays when at least on input is an array:
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope="slope", aspect="aspect")
# An empty array is passed to signal the desired output:
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope=np.nulls((0, 0)))
# An array to be filled with data is passed, the return value is kept as is:
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope=np.nulls((1, 1)))
# NumPy universal function concept can be used explicitly to indicate,
# possibly more easily allowing for nameless args as opposed to keyword arguments,
# but outputs still need to be explicitly requested:
# Returns by value (tuple: (np.array, np.array)):
# tools.r_slope_aspect.ufunc(np.ones((1, 1)), slope=True, aspect=True)
# Modifies its arguments in-place:
# tools.r_slope_aspect.ufunc(np.ones((1, 1)), slope=True, aspect=True, out=(np.array((1, 1)), np.array((1, 1))))
# Custom signaling classes or objects are passed (assuming empty classes AsNumpy and AsInput):
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope=ToNumpy(), aspect=ToNumpy())
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope=AsInput, aspect=AsInput)
# NumPy functions usually return a tuple, for multiple outputs. Universal function does
# unless the output is written to out parameter which is also provided as a tuple. We
# have names, so generally, we can return a dictionary:
# {"slope": np.array(...), "aspect": np.array(...) }.


def test_numpy_one_input_one_output(xy_dataset_session):
    """Check that a NumPy array works as input and for signaling output

    It tests that the np.ndarray class is supported to signal output.
    Return type is not strictly defined, so we are not testing for it explicitly
    (only by actually using it as an NumPy array).
    """
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=2, cols=3)
    slope = tools.r_slope_aspect(elevation=np.ones((2, 3)), slope=np.ndarray)
    assert slope.shape == (2, 3)
    assert np.all(slope == np.full((2, 3), 0))


def test_numpy_with_name_and_parameter(xy_dataset_session):
    """Check that a NumPy array works as input and for signaling output

    It tests that the np.ndarray class is supported to signal output.
    Return type is not strictly defined, so we are not testing for it explicitly
    (only by actually using it as an NumPy array).
    """
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=2, cols=3)
    slope = tools.run("r.slope.aspect", elevation=np.ones((2, 3)), slope=np.ndarray)
    assert slope.shape == (2, 3)
    assert np.all(slope == np.full((2, 3), 0))


def test_numpy_one_input_multiple_outputs(xy_dataset_session):
    """Check that a NumPy array function works for signaling multiple outputs

    Besides multiple outputs it tests that np.array is supported to signal output.
    """
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=2, cols=3)
    (slope, aspect) = tools.r_slope_aspect(
        elevation=np.ones((2, 3)), slope=np.array, aspect=np.array
    )
    assert slope.shape == (2, 3)
    assert np.all(slope == np.full((2, 3), 0))
    assert aspect.shape == (2, 3)
    assert np.all(aspect == np.full((2, 3), 0))


def test_numpy_multiple_inputs_one_output(xy_dataset_session):
    """Check that a NumPy array works for multiple inputs"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=2, cols=3)
    result = tools.r_mapcalc_simple(
        expression="A + B", a=np.full((2, 3), 2), b=np.full((2, 3), 5), output=np.array
    )
    assert result.shape == (2, 3)
    assert np.all(result == np.full((2, 3), 7))


def test_numpy_grass_array_input_output(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used

    When grass array output is requested, we explicitly test the return value type.
    """
    tools = Tools(session=xy_dataset_session)
    rows = 2
    cols = 3
    tools.g_region(rows=rows, cols=cols)
    tools.r_mapcalc_simple(expression="5", output="const_5")
    const_5 = gs.array.array("const_5", env=xy_dataset_session.env)
    result = tools.r_mapcalc_simple(
        expression="2 * A", a=const_5, output=gs.array.array
    )
    assert result.shape == (rows, cols)
    assert np.all(result == np.full((rows, cols), 10))
    assert isinstance(result, gs.array.array)
