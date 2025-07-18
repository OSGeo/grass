import os
import pytest
import grass.script as gs
from grass.exceptions import CalledModuleError


@pytest.fixture
def setup_vector_with_values(tmp_path):
    """
    Pytest fixture to create a temporary GRASS GIS project with a vector map
    containing sample points and associated attribute data.

    This setup initializes the GRASS environment, sets the computational region,
    imports point data from ASCII format, creates an attribute column 'value',
    and populates it with numeric values for testing classification.

    Yields:
        session: active GRASS GIS session with the prepared project and vector map.
    """
    project = tmp_path / "v_class_project"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        # Define the computational region (extent and resolution)
        gs.run_command("g.region", n=10, s=0, e=10, w=0, res=1, env=session.env)

        # ASCII representation of 10 points with categories (cat) 1 to 10
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
        # Import points into a new vector map named 'test_points'
        gs.write_command(
            "v.in.ascii",
            input="-",  # Use '-' to read data from stdin
            output="test_points",
            format="point",
            separator="space",
            overwrite=True,
            env=session.env,
            stdin=vector_input.encode("utf-8"),  # Provide ASCII data as bytes to stdin
        )

        # Add a new attribute column 'value' of type double precision float
        gs.run_command(
            "v.db.addcolumn", map="test_points", columns="value double", env=session.env
        )

        # Update each point's 'value' attribute to match its category (cat)
        for cat, val in enumerate(range(1, 11), start=1):
            gs.run_command(
                "db.execute",
                sql=f"UPDATE test_points SET value = {val} WHERE cat = {cat}",
                env=session.env,
            )

        yield session


@pytest.mark.parametrize("algorithm", ["int", "std", "qua", "equ", "dis"])
def test_v_class_returns_expected_class_count(setup_vector_with_values, algorithm):
    """
    Test that `v.class` returns the correct number of break points for various classification algorithms.

    Given 5 classes, the number of breaks returned should be 4 (number_of_classes - 1).
    This test runs classification with different supported algorithms and verifies the output format.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm=algorithm,
        nbclasses=5,
        flags="g",  # Flag to return breaks as a comma-separated list (no extra output)
        env=session.env,
    )
    breaks = [float(b) for b in output.strip().split(",")]
    assert len(breaks) == 4  # For 5 classes, expect 4 break points


def test_v_class_with_expression(setup_vector_with_values):
    """
    Test classification using a column expression instead of a plain column name.

    Here, the column expression 'value/2' is used to verify that `v.class` handles expressions correctly.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value/2",  # Using expression rather than a direct column
        algorithm="int",
        nbclasses=5,
        flags="g",
        env=session.env,
    )
    breaks = [float(b) for b in output.strip().split(",")]
    assert len(breaks) == 4


def test_v_class_with_where_clause(setup_vector_with_values):
    """
    Test classification with a SQL WHERE clause filtering the input features.

    This test restricts the classification to points with 'value' >= 6 and verifies the number of breaks.
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
    assert len(breaks) == 1  # For 2 classes, expect 1 break


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


@pytest.mark.parametrize(("nbclasses", "expected_breaks"), [(1, 1), (2, 1), (10, 9)])
def test_v_class_varied_nbclasses(setup_vector_with_values, nbclasses, expected_breaks):
    """
    Test classification with different numbers of classes.

    Verifies that the number of returned break points matches the expected count for various class counts.
    """
    session = setup_vector_with_values
    output = gs.read_command(
        "v.class",
        map="test_points",
        column="value",
        algorithm="int",
        nbclasses=nbclasses,
        flags="g",
        env=session.env,
    )
    breaks = [float(b) for b in output.strip().split(",")] if output.strip() else []
    assert len(breaks) == expected_breaks


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
    for cat, val in zip(range(1, 11), ["A"] * 10):
        gs.run_command(
            "db.execute",
            sql=f"UPDATE test_points SET name = '{val}' WHERE cat = {cat}",
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
    Test classification behavior when the WHERE clause excludes all rows (no matching features).

    Checks that output contains the expected number of break points (usually zeros).
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
    # The breaks should be zeros since no data was classified
    assert len(breaks) == 4
    assert all(b == 0.0 for b in breaks)


def test_v_class_large_dataset(tmp_path):
    """
    Test classification on a larger dataset (1000 points).

    Verifies performance and correctness of v.class on bigger input with more classes.
    """
    project = tmp_path / "large_project"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        # Define larger computational region
        gs.run_command("g.region", n=100, s=0, e=100, w=0, res=1, env=session.env)

        # Generate 1000 points with values 1 to 1000
        points = "\n".join(f"{i} 1 {i}" for i in range(1, 1001))

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

        # Add 'value' attribute column
        gs.run_command(
            "v.db.addcolumn",
            map="large_points",
            columns="value double",
            env=session.env,
        )

        # Populate attribute table values
        for cat in range(1, 1001):
            gs.run_command(
                "db.execute",
                sql=f"UPDATE large_points SET value = {cat} WHERE cat = {cat}",
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
        breaks = [float(b) for b in output.strip().split(",")]
        assert len(breaks) == 9  # 10 classes → 9 breaks


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

    This algorithm may emit warnings about statistical significance,
    which are filtered out during testing for cleaner output.
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
    breaks = [float(b) for b in output.strip().split(",")]
    assert len(breaks) == 4
