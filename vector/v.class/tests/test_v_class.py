import os
import pytest
import shutil
import grass.script as gs
from grass.exceptions import CalledModuleError


@pytest.fixture(scope="module")
def setup_vector_with_values(tmp_path_factory):
    """
    Pytest fixture to create a temporary GRASS project with a vector map
    containing sample points and associated attribute data.

    This setup initializes the GRASS environment, sets the computational region,
    imports point data from ASCII format, creates an attribute column 'value',
    and populates it with numeric values for testing classification.

    Yields:
        session: active GRASS session with the prepared project and vector map.
    """
    # Create a single temporary project directory once per module
    project = tmp_path_factory.mktemp("v_class_project")

    # Remove if exists (defensive cleanup)
    if project.exists():
        shutil.rmtree(project)

    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", n=10, s=0, e=10, w=0, res=1, env=session.env)

        vector_input = """\
1 1 1
2 1 2
3 1 3
4 1 4
5 1 5
6 1 6
7 1 7
8 1 8
9 1 9
10 1 10
"""
        gs.write_command(
            "v.in.ascii",
            input="-",
            output="test_points",
            format="point",
            separator="space",
            overwrite=True,
            env=session.env,
            stdin=vector_input.encode("utf-8"),
        )

        gs.run_command(
            "v.db.addcolumn", map="test_points", columns="value double", env=session.env
        )

        sql_statements = "\n".join(
            f"UPDATE test_points SET value = {val} WHERE cat = {cat};"
            for cat, val in zip(range(1, 11), range(1, 11), strict=False)
        )
        gs.write_command(
            "db.execute",
            input="-",
            stdin=sql_statements.encode("utf-8"),
            env=session.env,
        )

        yield session


@pytest.mark.parametrize(
    ("algorithm", "expected"),
    [
        ("int", [2.8, 4.6, 6.4, 8.2]),  # Equal interval over range 1-10
        ("qua", [3.0, 5.0, 7.0, 9.0]),  # Quantiles on 10 values
    ],
)
def test_v_class_break_values(setup_vector_with_values, algorithm, expected):
    """
    Test that v.class returns the correct break values for known algorithms.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm=algorithm,
        nbclasses=5,
        flags="g",
        env=session.env,
    )
    breaks = [float(b) for b in output.strip().split(",")]
    assert breaks == pytest.approx(expected, rel=1e-2)

    # Test with explicit 'plain' format
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm=algorithm,
        nbclasses=5,
        flags="g",
        format="plain",
        env=session.env,
    )
    breaks = [float(b) for b in output.strip().split(",")]
    assert breaks == pytest.approx(expected, rel=1e-2)


def test_v_class_required(setup_vector_with_values):
    """
    Test that v.class produces the expected output when only the required inputs are provided.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm="qua",
        nbclasses=5,
        env=session.env,
    )

    result = output.splitlines()
    expected = [
        "",
        "Classification of value into 5 classes",
        "Using algorithm: *** qua ***",
        "Mean: 5.500000\tStandard deviation = 2.872281",
        "",
        "   From (excl.)     To (incl.)      Frequency",
        "",
        "        1.00000        3.00000              3",
        "        3.00000        5.00000              2",
        "        5.00000        7.00000              2",
        "        7.00000        9.00000              2",
        "        9.00000       10.00000              1",
        "",
        "Note: Minimum of first class is including",
        "",
    ]
    assert result == expected

    # Test with explicit 'plain' format
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm="qua",
        nbclasses=5,
        format="plain",
        env=session.env,
    )
    result = output.splitlines()
    assert result == expected


def test_v_class_expression_breaks(setup_vector_with_values):
    """
    Test break values when passing a column expression (value/2).
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value/2",
        algorithm="int",
        nbclasses=5,
        flags="g",
        env=session.env,
    )
    breaks = [float(b) for b in output.strip().split(",")]
    expected = [1.4, 2.3, 3.2, 4.1]  # breaks computed over values 0.5–5.0
    assert breaks == pytest.approx(expected, rel=1e-2)


def test_v_class_with_where_clause(setup_vector_with_values):
    """
    Test filtering using SQL WHERE clause. Only values >= 6 should be considered.
    Values: 6, 7, 8, 9, 10 → breaks for 2 classes should split those.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        where="value >= 6",
        algorithm="qua",
        nbclasses=2,
        flags="g",
        env=session.env,
    )
    breaks = [float(b) for b in output.strip().split(",")]
    assert breaks == pytest.approx([8.0], rel=1e-2)


