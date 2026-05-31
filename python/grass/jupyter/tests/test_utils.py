def test_set_target_region_exists():
    """Basic sanity test to ensure function is callable."""
    from grass.jupyter.utils import set_target_region

    assert callable(set_target_region)
