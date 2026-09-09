"""Tests for the timezone ctypes bindings

datetime_set_timezone(), datetime_change_timezone() and friends only read
and write fields of a DateTime struct already in memory, so no GRASS session
is needed here.
"""

from ctypes import byref, c_int, create_string_buffer

import pytest

from grass.lib import date as libdate


def scan(text):
    """Parse text with datetime_scan(), returning the resulting DateTime"""
    dt = libdate.DateTime()
    assert libdate.datetime_scan(byref(dt), text.encode()) == 0
    return dt


def format_datetime(dt):
    """Format a DateTime with datetime_format(), returning the text"""
    buf = create_string_buffer(128)
    assert libdate.datetime_format(byref(dt), buf) == 0
    return buf.value.decode()


@pytest.mark.parametrize(
    ("tz", "hours", "minutes"),
    [
        (330, 5, 30),
        # The sign is dropped: datetime_decompose_timezone() only reports
        # magnitude, leaving the caller to read the sign off dt.tz itself.
        (-330, 5, 30),
        (0, 0, 0),
    ],
)
def test_decompose_timezone(tz, hours, minutes) -> None:
    hour, minute = c_int(), c_int()
    libdate.datetime_decompose_timezone(tz, byref(hour), byref(minute))
    assert (hour.value, minute.value) == (hours, minutes)


@pytest.mark.parametrize(
    ("minutes", "valid"),
    [
        # -12:00 and +13:00 are the documented boundary values.
        (-720, True),
        (780, True),
        (-721, False),
        (781, False),
    ],
)
def test_is_valid_timezone_boundaries(minutes, valid) -> None:
    assert bool(libdate.datetime_is_valid_timezone(minutes)) == valid


def test_get_timezone_fails_when_none_is_set() -> None:
    dt = scan("15 aug 2001 12:30:45")
    minutes = c_int()
    assert libdate.datetime_get_timezone(byref(dt), byref(minutes)) != 0


def test_set_get_and_unset_timezone_round_trip() -> None:
    dt = scan("15 aug 2001 12:30:45")
    minutes = c_int()

    assert libdate.datetime_set_timezone(byref(dt), 330) == 0
    assert libdate.datetime_get_timezone(byref(dt), byref(minutes)) == 0
    assert minutes.value == 330

    assert libdate.datetime_unset_timezone(byref(dt)) == 0
    assert libdate.datetime_get_timezone(byref(dt), byref(minutes)) != 0


@pytest.mark.parametrize(
    ("text", "expected_clock"),
    [
        # 12:30:45 +05:30 is 07:00:45 in UTC, same calendar day.
        ("15 aug 2001 12:30:45 +0530", "15 Aug 2001 07:00:45"),
        # Shifting to UTC here crosses midnight into the previous day.
        ("1 jan 2000 00:30:00 +0530", "31 Dec 1999 19:00:00"),
    ],
)
def test_change_to_utc_shifts_the_clock(text, expected_clock) -> None:
    """datetime_change_to_utc() adjusts the clock fields correctly

    It does not, however, update dt.tz to 0 to match: the formatted output
    keeps showing the original offset even though the clock now reads the
    UTC time. This contradicts datetime_change_timezone()'s own doc comment
    ("... and set dt.tz = minutes"), but is harmless today because the only
    caller, datetime_difference(), discards the timezone field afterward and
    only reads the shifted clock fields.
    """
    dt = scan(text)
    original_tz = dt.tz

    assert libdate.datetime_change_to_utc(byref(dt)) == 0

    formatted = format_datetime(dt)
    assert formatted.startswith(expected_clock)
    assert dt.tz == original_tz


def test_change_timezone_rejects_an_invalid_target() -> None:
    dt = scan("15 aug 2001 12:30:45 +0530")
    assert libdate.datetime_change_timezone(byref(dt), 781) != 0
