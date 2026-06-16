"""Test t.vect.db.select functionality against expected outputs."""

from grass.tools import Tools


def test_t_vect_db_select_interval_default(space_time_vector_dataset):
    """Verify standard plain text layout for interval datasets."""
    tools = Tools(session=space_time_vector_dataset.session)

    result = tools.t_vect_db_select(input=space_time_vector_dataset.interval_name)
    assert result.returncode == 0

    expected_output = """start_time|end_time|cat|observation
2001-03-01 00:00:00|2001-04-01 00:00:00|1|100
2001-03-01 00:00:00|2001-04-01 00:00:00|2|100
2001-03-01 00:00:00|2001-04-01 00:00:00|3|100
2001-04-01 00:00:00|2001-05-01 00:00:00|1|200
2001-04-01 00:00:00|2001-05-01 00:00:00|2|200
2001-04-01 00:00:00|2001-05-01 00:00:00|3|200
2001-05-01 00:00:00|2001-06-01 00:00:00|1|300
2001-05-01 00:00:00|2001-06-01 00:00:00|2|300
2001-05-01 00:00:00|2001-06-01 00:00:00|3|300"""

    actual_lines = result.text_split("\n")
    expected_lines = expected_output.strip().splitlines()

    for actual, expected in zip(actual_lines, expected_lines, strict=True):
        assert actual == expected


def test_t_vect_db_select_interval_where(space_time_vector_dataset):
    """Verify filtering via attribute clauses with custom separators on intervals."""
    tools = Tools(session=space_time_vector_dataset.session)

    result = tools.t_vect_db_select(
        input=space_time_vector_dataset.interval_name,
        where="cat = 1",
        separator="  |  ",
    )
    assert result.returncode == 0

    expected_output = """start_time  |  end_time  |  cat  |  observation
2001-03-01 00:00:00  |  2001-04-01 00:00:00  |  1  |  100
2001-04-01 00:00:00  |  2001-05-01 00:00:00  |  1  |  200
2001-05-01 00:00:00  |  2001-06-01 00:00:00  |  1  |  300"""

    actual_lines = result.text_split("\n")
    expected_lines = expected_output.strip().splitlines()

    for actual, expected in zip(actual_lines, expected_lines, strict=True):
        assert actual == expected


def test_t_vect_db_select_interval_columns(space_time_vector_dataset):
    """Verify selective attribute column specification on intervals."""
    tools = Tools(session=space_time_vector_dataset.session)

    result = tools.t_vect_db_select(
        input=space_time_vector_dataset.interval_name,
        where="cat = 1",
        separator="  |  ",
        columns="observation",
    )
    assert result.returncode == 0

    expected_output = """start_time  |  end_time  |  observation
2001-03-01 00:00:00  |  2001-04-01 00:00:00  |  100
2001-04-01 00:00:00  |  2001-05-01 00:00:00  |  200
2001-05-01 00:00:00  |  2001-06-01 00:00:00  |  300"""

    actual_lines = result.text_split("\n")
    expected_lines = expected_output.strip().splitlines()

    for actual, expected in zip(actual_lines, expected_lines, strict=True):
        assert actual == expected


def test_t_vect_db_select_instance_default(space_time_vector_dataset):
    """Verify standard plain text layout for instance datasets."""
    tools = Tools(session=space_time_vector_dataset.session)

    result = tools.t_vect_db_select(input=space_time_vector_dataset.instance_name)
    assert result.returncode == 0

    expected_output = """start_time|end_time|cat|observation
2004-01-01 00:00:00||1|100
2004-01-01 00:00:00||2|100
2004-01-01 00:00:00||3|100
2004-04-01 00:00:00||1|200
2004-04-01 00:00:00||2|200
2004-04-01 00:00:00||3|200
2004-07-01 00:00:00||1|300
2004-07-01 00:00:00||2|300
2004-07-01 00:00:00||3|300"""

    actual_lines = result.text_split("\n")
    expected_lines = expected_output.strip().splitlines()

    for actual, expected in zip(actual_lines, expected_lines, strict=True):
        assert actual == expected


def test_t_vect_db_select_instance_where(space_time_vector_dataset):
    """Verify filtering via attribute clauses with custom separators on instances."""
    tools = Tools(session=space_time_vector_dataset.session)

    result = tools.t_vect_db_select(
        input=space_time_vector_dataset.instance_name,
        where="cat = 1",
        separator="  |  ",
    )
    assert result.returncode == 0

    expected_output = """start_time  |  end_time  |  cat  |  observation
2004-01-01 00:00:00  |    |  1  |  100
2004-04-01 00:00:00  |    |  1  |  200
2004-07-01 00:00:00  |    |  1  |  300"""

    actual_lines = result.text_split("\n")
    expected_lines = expected_output.strip().splitlines()

    for actual, expected in zip(actual_lines, expected_lines, strict=True):
        assert actual == expected
