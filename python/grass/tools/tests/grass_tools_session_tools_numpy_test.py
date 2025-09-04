import numpy as np

import grass.script.array as ga
from grass.tools import Tools


def test_numpy_one_input(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used"""
    tools = Tools(session=xy_dataset_session)
    tools.r_slope_aspect(elevation=np.ones((1, 1)), slope="slope")
    assert tools.r_info(map="slope", format="json")["datatype"] == "FCELL"


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
    const_5 = ga.array("const_5", env=xy_dataset_session.env)
    result = tools.r_mapcalc_simple(expression="2 * A", a=const_5, output=ga.array)
    assert result.shape == (rows, cols)
    assert np.all(result == np.full((rows, cols), 10))
    assert isinstance(result, ga.array)


def test_numpy_one_input_multiple_outputs_into_result(xy_dataset_session):
    """Check that we can store NumPy arrays in the result object"""
    tools = Tools(session=xy_dataset_session, consistent_return_value=True)
    tools.g_region(rows=2, cols=3)
    result = tools.r_slope_aspect(
        elevation=np.ones((2, 3)), slope=np.array, aspect=np.array
    )
    assert result.arrays.slope.shape == (2, 3)
    assert np.all(result.arrays.slope == np.full((2, 3), 0))
    assert result.arrays.aspect.shape == (2, 3)
    assert np.all(result.arrays.aspect == np.full((2, 3), 0))

    # Unpacking tuple works with the array attribute of the result object.
    (slope, aspect) = result.arrays
    assert slope.shape == (2, 3)
    assert aspect.shape == (2, 3)

    # Other attributes of result object still work.
    assert result.stdout == ""
    assert result.text == ""
    assert len(result.stderr)
