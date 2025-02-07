import grass.script as gs

DEFAULT_MASK_NAME = "MASK"


def raster_exists(name, env=None):
    return bool(gs.find_file(name, element="raster", env=env)["name"])


def raster_sum(name, env=None):
    return gs.parse_command("r.univar", map="ones", env=env, format="json")[0]["sum"]


def test_mask_manager_no_operation(session_2x2):
    """Test MaskManager not doing anything."""
    assert "GRASS_MASK" not in session_2x2.env

    with gs.MaskManager(env=session_2x2.env) as manager:
        # Inside context: mask name is generated, GRASS_MASK is set
        assert "GRASS_MASK" in session_2x2.env
        assert session_2x2.env["GRASS_MASK"] == manager.mask_name
        assert not raster_exists(manager.mask_name, env=session_2x2.env)
        status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
        assert status["name"].startswith(manager.mask_name)
        assert not status["present"]
        assert raster_sum("ones", env=session_2x2.env) == 4

    assert "GRASS_MASK" not in session_2x2.env
    assert not raster_exists(manager.mask_name, env=session_2x2.env)
    status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
    assert status["name"].startswith(DEFAULT_MASK_NAME)
    assert not status["present"]
    assert raster_sum("ones", env=session_2x2.env) == 4


def test_mask_manager_generated_name_remove_default_r_mask(session_2x2):
    """Test MaskManager with generated name and default remove=True."""
    assert "GRASS_MASK" not in session_2x2.env

    with gs.MaskManager(env=session_2x2.env) as manager:
        # Inside context: mask name is generated, GRASS_MASK is set
        assert "GRASS_MASK" in session_2x2.env
        assert session_2x2.env["GRASS_MASK"] == manager.mask_name
        gs.run_command("r.mask", raster="nulls_and_one_1_1", env=session_2x2.env)
        assert raster_exists(manager.mask_name, env=session_2x2.env)
        status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
        assert status["name"].startswith(manager.mask_name)
        assert status["present"]
        assert raster_sum("ones", env=session_2x2.env) == 1

    # After context: GRASS_MASK unset, mask should be removed
    assert "GRASS_MASK" not in session_2x2.env
    assert not raster_exists(manager.mask_name, env=session_2x2.env)
    status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
    assert status["name"].startswith(DEFAULT_MASK_NAME)
    assert not status["present"]
    assert raster_sum("ones", env=session_2x2.env) == 4


def test_mask_manager_generated_name_remove_true_r_mask(session_2x2):
    """Test MaskManager with generated name and default remove=True."""
    assert "GRASS_MASK" not in session_2x2.env

    with gs.MaskManager(env=session_2x2.env, remove=True) as manager:
        # Inside context: mask name is generated, GRASS_MASK is set
        assert "GRASS_MASK" in session_2x2.env
        assert session_2x2.env["GRASS_MASK"] == manager.mask_name
        gs.run_command("r.mask", raster="nulls_and_one_1_1", env=session_2x2.env)
        assert raster_exists(manager.mask_name, env=session_2x2.env)
        status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
        assert status["name"].startswith(manager.mask_name)
        assert status["present"]
        assert raster_sum("ones", env=session_2x2.env) == 1

    # After context: GRASS_MASK unset, mask should be removed
    assert "GRASS_MASK" not in session_2x2.env
    assert not raster_exists(manager.mask_name, env=session_2x2.env)
    status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
    assert status["name"].startswith(DEFAULT_MASK_NAME)
    assert not status["present"]
    assert raster_sum("ones", env=session_2x2.env) == 4


def test_mask_manager_generated_name_remove_false_r_mask(session_2x2):
    """Test MaskManager with generated name and remove=False."""
    assert "GRASS_MASK" not in session_2x2.env

    with gs.MaskManager(env=session_2x2.env, remove=False) as manager:
        assert "GRASS_MASK" in session_2x2.env
        assert session_2x2.env["GRASS_MASK"] == manager.mask_name
        gs.run_command("r.mask", raster="nulls_and_one_1_1", env=session_2x2.env)
        assert raster_exists(manager.mask_name, env=session_2x2.env)
        status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
        assert status["name"].startswith(manager.mask_name)
        assert status["present"]
        assert raster_sum("ones", env=session_2x2.env) == 1

    # After context: GRASS_MASK unset, mask should not be removed
    assert "GRASS_MASK" not in session_2x2.env
    assert raster_exists(manager.mask_name, env=session_2x2.env)
    status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
    assert status["name"].startswith(DEFAULT_MASK_NAME)
    assert not status["present"]
    assert raster_sum("ones", env=session_2x2.env) == 4


