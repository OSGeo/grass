"""Tests for the error-state ctypes bindings

datetime_error(), datetime_error_code() and datetime_error_msg() share a
single, process-global error state (static variables in error.c), not one
tied to any particular DateTime struct. No GRASS session is needed here, but
because the state is global, each test sets it explicitly rather than
assuming what a previous test may have left behind.
"""

from grass.lib import date as libdate


def test_error_sets_the_code_and_message() -> None:
    assert libdate.datetime_error(-5, b"custom message") == -5
    assert libdate.datetime_error_code() == -5
    assert libdate.datetime_error_msg() == b"custom message"


def test_clear_error_resets_the_code_and_message() -> None:
    libdate.datetime_error(-5, b"custom message")
    libdate.datetime_clear_error()
    assert libdate.datetime_error_code() == 0
    assert libdate.datetime_error_msg() == b""


def test_error_with_code_zero_clears_the_message() -> None:
    libdate.datetime_error(-5, b"custom message")
    libdate.datetime_error(0, None)
    assert libdate.datetime_error_code() == 0
    assert libdate.datetime_error_msg() == b""
