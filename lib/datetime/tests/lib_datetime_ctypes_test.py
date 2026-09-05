"""Tests for scanning and formatting through the datetime library ctypes bindings

datetime_scan and datetime_format are pure string parsing and formatting: they
read or write a DateTime struct and touch nothing outside it, so no GRASS
session is needed here.
"""

from ctypes import byref, create_string_buffer

import pytest

from grass.lib import date as libdate

# The size of the buffer passed to datetime_format(), matching the temp
# buffers GRASS's own callers use (e.g. G_format_timestamp() in lib/gis).
FORMAT_BUFFER_SIZE = 128


def scan(text):
    """Parse text with datetime_scan(), returning (return_code, DateTime)"""
    dt = libdate.DateTime()
    ret = libdate.datetime_scan(byref(dt), text.encode())
    return ret, dt


def format_datetime(dt):
    """Format a DateTime with datetime_format(), returning (return_code, text)"""
    buf = create_string_buffer(FORMAT_BUFFER_SIZE)
    ret = libdate.datetime_format(byref(dt), buf)
    return ret, buf.value.decode()


def round_trip(text):
    """Scan text and format the result back, asserting the scan succeeded"""
    ret, dt = scan(text)
    assert ret == 0, f"datetime_scan rejected {text!r}"
    ret, formatted = format_datetime(dt)
    assert ret == 0, f"datetime_format rejected the result of scanning {text!r}"
    return formatted


@pytest.mark.parametrize(
    ("text", "expected"),
    [
        # A bare day/month/year: the coarsest granularity that is still a
        # full date.
        ("15 aug 2001", "15 Aug 2001"),
        # A time of day extends the granularity down to seconds.
        ("15 aug 2001 12:30:45", "15 Aug 2001 12:30:45"),
        # A BC year is round-tripped with a "bc" suffix, not a negative year.
        ("44 bc", "44 bc"),
        # Midnight and noon are the two hours where a naive 24h-to-12h
        # conversion breaks (0 % 12 == 12 % 12 == 0); this locks in that
        # GRASS's own format keeps them as plain 00:00 and 12:00.
        ("1 jan 2000 00:00", "1 Jan 2000 00:00"),
        ("1 jan 2000 12:00", "1 Jan 2000 12:00"),
        # A relative interval combining several units.
        ("2 years 3 months", "2 years 3 months"),
        # A timezone offset round-trips exactly, including the two
        # boundary values datetime_is_valid_timezone() allows: -12:00 and
        # +13:00.
        ("15 aug 2001 12:30:45 +0530", "15 Aug 2001 12:30:45 +0530"),
        ("1 jan 2000 00:00 +1300", "1 Jan 2000 00:00 +1300"),
        ("1 jan 2000 00:00 -1200", "1 Jan 2000 00:00 -1200"),
    ],
)
def test_scan_format_round_trip(text, expected) -> None:
    assert round_trip(text) == expected


def test_fractional_seconds_are_not_zero_padded() -> None:
    """A fractional second under 10 is not padded to two digits

    datetime_format() builds the seconds field with the printf-style spec
    "%02.*f", which pads the *whole* formatted number to a minimum width of
    2; since "1.500" is already 5 characters, no leading zero is added. This
    looks like a formatting bug at a glance, so it is worth locking in as
    the current, apparently intentional behavior.
    """
    assert round_trip("1 jan 2000 00:00:01.500") == "1 Jan 2000 00:00:1.500"


def test_microsecond_precision_is_preserved() -> None:
    assert round_trip("1 jan 2000 00:00:12.123456") == "1 Jan 2000 00:00:12.123456"


@pytest.mark.parametrize(
    "text",
    [
        "",
        "not a date",
        "2000-01-01",
        # A day that does not exist in the given month.
        "32 jan 2000",
        # 12-hour clock input is rejected outright rather than silently
        # misread, e.g. treating "2:30 pm" as 2:30 in the morning.
        "2:30 pm",
        "02:30 PM",
        # One minute past either end of the valid timezone range.
        "1 jan 2000 00:00 +1301",
        "1 jan 2000 00:00 -1201",
    ],
)
def test_scan_rejects_invalid_input(text) -> None:
    ret, _dt = scan(text)
    assert ret != 0