def test_mask_manager_generated_name_remove_true_g_copy(session_2x2):
    """Test MaskManager with generated name and default remove=True and g.copy mask."""
    assert "GRASS_MASK" not in session_2x2.env

    with gs.MaskManager(env=session_2x2.env) as manager:
        # Inside context: mask name is generated, GRASS_MASK is set
        assert "GRASS_MASK" in session_2x2.env
        assert session_2x2.env["GRASS_MASK"] == manager.mask_name
        gs.run_command(
            "g.copy",
            raster=("nulls_and_one_1_1", manager.mask_name),
            env=session_2x2.env,
        )
        status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
        assert status["name"].startswith(manager.mask_name)
        assert status["present"]
        assert raster_exists(manager.mask_name, env=session_2x2.env)
        assert raster_sum("ones", env=session_2x2.env) == 1

    # After context: GRASS_MASK unset, mask should be removed
    assert "GRASS_MASK" not in session_2x2.env
    assert not raster_exists(manager.mask_name, env=session_2x2.env)
    status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
    assert status["name"].startswith(DEFAULT_MASK_NAME)
    assert not status["present"]
    assert raster_sum("ones", env=session_2x2.env) == 4


def test_mask_manager_provided_name_remove_default(session_2x2):
    """Test MaskManager with provided name and default remove=False."""
    mask_name = "nulls_and_one_1_1"

    with gs.MaskManager(mask_name=mask_name, env=session_2x2.env, remove=None):
        assert "GRASS_MASK" in session_2x2.env
        assert session_2x2.env["GRASS_MASK"] == mask_name
        assert raster_exists(mask_name, env=session_2x2.env)
        status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
        assert status["name"].startswith(mask_name)
        assert status["present"]
        assert raster_sum("ones", env=session_2x2.env) == 1

    # After context: GRASS_MASK unset, mask should not be removed
    assert "GRASS_MASK" not in session_2x2.env
    assert raster_exists(mask_name, env=session_2x2.env)
    status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
    assert status["name"].startswith(DEFAULT_MASK_NAME)
    assert not status["present"]
    assert raster_sum("ones", env=session_2x2.env) == 4


def test_mask_manager_provided_classic_name_remove_true(session_2x2):
    """Test MaskManager with provided name and remove=True."""
    mask_name = DEFAULT_MASK_NAME

    with gs.MaskManager(mask_name=mask_name, env=session_2x2.env, remove=True):
        assert "GRASS_MASK" in session_2x2.env
        assert session_2x2.env["GRASS_MASK"] == mask_name
        gs.run_command("r.mask", raster="nulls_and_one_1_1", env=session_2x2.env)
        assert raster_exists(mask_name, env=session_2x2.env)
        status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
        assert status["name"].startswith(mask_name)
        assert status["present"]
        assert raster_sum("ones", env=session_2x2.env) == 1

    # After context: GRASS_MASK unset, mask should not be removed
    assert "GRASS_MASK" not in session_2x2.env
    assert not raster_exists(mask_name, env=session_2x2.env)
    status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
    assert status["name"].startswith(DEFAULT_MASK_NAME)
    assert not status["present"]
    assert raster_sum("ones", env=session_2x2.env) == 4


def test_mask_manager_provided_name_remove_true(session_2x2):
    """Test MaskManager with provided name and remove=True."""
    mask_name = "nulls_and_one_1_1"

    with gs.MaskManager(
        mask_name=mask_name, env=session_2x2.env, remove=True
    ) as manager:
        assert "GRASS_MASK" in session_2x2.env
        assert session_2x2.env["GRASS_MASK"] == mask_name
        assert raster_exists(mask_name, env=session_2x2.env)
        status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
        assert status["name"].startswith(manager.mask_name)
        assert status["present"]
        assert raster_sum("ones", env=session_2x2.env) == 1

    # After context: GRASS_MASK unset, mask should be removed
    assert "GRASS_MASK" not in session_2x2.env
    assert not raster_exists(mask_name, env=session_2x2.env)
    status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
    assert status["name"].startswith(DEFAULT_MASK_NAME)
    assert not status["present"]
    assert raster_sum("ones", env=session_2x2.env) == 4


def test_mask_manager_provided_name_remove_false(session_2x2):
    """Test MaskManager with provided name and remove=False."""
    mask_name = "nulls_and_one_1_1"

    with gs.MaskManager(
        mask_name=mask_name, env=session_2x2.env, remove=False
    ) as manager:
        assert "GRASS_MASK" in session_2x2.env
        assert session_2x2.env["GRASS_MASK"] == mask_name
        assert raster_exists(mask_name, env=session_2x2.env)
        status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
        assert status["name"].startswith(manager.mask_name)
        assert status["present"]
        assert raster_sum("ones", env=session_2x2.env) == 1

    # After context: GRASS_MASK unset, mask should not be removed
    assert "GRASS_MASK" not in session_2x2.env
    assert raster_exists(mask_name, env=session_2x2.env)
    status = gs.parse_command("r.mask.status", format="json", env=session_2x2.env)
    assert status["name"].startswith(DEFAULT_MASK_NAME)
    assert not status["present"]
    assert raster_sum("ones", env=session_2x2.env) == 4
