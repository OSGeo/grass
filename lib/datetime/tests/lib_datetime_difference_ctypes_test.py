"""Tests for the datetime_difference() ctypes binding

datetime_difference() only reads two DateTime structs and writes a third, so
no GRASS session is needed here.
"""

from ctypes import byref, create_string_buffer

import pytest

from grass.lib import date as libdate


def scan(text):
    """Parse text with datetime_scan(), returning the resulting DateTime"""
    dt = libdate.DateTime()
    assert libdate.datetime_scan(byref(dt), text.encode()) == 0, text
    return dt


def format_datetime(dt):
    """Format a DateTime with datetime_format(), returning the text"""
    buf = create_string_buffer(128)
    assert libdate.datetime_format(byref(dt), buf) == 0
    return buf.value.decode()


def difference(a_text, b_text):
    """Compute a - b and return (return_code, formatted result)"""
    a = scan(a_text)
    b = scan(b_text)
    result = libdate.DateTime()
    ret = libdate.datetime_difference(byref(a), byref(b), byref(result))
    return ret, format_datetime(result)


@pytest.mark.parametrize(
    ("a", "b", "expected"),
    [
        ("20 aug 2001", "15 aug 2001", "5 days"),
        ("15 aug 2001", "15 aug 2001", "0 days"),
        # Crosses a month and year boundary.
        ("1 jan 2002", "25 dec 2001", "7 days"),
        # A time-of-day difference on the same calendar day.
        (
            "15 aug 2001 12:30:00",
            "15 aug 2001 10:00:00",
            "0 days 2 hours 30 minutes 0 seconds",
        ),
    ],
)
def test_difference_of_day_precision_dates(a, b, expected) -> None:
    ret, formatted = difference(a, b)
    assert ret == 0
    assert formatted == expected


def test_difference_is_negative_when_a_is_earlier_than_b() -> None:
    """The sign of the result reflects which operand is later

    The formatted output puts a space between the sign and the value
    ("- 5 days", not "-5 days"): datetime_format() writes the "-" first,
    then, since the buffer is already non-empty, prefixes the day field
    with a space the same way it would between any two fields.
    """
    ret, formatted = difference("15 aug 2001", "20 aug 2001")
    assert ret == 0
    assert formatted == "- 5 days"


@pytest.mark.parametrize(
    ("a", "b", "expected"),
    [
        ("mar 2001", "jan 2001", "0 years 2 months"),
        # Crossing a year boundary still measures forward in months.
        ("jan 2002", "nov 2001", "0 years 2 months"),
    ],
)
def test_difference_of_year_month_precision_dates(a, b, expected) -> None:
    ret, formatted = difference(a, b)
    assert ret == 0
    assert formatted == expected


def test_difference_accounts_for_timezone_offsets() -> None:
    """Both operands are converted to UTC before comparing clock fields"""
    # 12:30 +05:30 is 07:00 UTC; 12:30 +00:00 is 12:30 UTC, so a is earlier.
    ret, formatted = difference(
        "15 aug 2001 12:30:00 +0530", "15 aug 2001 12:30:00 +0000"
    )
    assert ret == 0
    assert formatted == "- 0 days 5 hours 30 minutes 0 seconds"


def test_difference_rejects_a_timezone_on_only_one_operand() -> None:
    a = scan("15 aug 2001 12:30:00 +0530")
    b = scan("15 aug 2001 10:00:00")
    result = libdate.DateTime()
    assert libdate.datetime_difference(byref(a), byref(b), byref(result)) != 0
