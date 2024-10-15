import grass.script as gs


def test_mask_manager_generated_name_remove_default(session):
    """
    Test MaskManager with a generated mask name.
    When remove is None, the mask should be removed after context.
    """
    assert "GRASS_MASK" not in session.env

    with gs.MaskManager(env=session.env) as manager:
        # Mask name should be generated
        assert "GRASS_MASK" in session.env
        assert session.env["GRASS_MASK"] == manager.mask_name

    # After context: GRASS_MASK should be unset and mask should be removed
    assert "GRASS_MASK" not in session.env
    # Check if the mask has been removed
    assert not gs.find_file(manager.mask_name, element="raster", env=session.env)[
        "name"
    ]
