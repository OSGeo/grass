import json
import grass.script as gs
import pytest


@pytest.mark.parametrize("method, threshold, look_ahead, reduction", [
    ("douglas", 1.0, 7, 50),
    ("lang", 2.0, 7, 50),
    ("reduction", 0.5, 7, 50),
    ("snakes", 1.0, 7, 75),
    ("chaiken", 0.5, 7, 100)
])
def test_generalization_parameters(area_dataset, method, threshold, look_ahead, reduction):
    """Test v.generalize with different methods, thresholds, and additional parameters"""

    output = f"generalized_{method}_{threshold}_{look_ahead}_{reduction}"
    
    # Run the v.generalize command with the given parameters
    gs.run_command(
        "v.generalize",
        input=area_dataset.name,
        output=output,
        method=method,
        threshold=threshold,
        look_ahead=look_ahead,
        reduction=reduction,
        env=area_dataset.session.env,
    )

    # Check the vector information for both original and generalized vectors
    original_info = gs.vector_info(area_dataset.name, env=area_dataset.session.env)
    info = gs.vector_info(output, env=area_dataset.session.env)

    # Assert that certain features changed as expected
    assert info["nodes"] < original_info["nodes"], "Number of nodes should decrease"
    assert info["points"] < original_info["points"], "Number of points should decrease"
    assert info["lines"] <= original_info["lines"], "Number of lines should not increase"
    assert info["boundaries"] <= original_info["boundaries"], "Number of boundaries should not increase"
    
    # Check that areas and islands remain the same
    assert info["areas"] == original_info["areas"], "Number of areas should remain the same"
    assert info["islands"] == original_info["islands"], "Number of islands should remain the same"

    # Optional: Check the attribute table if necessary
    records = json.loads(
        gs.read_command(
            "v.db.select", map=output, format="json", env=area_dataset.session.env
        )
    )["records"]
    
    # Example to check that the attribute table is consistent
    assert len(records) == len(original_info["primitives"]), "Attribute records count mismatch"
    
    # Clean up: remove the output vector map after the test
    gs.run_command('g.remove', type='vector', name=output)


@pytest.mark.parametrize("flag", [
    ("-l"),  # Disable loop support
    ("-t"),  # Do not copy attributes
    ("--overwrite"),  # Allow overwriting files
])
def test_generalization_flags(area_dataset, flag):
    """Test v.generalize with different flags to control behavior"""
    
    output = f"generalized_flags_{flag}"

    # Run the v.generalize command with a sample method and threshold, applying the flag
    gs.run_command(
        "v.generalize",
        input=area_dataset.name,
        output=output,
        method="douglas",
        threshold=1.0,
        flags=flag,  # Apply the specified flag
        env=area_dataset.session.env,
    )

    # Check the vector information for both original and generalized vectors
    original_info = gs.vector_info(area_dataset.name, env=area_dataset.session.env)
    info = gs.vector_info(output, env=area_dataset.session.env)

    # Check generalization outcomes
    assert info["nodes"] < original_info["nodes"], "Number of nodes should decrease"
    assert info["points"] < original_info["points"], "Number of points should decrease"
    
    # Clean up: remove the output vector map after the test
    gs.run_command('g.remove', type='vector', name=output)