def test_v_class_invalid_column(setup_vector_with_values):
    """
    Test that running classification on an invalid/nonexistent column raises a CalledModuleError.

    Ensures error handling works properly when user requests classification on a column not present in the attribute table.
    """
    session = setup_vector_with_values
    with pytest.raises(CalledModuleError):
        gs.read_command(
            "v.class",
            map="test_points",
            column="invalid_column",
            algorithm="int",
            nbclasses=5,
            flags="g",
            env=session.env,
        )


@pytest.mark.parametrize(
    ("nbclasses", "expected_breaks"),
    [(2, [6.0]), (10, [2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0])],
)
def test_v_class_varied_nbclasses(setup_vector_with_values, nbclasses, expected_breaks):
    """
    Test v.class with different numbers of classes and check expected breaks.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm="qua",
        nbclasses=nbclasses,
        flags="g",
        env=session.env,
    )
    breaks = [float(b) for b in output.strip().split(",")]
    assert breaks == pytest.approx(expected_breaks, rel=1e-2)


def test_v_class_non_numeric_column(setup_vector_with_values):
    """
    Test that attempting classification on a non-numeric column raises an error.

    Adds a text column 'name', populates it with string data, and verifies `v.class` fails gracefully.
    """
    session = setup_vector_with_values
    gs.run_command(
        "v.db.addcolumn", map="test_points", columns="name varchar(20)", env=session.env
    )

    # Populate 'name' with string values (all 'A')
    sql_statements = "\n".join(
        f"UPDATE test_points SET name = 'A' WHERE cat = {cat};" for cat in range(1, 11)
    )
    gs.write_command(
        "db.execute",
        input="-",
        stdin=sql_statements.encode("utf-8"),
        env=session.env,
    )

    with pytest.raises(CalledModuleError):
        gs.read_command(
            "v.class",
            map="test_points",
            column="name",
            algorithm="int",
            nbclasses=5,
            flags="g",
            env=session.env,
        )


def test_v_class_where_no_matches(setup_vector_with_values):
    """
    Test behavior when the WHERE clause matches no records.

    Ensures break values are zeroed when classification has no input data.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        where="value > 100",  # No points satisfy this
        algorithm="int",
        nbclasses=5,
        flags="g",
        env=session.env,
    )
    breaks = [float(b) for b in output.strip().split(",")] if output.strip() else []
    assert len(breaks) == 4  # For 5 classes → 4 breaks
    assert all(b == 0.0 for b in breaks)


# @pytest.mark.skip(reason="Temporarily skipped due to CI performance constraints")
def test_v_class_large_dataset(tmp_path):
    """
    Test classification on a larger dataset (100 points).

    Verifies break count, sorted order, and plausible range of break values.
    """
    project = tmp_path / "large_project"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", n=100, s=0, e=100, w=0, res=1, env=session.env)

        points = "\n".join(f"{i} 1 {i}" for i in range(1, 101))

        gs.write_command(
            "v.in.ascii",
            input="-",
            output="large_points",
            format="point",
            separator="space",
            overwrite=True,
            env=session.env,
            stdin=points.encode("utf-8"),
        )

        gs.run_command(
            "v.db.addcolumn",
            map="large_points",
            columns="value double",
            env=session.env,
        )

        sql_statements = "\n".join(
            f"UPDATE large_points SET value = {cat} WHERE cat = {cat};"
            for cat in range(1, 101)
        )
        gs.write_command(
            "db.execute",
            input="-",
            stdin=sql_statements.encode("utf-8"),
            env=session.env,
        )

        output = gs.read_command(
            "v.class",
            map="large_points",
            column="value",
            algorithm="int",
            nbclasses=10,
            flags="g",
            env=session.env,
        )
        breaks = [float(b) for b in output.strip().split(",")] if output.strip() else []

        # Sanity checks:
        assert len(breaks) == 9  # 10 classes → 9 breaks
        assert all(
            breaks[i] <= breaks[i + 1] for i in range(len(breaks) - 1)
        )  # sorted ascending
        # Values should be within data range 1-100
        assert all(1 <= b <= 100 for b in breaks)


