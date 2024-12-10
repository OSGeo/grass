import grass.script as gs


def validate_r_coin_output(actual_results, expected_results):
    """Validate r.coin output against expected results."""
    assert set(actual_results) == set(
        expected_results
    ), f"Expected {expected_results}, but got {actual_results}"


def test_r_coin(setup_maps):
    """Test the r.coin module."""
    session = setup_maps
    coin_output = gs.parse_command(
        "r.coin", first="map1", second="map2", units="c", env=session.env
    )

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
                    cat1 = int(columns[1].strip()[1:])  # Category from the row header
                    val1 = int(columns[2].strip())  # Value in column 1
                    val2 = int(columns[3].strip())  # Value in column 2
                    val3 = int(columns[4].strip())  # Value in column 3

                    actual_results.extend(
                        [(cat1, 1, val1), (cat1, 2, val2), (cat1, 3, val3)]
                    )
                except ValueError:
                    pass  # Ignore lines that cannot be parsed as numbers

    # Expected results
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

    # Validate results
    validate_r_coin_output(actual_results, expected_results)
