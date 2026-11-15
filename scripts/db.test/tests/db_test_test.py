import pytest

from grass.tools import Tools, ToolError


def test_run_without_database_connection(xy_empty_dataset_session):
    with Tools(session=xy_empty_dataset_session, consistent_return_value=True) as tools:
        # The assumption for the test is that the database connection was not established.
        # We test that state using db.connect directly before testing how db.test behaves
        # in that state.
        with pytest.raises(ToolError, match="Database connection not defined"):
            tools.db_connect(flags="p", format="json")
        with pytest.raises(ToolError, match="Database connection not defined"):
            tools.db_test(test="test1")


def test_run_with_check_or_connect_flag(xy_empty_dataset_session):
    with Tools(session=xy_empty_dataset_session, consistent_return_value=True) as tools:
        result = tools.db_connect(flags="c")
        result = tools.db_test(test="test1")
        assert "ERROR" not in result.stderr


def test_run_with_default_settings_flag(xy_empty_dataset_session):
    with Tools(session=xy_empty_dataset_session, consistent_return_value=True) as tools:
        result = tools.db_connect(flags="d")
        result = tools.db_test(test="test1")
        assert "ERROR" not in result.stderr


def test_run_with_sqlite(xy_empty_dataset_session):
    with Tools(session=xy_empty_dataset_session, consistent_return_value=True) as tools:
        result = tools.db_connect(driver="sqlite")
        result = tools.db_test(test="test1")
        assert "ERROR" not in result.stderr
