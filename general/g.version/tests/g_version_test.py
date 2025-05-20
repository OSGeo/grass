import grass.script as gs


def test_g_version_no_flag(session):
    """Test that g.version output contains the word 'GRASS'."""
    output = gs.read_command("g.version", env=session.env).strip()
    assert "GRASS" in output, (
        "Expected 'GRASS' in g.version output, but it was not found."
    )


def test_c_flag(session):
    """Test the output of g.version -c for Copyright and License Statement."""
    expected_text = "Copyright and License Statement"
    output = gs.read_command("g.version", flags="c", env=session.env).strip()
    assert expected_text in output, (
        f"Expected '{expected_text}' in g.version -c output, but got: '{output}'"
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
    expected_keys = [
        "version",
        "date",
        "revision",
        "build_date",
        "build_platform",
        "build_off_t_size",
    ]
    output = gs.parse_command("g.version", flags="g", env=session.env)
    for key in expected_keys:
        assert key in output, (
            f"Expected key '{key}' in g.version -g output, but it was not found."
        )


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

    def curly_brackets_paired(text):
        counter = 0
        for character in text:
            if character == "{":
                counter += 1
            elif character == "}":
                counter -= 1
            if counter < 0:
                return False
        return counter == 0

    assert curly_brackets_paired(output), (
        "Curly brackets are not properly paired in the g.version -x output."
    )