def test_v_class_output_format_consistency(setup_vector_with_values):
    """
    Test that the output of `v.class` is always a comma-separated list of floats.

    Checks this for all supported algorithms.
    """
    session = setup_vector_with_values
    for algorithm in ["int", "std", "qua", "equ", "dis"]:
        output = gs.read_command(
            "v.class",
            map="test_points",
            column="value",
            algorithm=algorithm,
            nbclasses=5,
            flags="g",
            env=session.env,
        )
        parts = output.strip().split(",")
        for part in parts:
            # Assert each part can be converted to float successfully
            float_val = float(part)
            assert isinstance(float_val, float)


# Optional: test that warnings are issued for discontinuities algorithm
@pytest.mark.filterwarnings("ignore")  # Suppress warning output during test
def test_v_class_warning_discontinuities(setup_vector_with_values):
    """
    Test that the 'dis' (discontinuities) algorithm runs and produces output.

    Validates output format and break values.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm="dis",
        nbclasses=5,
        flags="g",
        env=session.env,
    )
    breaks = [float(b) for b in output.strip().split(",")] if output.strip() else []

    # Checks:
    assert len(breaks) == 4  # 5 classes → 4 breaks
    assert all(isinstance(b, float) for b in breaks)
    assert all(
        breaks[i] <= breaks[i + 1] for i in range(len(breaks) - 1)
    )  # sorted ascending


def assert_json_equal(expected, actual):
    if isinstance(expected, dict):
        assert isinstance(actual, dict)
        assert len(expected.keys()) == len(actual.keys())
        for key in expected:
            assert_json_equal(expected[key], actual[key])
    elif isinstance(expected, list):
        assert isinstance(actual, list)
        assert len(expected) == len(actual)
        for exp_item, act_item in zip(expected, actual, strict=False):
            assert_json_equal(exp_item, act_item)
    elif isinstance(expected, float):
        assert actual == pytest.approx(expected, rel=1e-6)
    else:
        assert expected == actual


def test_v_class_required_json(setup_vector_with_values):
    """
    Test that v.class produces the expected output when only the required inputs are provided with JSON format.
    """
    session = setup_vector_with_values
    result = gs.parse_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm="qua",
        nbclasses=5,
        format="json",
        env=session.env,
    )

    expected = {
        "classes": 5,
        "mean": 5.5,
        "standard_deviation": 2.8722813232690143,
        "breaks": [3, 5, 7, 9],
        "intervals": [
            {"frequency": 3, "from": 1, "to": 3},
            {"frequency": 2, "from": 3, "to": 5},
            {"frequency": 2, "from": 5, "to": 7},
            {"frequency": 2, "from": 7, "to": 9},
            {"frequency": 1, "from": 9, "to": 10},
        ],
    }
    assert_json_equal(expected, result)


def test_v_class_breaks_json(setup_vector_with_values):
    """
    Test that v.class produces the correct break values with JSON format.
    """
    session = setup_vector_with_values
    result = gs.parse_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm="qua",
        nbclasses=5,
        format="json",
        flags="b",
        env=session.env,
    )

    expected = [3, 5, 7, 9]
    assert_json_equal(expected, result)


def test_v_class_required_csv(setup_vector_with_values):
    """
    Test that v.class produces the expected output when only the required inputs are provided with CSV format.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm="qua",
        nbclasses=5,
        format="csv",
        env=session.env,
    )

    expected = [
        "from,to,frequency",
        "1.00000,3.00000,3",
        "3.00000,5.00000,2",
        "5.00000,7.00000,2",
        "7.00000,9.00000,2",
        "9.00000,10.00000,1",
    ]
    result = output.splitlines()
    assert result == expected


def test_v_class_breaks_list(setup_vector_with_values):
    """
    Test that v.class produces the correct break values with LIST format.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm="qua",
        nbclasses=5,
        format="list",
        flags="b",
        env=session.env,
    )

    expected = [3, 5, 7, 9]
    breaks = [float(b) for b in output.strip().split(",")]
    assert breaks == pytest.approx(expected, rel=1e-2)


def test_v_class_breaks_separator_list(setup_vector_with_values):
    """
    Test that v.class produces the correct break values with LIST format and pipe separator.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm="qua",
        nbclasses=5,
        format="list",
        flags="b",
        separator="pipe",
        env=session.env,
    )

    expected = [3, 5, 7, 9]
    breaks = [float(b) for b in output.strip().split("|")]
    assert breaks == pytest.approx(expected, rel=1e-2)
