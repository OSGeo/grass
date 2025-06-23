import grass.script as gs

DEFAULT_KEYS = [
    "version",
    "date",
    "revision",
    "build_date",
    "build_platform",
    "build_off_t_size",
]


def curly_brackets_paired(text):
    """Check whether all curly brackets in the given text are properly paired."""
    counter = 0
    for character in text:
        if character == "{":
            counter += 1
        elif character == "}":
            counter -= 1
        if counter < 0:
            return False
    return counter == 0


def test_g_version_no_flag(session):
    """Test that g.version output contains the word 'GRASS'."""
    output = gs.read_command("g.version", env=session.env).strip()
    assert "GRASS" in output, (
        "Expected 'GRASS' in g.version output, but it was not found."
    )

    plain_output = gs.read_command("g.version", format="plain", env=session.env).strip()
    assert output == plain_output, "Mismatch in plain output"


def test_c_flag(session):
    """Test the output of g.version -c for Copyright and License Statement."""
    expected_text = "Copyright and License Statement"
    output = gs.read_command("g.version", flags="c", env=session.env).strip()
    assert expected_text in output, (
        f"Expected '{expected_text}' in g.version -c output, but got: '{output}'"
    )

    # Test that g.version with -c flag and shell format contains expected output.
    shell_output = gs.read_command(
        "g.version", flags="c", format="shell", env=session.env
    )
    assert expected_text in shell_output, (
        f"Expected '{expected_text}' in g.version -c shell output, but got: '{shell_output}'"
    )


def test_e_flag(session):
    """Test that g.version -e contains the expected keys."""
    expected_keys = ["PROJ:", "GDAL/OGR:", "SQLite:"]
    output = gs.read_command("g.version", flags="e", env=session.env).strip()
    for key in expected_keys:
        assert key in output, (
            f"Expected key '{key}' in g.version -e output, but it was not found."
        )


def test_b_flag(session):
    """Test that g.version -b output contains the word 'GRASS'."""
    output = gs.read_command("g.version", flags="b", env=session.env).strip()
    assert "GRASS" in output, (
        "Expected 'GRASS' in g.version -b output, but it was not found."
    )


def test_g_flag(session):
    """Test that g.version -g contains the expected keys."""
    expected_keys = DEFAULT_KEYS
    output = gs.parse_command("g.version", flags="g", env=session.env)
    for key in expected_keys:
        assert key in output, (
            f"Expected key '{key}' in g.version -g output, but it was not found."
        )

    # Test that g.version with shell format contains the expected keys.
    shell_output = gs.parse_command("g.version", format="shell", env=session.env)
    assert output == shell_output, "Mismatch in shell output"


def test_r_flag(session):
    """Test that g.version -r contains the expected keys."""
    expected_texts = ["libgis revision:", "libgis date:"]
    output = gs.read_command("g.version", flags="r", env=session.env).strip()
    for text in expected_texts:
        assert text in output, (
            f"Expected key '{text}' in g.version -r output, but it was not found."
        )


def test_x_flag(session):
    """Test that g.version -x output has paired curly brackets."""
    output = gs.read_command("g.version", flags="x", env=session.env).strip()

    assert curly_brackets_paired(output), (
        "Curly brackets are not properly paired in the g.version -x output."
    )

    # Test that g.version with -x flag and shell output has paired curly brackets.
    shell_output = gs.read_command(
        "g.version", flags="x", format="shell", env=session.env
    )
    assert curly_brackets_paired(shell_output), (
        "Curly brackets are not properly paired in the g.version -x shell output."
    )


def test_e_flag_shell(session):
    """Test that g.version -e with shell format contains the expected keys."""
    expected_keys = [
        *DEFAULT_KEYS,
        "proj",
        "gdal",
        "geos",
        "sqlite",
    ]
    output = gs.parse_command("g.version", flags="e", format="shell", env=session.env)
    for key in expected_keys:
        assert key in output, (
            f"Expected key '{key}' in g.version -e shell output, but it was not found."
        )


def test_b_flag_shell(session):
    """Test that g.version -b with shell format produces non-empty output."""
    output = gs.read_command("g.version", flags="b", format="shell", env=session.env)
    # Ensure we got some output (and that the command didn't fail).
    assert output, (
        "Expected non-empty shell output for g.version -b, but got an empty string"
    )


def test_r_flag_shell(session):
    """Test that g.version -r with shell format contains the expected keys."""
    expected_keys = [*DEFAULT_KEYS, "libgis_revision", "libgis_date"]
    output = gs.parse_command("g.version", flags="r", format="shell", env=session.env)
    for key in expected_keys:
        assert key in output, (
            f"Expected key '{key}' in g.version -r shell output, but it was not found."
        )


def test_g_version_no_flag_json(session):
    """Test that g.version with json format contains the expected keys."""
    output = gs.parse_command("g.version", format="json", env=session.env)
    expected_keys = DEFAULT_KEYS
    for key in expected_keys:
        assert key in output.keys(), (
            f"Expected key '{key}' in g.version json output, but it was not found."
        )


def test_g_version_c_flag_json(session):
    """Test that g.version -c with json format contains the expected output."""
    output = gs.parse_command("g.version", flags="c", format="json", env=session.env)
    expected_keys = [
        *DEFAULT_KEYS,
        "copyright",
    ]
    for key in expected_keys:
        assert key in output.keys(), (
            f"Expected key '{key}' in g.version -c json output, but it was not found."
        )

    expected_copyright_text = "Copyright and License Statement"
    assert expected_copyright_text in output["copyright"], (
        f"Expected '{expected_copyright_text}' in g.version -c json output, but got: '{output['copyright']}'"
    )


def test_g_version_x_flag_json(session):
    """Test that g.version -x with json format contains the expected output."""
    output = gs.parse_command("g.version", flags="x", format="json", env=session.env)
    expected_keys = [
        *DEFAULT_KEYS,
        "citation",
    ]
    for key in expected_keys:
        assert key in output.keys(), (
            f"Expected key '{key}' in g.version -x json output, but it was not found."
        )

    assert curly_brackets_paired(output["citation"]), (
        "Curly brackets are not properly paired in the g.version -x json output."
    )


def test_g_version_b_flag_json(session):
    """Test that g.version -b with json format contains the expected output."""
    output = gs.parse_command("g.version", flags="b", format="json", env=session.env)
    expected_keys = [
        *DEFAULT_KEYS,
        "build_info",
    ]
    for key in expected_keys:
        assert key in output.keys(), (
            f"Expected key '{key}' in g.version -b json output, but it was not found."
        )

    # Ensure we got some output.
    assert output["build_info"], (
        "Expected non-empty json output for g.version -b, but got an empty string"
    )


def test_g_version_r_flag_json(session):
    """Test that g.version -r with json format contains the expected keys."""
    output = gs.parse_command("g.version", flags="r", format="json", env=session.env)
    expected_keys = [
        *DEFAULT_KEYS,
        "libgis_revision",
        "libgis_date",
    ]
    for key in expected_keys:
        assert key in output.keys(), (
            f"Expected key '{key}' in g.version -r json output, but it was not found."
        )


def test_g_version_e_flag_json(session):
    """Test that g.version -e with json format contains the expected keys."""
    output = gs.parse_command("g.version", flags="e", format="json", env=session.env)
    expected_keys = [
        *DEFAULT_KEYS,
        "proj",
        "gdal",
        "geos",
        "sqlite",
    ]
    for key in expected_keys:
        assert key in output.keys(), (
            f"Expected key '{key}' in g.version -e json output, but it was not found."
        )
