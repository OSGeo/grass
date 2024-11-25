import pytest
from grass.script import run_command, parse_command


@pytest.fixture(scope="module")
def setup_maps():
    """Set up a temporary region and generate test raster maps."""

    run_command("g.region", n=3, s=0, e=3, w=0, res=1)

    # 1 1 2
    # 1 2 3
    # 2 2 3
    run_command(
        "r.mapcalc",
        expression=(
            "map1 = "
            "if(row() == 1 && col() <= 2, 1, "
            "if(row() == 1 && col() == 3, 2, "
            "if(row() == 2 && col() == 1, 1, "
            "if(row() == 2 && col() == 2, 2, "
            "if(row() == 2 && col() == 3, 3, "
            "if(row() == 3 && col() <= 2, 2, 3))))))"
        ),
        overwrite=True,
    )

    # 1 2 2
    # 2 1 3
    # 3 3 3
    run_command(
        "r.mapcalc",
        expression=(
            "map2 = "
            "if(row() == 1 && col() == 1, 1, "
            "if(row() == 1 && col() >= 2, 2, "
            "if(row() == 2 && col() == 1, 2, "
            "if(row() == 2 && col() == 2, 1, "
            "if(row() == 2 && col() == 3, 3, "
            "if(row() >= 3, 3, null()))))))"
        ),
        overwrite=True,
    )
    yield

    run_command("g.remove", flags="f", type="raster", name=["map1", "map2"])


def test_r_coin(setup_maps):
    """Test the r.coin module."""

    coin_output = parse_command("r.coin", first="map1", second="map2", units="c")

    # Start parsing the output
    actual_results = []
    is_data_section = False

    for line in coin_output:
        # Detect the start of the data section
        if "|   cat# |" in line:
            is_data_section = True
            continue

        # Stop parsing after the data section ends
        if is_data_section and line.startswith("+"):
            break

        # Parse rows in the data section
        if is_data_section:
            columns = line.split("|")
            if len(columns) > 2:
                try:
                    # Extract categories and counts from the table
                    cat1 = int(columns[1].strip()[1:])  # Category from the row headr
                    val1 = int(columns[2].strip())  # Value in column 1
                    val2 = int(columns[3].strip())  # Value in column 2
                    val3 = int(columns[4].strip())  # Value in column 3

                    actual_results.extend(
                        [(cat1, 1, val1), (cat1, 2, val2), (cat1, 3, val3)]
                    )
                except ValueError:
                    # Ignore lines that cannot be parsed as numbers
                    pass

    expected_results = [
        (1, 1, 1),
        (1, 2, 1),
        (1, 3, 0),
        (2, 1, 2),
        (2, 2, 1),
        (2, 3, 0),
        (3, 1, 0),
        (3, 2, 2),
        (3, 3, 2),
    ]

    # Assert the parsed results against the expected results
    assert set(actual_results) == set(
        expected_results
    ), f"Expected {expected_results}, but got {actual_results}"
