import pytest
import grass.script as gs

def test_vgeneralize():
    # Run v.generalize on the sample vector map
    gs.run_command('v.generalize', input='sample_points', output='generalized_points', threshold=1.0)

    # Read the output vector map to verify results
    original_map = gs.read_command('v.db.select', map='sample_points')
    generalized_map = gs.read_command('v.db.select', map='generalized_points')

    # Compare the results to ensure no changes (i.e., the test fails if the map changes)
    assert original_map == generalized_map, "Test failed: The output map is different from the original map."
