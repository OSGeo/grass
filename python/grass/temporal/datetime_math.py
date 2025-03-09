"""
Functions for mathematical datetime operations

(C) 2011-2024 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

from __future__ import annotations

import copy
from datetime import datetime, timedelta
from typing import TypedDict

from .core import get_tgis_message_interface

try:
    from dateutil import parser

    has_dateutil = True
except (ImportError, ModuleNotFoundError):
    has_dateutil = False


DAY_IN_SECONDS = 86400
SECOND_AS_DAY = 1.1574074074074073e-05


def relative_time_to_time_delta(value: float) -> timedelta:
    """Convert the double value representing days into a timedelta object."""
    days = int(value)
    seconds = value % 1
    seconds = round(seconds * DAY_IN_SECONDS)
    return timedelta(days, seconds)


def time_delta_to_relative_time(delta: timedelta) -> float:
    """Convert the time delta into a double value, representing days."""
    return float(delta.days) + float(delta.seconds * SECOND_AS_DAY)


def relative_time_to_time_delta_seconds(value: float) -> timedelta:
    """Convert the double value representing seconds into a timedelta object."""
    days = value / 86400
    seconds = int(value % 86400)
    return timedelta(days, seconds)


def time_delta_to_relative_time_seconds(delta: timedelta) -> float:
    """Convert the time delta into a double value, representing seconds."""

    return float(delta.days * DAY_IN_SECONDS) + float(delta.seconds)


###############################################################################


def decrement_datetime_by_string(
    mydate: datetime, increment: str, mult=1
) -> datetime | None:
    """Return a new datetime object decremented with the provided
    relative dates specified as string.
    Additional a multiplier can be specified to multiply the increment
    before adding to the provided datetime object.

    Usage:

    .. code-block:: python

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "31 days"
         >>> decrement_datetime_by_string(dt, string)
         datetime.datetime(2000, 12, 1, 0, 0)

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "1 month"
         >>> decrement_datetime_by_string(dt, string)
         datetime.datetime(2000, 12, 1, 0, 0)

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "2 month"
         >>> decrement_datetime_by_string(dt, string)
         datetime.datetime(2000, 11, 1, 0, 0)

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "24 months"
         >>> decrement_datetime_by_string(dt, string)
         datetime.datetime(1999, 1, 1, 0, 0)

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "48 months"
         >>> decrement_datetime_by_string(dt, string)
         datetime.datetime(1997, 1, 1, 0, 0)

         >>> dt = datetime(2001, 6, 1, 0, 0, 0)
         >>> string = "5 months"
         >>> decrement_datetime_by_string(dt, string)
         datetime.datetime(2001, 1, 1, 0, 0)

         >>> dt = datetime(2001, 6, 1, 0, 0, 0)
         >>> string = "7 months"
         >>> decrement_datetime_by_string(dt, string)
         datetime.datetime(2000, 11, 1, 0, 0)

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "1 year"
         >>> decrement_datetime_by_string(dt, string)
         datetime.datetime(2000, 1, 1, 0, 0)


    :param mydate: A datetime object to incremented
    :param increment: A string providing increment information:
                      The string may include comma separated values of type
                      seconds, minutes, hours, days, weeks, months and years
                      Example: Increment the datetime 2001-01-01 00:00:00
                      with "60 seconds, 4 minutes, 12 hours, 10 days,
                      1 weeks, 5 months, 1 years" will result in the
                      datetime 2003-02-18 12:05:00
    :param mult: A multiplier, default is 1
    :return: The new datetime object or none in case of an error
    """
    return modify_datetime_by_string(mydate, increment, mult, sign=-1)


def increment_datetime_by_string(
    mydate: datetime, increment: str, mult=1
) -> datetime | None:
    """Return a new datetime object incremented with the provided
    relative dates specified as string.
    Additional a multiplier can be specified to multiply the increment
    before adding to the provided datetime object.

    Usage:

    .. code-block:: python

         >>> dt = datetime(2001, 9, 1, 0, 0, 0)
         >>> string = (
         ...     "60 seconds, 4 minutes, 12 hours, 10 days, 1 weeks, 5 months, 1 years"
         ... )
         >>> increment_datetime_by_string(dt, string)
         datetime.datetime(2003, 2, 18, 12, 5)

         >>> dt = datetime(2001, 11, 1, 0, 0, 0)
         >>> string = "1 months"
         >>> increment_datetime_by_string(dt, string)
         datetime.datetime(2001, 12, 1, 0, 0)

         >>> dt = datetime(2001, 11, 1, 0, 0, 0)
         >>> string = "13 months"
         >>> increment_datetime_by_string(dt, string)
         datetime.datetime(2002, 12, 1, 0, 0)

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "72 months"
         >>> increment_datetime_by_string(dt, string)
         datetime.datetime(2007, 1, 1, 0, 0)

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "72 months"
         >>> increment_datetime_by_string(dt, string)
         datetime.datetime(2007, 1, 1, 0, 0)

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "5 minutes"
         >>> increment_datetime_by_string(dt, string)
         datetime.datetime(2001, 1, 1, 0, 5)

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "49 hours"
         >>> increment_datetime_by_string(dt, string)
         datetime.datetime(2001, 1, 3, 1, 0)

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "3600 seconds"
         >>> increment_datetime_by_string(dt, string)
         datetime.datetime(2001, 1, 1, 1, 0)

         >>> dt = datetime(2001, 1, 1, 0, 0, 0)
         >>> string = "30 days"
         >>> increment_datetime_by_string(dt, string)
         datetime.datetime(2001, 1, 31, 0, 0)


    :param mydate: A datetime object to incremented
    :param increment: A string providing increment information:
                      The string may include comma separated values of type
                      seconds, minutes, hours, days, weeks, months and years
                      Example: Increment the datetime 2001-01-01 00:00:00
                      with "60 seconds, 4 minutes, 12 hours, 10 days,
                      1 weeks, 5 months, 1 years" will result in the
                      datetime 2003-02-18 12:05:00
    :param mult: A multiplier, default is 1
    :return: The new datetime object or none in case of an error
    """
    return modify_datetime_by_string(mydate, increment, mult, sign=1)


def modify_datetime_by_string(
    mydate: datetime, increment: str, mult=1, sign: int = 1
) -> datetime | None:
    """Return a new datetime object incremented with the provided
    relative dates specified as string.
    Additional a multiplier can be specified to multiply the increment
    before adding to the provided datetime object.

    :param mydate: A datetime object to incremented
    :param increment: A string providing increment information:
                      The string may include comma separated values of type
                      seconds, minutes, hours, days, weeks, months and years
                      Example: Increment the datetime 2001-01-01 00:00:00
                      with "60 seconds, 4 minutes, 12 hours, 10 days,
                      1 weeks, 5 months, 1 years" will result in the
                      datetime 2003-02-18 12:05:00
    :param mult: A multiplier, default is 1
    :param sign: Choose 1 for positive sign (incrementing) or -1 for
                 negative sign (decrementing).
    :return: The new datetime object or none in case of an error
    """
    sign = int(sign)
    if sign not in {1, -1}:
        return None

    if increment:
        seconds = 0
        minutes = 0
        hours = 0
        days = 0
        weeks = 0
        months = 0
        years = 0

        inclist = []
        # Split the increment string
        incparts = increment.split(",")
        for incpart in incparts:
            inclist.append(incpart.strip().split(" "))

        for inc in inclist:
            msgr = get_tgis_message_interface()
            if len(inc) < 2:
                msgr.error(_("Wrong increment format: %s") % (increment))
                return None
            if inc[1].find("seconds") >= 0 or inc[1].find("second") >= 0:
                seconds = sign * mult * int(inc[0])
            elif inc[1].find("minutes") >= 0 or inc[1].find("minute") >= 0:
                minutes = sign * mult * int(inc[0])
            elif inc[1].find("hours") >= 0 or inc[1].find("hour") >= 0:
                hours = sign * mult * int(inc[0])
            elif inc[1].find("days") >= 0 or inc[1].find("day") >= 0:
                days = sign * mult * int(inc[0])
            elif inc[1].find("weeks") >= 0 or inc[1].find("week") >= 0:
                weeks = sign * mult * int(inc[0])
            elif inc[1].find("months") >= 0 or inc[1].find("month") >= 0:
                months = sign * mult * int(inc[0])
            elif inc[1].find("years") >= 0 or inc[1].find("year") >= 0:
                years = sign * mult * int(inc[0])
            else:
                msgr.error(_("Wrong increment format: %s") % (increment))
                return None

        return modify_datetime(
            mydate, years, months, weeks, days, hours, minutes, seconds
        )

    return mydate


###############################################################################


def modify_datetime(
    mydate: datetime, years=0, months=0, weeks=0, days=0, hours=0, minutes=0, seconds=0
) -> datetime:
    """Return a new datetime object incremented with the provided
    relative dates and times"""

    tdelta_seconds = timedelta(seconds=seconds)
    tdelta_minutes = timedelta(minutes=minutes)
    tdelta_hours = timedelta(hours=hours)
    tdelta_days = timedelta(days=days)
    tdelta_weeks = timedelta(weeks=weeks)
    tdelta_months = timedelta(0)
    tdelta_years = timedelta(0)

    if months > 0:
        # Compute the actual number of days in the month to add as timedelta
        year = mydate.year
        month = mydate.month

        all_months = int(months) + int(month)
        years_to_add = int(all_months / 12.001)
        residual_months = all_months - (years_to_add * 12)

        # Make a deep copy of the datetime object
        dt1 = copy.copy(mydate)

        # Make sure the month starts with a 1
        if residual_months == 0:
            residual_months = 1

        dt1 = dt1.replace(year=year + years_to_add, month=residual_months)

        tdelta_months = dt1 - mydate
    elif months < 0:
        # Compute the actual number of days in the month to add as timedelta
        year = mydate.year
        month = mydate.month
        years_to_remove = 0

        all_months = int(months) + int(month)
        if all_months <= 0:
            years_to_remove = abs(int(all_months / 12.001))
            residual_months = all_months + (years_to_remove * 12)
            years_to_remove += 1
        else:
            residual_months = all_months

        # Make a deep copy of the datetime object
        dt1 = copy.copy(mydate)

        # Correct the months
        if residual_months <= 0:
            residual_months += 12

        dt1 = dt1.replace(year=year - years_to_remove, month=residual_months)

        tdelta_months = dt1 - mydate

    if years != 0:
        # Make a deep copy of the datetime object
        dt1 = copy.copy(mydate)
        # Compute the number of days
        dt1 = dt1.replace(year=mydate.year + int(years))
        tdelta_years = dt1 - mydate

    return (
        mydate
        + tdelta_seconds
        + tdelta_minutes
        + tdelta_hours
        + tdelta_days
        + tdelta_weeks
        + tdelta_months
        + tdelta_years
    )


###############################################################################


def adjust_datetime_to_granularity(mydate: datetime, granularity):
    """Modify the datetime object to fit the given granularity

    - Years will start at the first of January
    - Months will start at the first day of the month
    - Days will start at the first Hour of the day
    - Hours will start at the first minute of an hour
    - Minutes will start at the first second of a minute

    Usage:

    .. code-block:: python

        >>> dt = datetime(2001, 8, 8, 12, 30, 30)
        >>> adjust_datetime_to_granularity(dt, "5 seconds")
        datetime.datetime(2001, 8, 8, 12, 30, 30)

        >>> adjust_datetime_to_granularity(dt, "20 minutes")
        datetime.datetime(2001, 8, 8, 12, 30)

        >>> adjust_datetime_to_granularity(dt, "20 minutes")
        datetime.datetime(2001, 8, 8, 12, 30)

        >>> adjust_datetime_to_granularity(dt, "3 hours")
        datetime.datetime(2001, 8, 8, 12, 0)

        >>> adjust_datetime_to_granularity(dt, "5 days")
        datetime.datetime(2001, 8, 8, 0, 0)

        >>> adjust_datetime_to_granularity(dt, "2 weeks")
        datetime.datetime(2001, 8, 6, 0, 0)

        >>> adjust_datetime_to_granularity(dt, "6 months")
        datetime.datetime(2001, 8, 1, 0, 0)

        >>> adjust_datetime_to_granularity(dt, "2 years")
        datetime.datetime(2001, 1, 1, 0, 0)

        >>> adjust_datetime_to_granularity(
        ...     dt, "2 years, 3 months, 5 days, 3 hours, 3 minutes, 2 seconds"
        ... )
        datetime.datetime(2001, 8, 8, 12, 30, 30)

        >>> adjust_datetime_to_granularity(dt, "3 months, 5 days, 3 minutes")
        datetime.datetime(2001, 8, 8, 12, 30)

        >>> adjust_datetime_to_granularity(dt, "3 weeks, 5 days")
        datetime.datetime(2001, 8, 8, 0, 0)

    """

    if granularity:
        has_seconds = False
        has_minutes = False
        has_hours = False
        has_days = False
        has_weeks = False
        has_months = False
        has_years = False

        seconds = mydate.second
        minutes = mydate.minute
        hours = mydate.hour
        days = mydate.day
        weekday = mydate.weekday()
        months = mydate.month
        years = mydate.year

        granlist = []
        # Split the increment string
        granparts = granularity.split(",")
        for granpart in granparts:
            granlist.append(granpart.strip().split(" "))

        for inc in granlist:
            if inc[1].find("seconds") >= 0 or inc[1].find("second") >= 0:
                has_seconds = True
            elif inc[1].find("minutes") >= 0 or inc[1].find("minute") >= 0:
                has_minutes = True
            elif inc[1].find("hours") >= 0 or inc[1].find("hour") >= 0:
                has_hours = True
            elif inc[1].find("days") >= 0 or inc[1].find("day") >= 0:
                has_days = True
            elif inc[1].find("weeks") >= 0 or inc[1].find("week") >= 0:
                has_weeks = True
            elif inc[1].find("months") >= 0 or inc[1].find("month") >= 0:
                has_months = True
            elif inc[1].find("years") >= 0 or inc[1].find("year") >= 0:
                has_years = True
            else:
                msgr = get_tgis_message_interface()
                msgr.error(_("Wrong granularity format: %s") % (granularity))
                return None

        if has_seconds:
            pass
        elif has_minutes:  # Start at 0 seconds
            seconds = 0
        elif has_hours:  # Start at 0 minutes and seconds
            seconds = 0
            minutes = 0
        elif has_days:  # Start at 0 hours, minutes and seconds
            seconds = 0
            minutes = 0
            hours = 0
        elif has_weeks:  # Start at the first day of the week (Monday) at 00:00:00
            seconds = 0
            minutes = 0
            hours = 0
            if days > weekday:
                days -= weekday  # this needs to be fixed
            else:
                days += weekday  # this needs to be fixed
        elif has_months:  # Start at the first day of the month at 00:00:00
            seconds = 0
            minutes = 0
            hours = 0
            days = 1
        elif has_years:  # Start at the first day of the first month at 00:00:00
            seconds = 0
            minutes = 0
            hours = 0
            days = 1
            months = 1

        dt = copy.copy(mydate)
        return dt.replace(
            year=years,
            month=months,
            day=days,
            hour=hours,
            minute=minutes,
            second=seconds,
        )


###############################################################################


class datetime_delta(TypedDict):
    """Typed dictionary to return the accumulated delta in year, month, day,
    hour, minute and second as well as max_days. At runtime, it is a plain dict."""

    year: int
    month: int
    day: int
    hour: int
    minute: int
    second: int
    max_days: int


def compute_datetime_delta(start: datetime, end: datetime) -> datetime_delta:
    """Return a dictionary with the accumulated delta in year, month, day,
    hour, minute and second

     Usage:

     .. code-block:: python

         >>> start = datetime(2001, 1, 1, 0, 0, 0)
         >>> end = datetime(2001, 1, 1, 0, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 0, 'hour': 0, 'minute': 0, 'second': 0, 'max_days': 0}

         >>> start = datetime(2001, 1, 1, 0, 0, 14)
         >>> end = datetime(2001, 1, 1, 0, 0, 44)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 0, 'hour': 0, 'minute': 0, 'second': 30, 'max_days': 0}

         >>> start = datetime(2001, 1, 1, 0, 0, 44)
         >>> end = datetime(2001, 1, 1, 0, 1, 14)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 0, 'hour': 0, 'minute': 1, 'second': 30, 'max_days': 0}

         >>> start = datetime(2001, 1, 1, 0, 0, 30)
         >>> end = datetime(2001, 1, 1, 0, 5, 30)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 0, 'hour': 0, 'minute': 5, 'second': 300, 'max_days': 0}

         >>> start = datetime(2001, 1, 1, 0, 0, 0)
         >>> end = datetime(2001, 1, 1, 0, 1, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 0, 'hour': 0, 'minute': 1, 'second': 0, 'max_days': 0}

         >>> start = datetime(2011, 10, 31, 0, 45, 0)
         >>> end = datetime(2011, 10, 31, 1, 45, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 0, 'hour': 1, 'minute': 60, 'second': 0, 'max_days': 0}

         >>> start = datetime(2011, 10, 31, 0, 45, 0)
         >>> end = datetime(2011, 10, 31, 1, 15, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 0, 'hour': 1, 'minute': 30, 'second': 0, 'max_days': 0}

         >>> start = datetime(2011, 10, 31, 0, 45, 0)
         >>> end = datetime(2011, 10, 31, 12, 15, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 0, 'hour': 12, 'minute': 690, 'second': 0, 'max_days': 0}

         >>> start = datetime(2011, 10, 31, 0, 0, 0)
         >>> end = datetime(2011, 10, 31, 1, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 0, 'hour': 1, 'minute': 0, 'second': 0, 'max_days': 0}

         >>> start = datetime(2011, 10, 31, 0, 0, 0)
         >>> end = datetime(2011, 11, 1, 1, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 1, 'hour': 25, 'minute': 0, 'second': 0, 'max_days': 1}

         >>> start = datetime(2011, 10, 31, 12, 0, 0)
         >>> end = datetime(2011, 11, 1, 6, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 0, 'hour': 18, 'minute': 0, 'second': 0, 'max_days': 0}

         >>> start = datetime(2011, 11, 1, 0, 0, 0)
         >>> end = datetime(2011, 12, 1, 1, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 1, 'day': 0, 'hour': 721, 'minute': 0, 'second': 0, 'max_days': 30}

         >>> start = datetime(2011, 11, 1, 0, 0, 0)
         >>> end = datetime(2011, 11, 5, 0, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 4, 'hour': 0, 'minute': 0, 'second': 0, 'max_days': 4}

         >>> start = datetime(2011, 10, 6, 0, 0, 0)
         >>> end = datetime(2011, 11, 5, 0, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 0, 'day': 30, 'hour': 0, 'minute': 0, 'second': 0, 'max_days': 30}

         >>> start = datetime(2011, 12, 2, 0, 0, 0)
         >>> end = datetime(2012, 1, 1, 0, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 1, 'month': 0, 'day': 30, 'hour': 0, 'minute': 0, 'second': 0, 'max_days': 30}

         >>> start = datetime(2011, 1, 1, 0, 0, 0)
         >>> end = datetime(2011, 2, 1, 0, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 0, 'month': 1, 'day': 0, 'hour': 0, 'minute': 0, 'second': 0, 'max_days': 31}

         >>> start = datetime(2011, 12, 1, 0, 0, 0)
         >>> end = datetime(2012, 1, 1, 0, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 1, 'month': 1, 'day': 0, 'hour': 0, 'minute': 0, 'second': 0, 'max_days': 31}

         >>> start = datetime(2011, 12, 1, 0, 0, 0)
         >>> end = datetime(2012, 6, 1, 0, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 1, 'month': 6, 'day': 0, 'hour': 0, 'minute': 0, 'second': 0, 'max_days': 183}

         >>> start = datetime(2011, 6, 1, 0, 0, 0)
         >>> end = datetime(2021, 6, 1, 0, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 10, 'month': 120, 'day': 0, 'hour': 0, 'minute': 0, 'second': 0, 'max_days': 3653}

         >>> start = datetime(2011, 6, 1, 0, 0, 0)
         >>> end = datetime(2012, 6, 1, 12, 0, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 1, 'month': 12, 'day': 0, 'hour': 8796, 'minute': 0, 'second': 0, 'max_days': 366}

         >>> start = datetime(2011, 6, 1, 0, 0, 0)
         >>> end = datetime(2012, 6, 1, 12, 30, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 1, 'month': 12, 'day': 0, 'hour': 8796, 'minute': 527790, 'second': 0, 'max_days': 366}

         >>> start = datetime(2011, 6, 1, 0, 0, 0)
         >>> end = datetime(2012, 6, 1, 12, 0, 5)
         >>> compute_datetime_delta(start, end)
         {'year': 1, 'month': 12, 'day': 0, 'hour': 8796, 'minute': 0, 'second': 31665605, 'max_days': 366}

         >>> start = datetime(2011, 6, 1, 0, 0, 0)
         >>> end = datetime(2012, 6, 1, 0, 30, 0)
         >>> compute_datetime_delta(start, end)
         {'year': 1, 'month': 12, 'day': 0, 'hour': 0, 'minute': 527070, 'second': 0, 'max_days': 366}

    :return: A dictionary with year, month, day, hour, minute, second and max_days as keys()
    """  # noqa: E501
    # TODO: set default values here, and ensure processing below covers all situations,
    # not leaking these default values
    comp = datetime_delta(
        year=0,
        month=0,
        day=0,
        hour=0,
        minute=0,
        second=0,
        max_days=(end - start).days,
    )

    day_diff = comp["max_days"]

    # Date
    # Count full years
    comp["year"] = end.year - start.year

    # Count full months
    if start.month == 1 and end.month == 1:
        comp["month"] = 0
    elif start.day == 1 and end.day == 1:
        d = end.month - start.month
        if d < 0:
            d += 12 * comp["year"]
        elif d == 0:
            d = 12 * comp["year"]
        comp["month"] = d

    # Count full days
    comp["day"] = 0 if start.day == 1 and end.day == 1 else day_diff

    # Time
    # Hours
    if start.hour == 0 and end.hour == 0:
        comp["hour"] = 0
    else:
        d = end.hour - start.hour
        if d < 0:
            d += 24 + 24 * day_diff
        else:
            d += 24 * day_diff
        comp["hour"] = d

    # Minutes
    if start.minute == 0 and end.minute == 0:
        comp["minute"] = 0
    else:
        d = end.minute - start.minute
        if d != 0:
            if comp["hour"]:
                d += 60 * comp["hour"]
            else:
                d += 24 * 60 * day_diff
        elif d == 0:
            d = 60 * comp["hour"] if comp["hour"] else 24 * 60 * day_diff

        comp["minute"] = d

    # Seconds
    if start.second == 0 and end.second == 0:
        comp["second"] = 0
    else:
        d = end.second - start.second
        if d != 0:
            if comp["minute"]:
                d += 60 * comp["minute"]
            elif comp["hour"]:
                d += 3600 * comp["hour"]
            else:
                d += 24 * 60 * 60 * day_diff
        elif d == 0:
            if comp["minute"]:
                d = 60 * comp["minute"]
            elif comp["hour"]:
                d = 3600 * comp["hour"]
            else:
                d = 24 * 60 * 60 * day_diff
        comp["second"] = d

    return comp


def check_datetime_string(time_string: str, use_dateutil: bool = True):
    """Check if  a string can be converted into a datetime object and return the object

    In case dateutil is not installed the supported ISO string formats are:

    - YYYY-mm-dd
    - YYYY-mm-dd HH:MM:SS
    - YYYY-mm-ddTHH:MM:SS
    - YYYY-mm-dd HH:MM:SS.s
    - YYYY-mm-ddTHH:MM:SS.s

    Time zones are not supported

    If dateutil is installed, all string formats of the dateutil module
    are supported, as well as time zones
    Time zones are not supported

    :param time_string: The time string to be checked for conversion
    :param use_dateutil: Use dateutil if available for datetime string parsing
    :return: datetime: object or an error message string in case of an error

    >>> s = "2000-01-01"
    >>> check_datetime_string(s)
    datetime.datetime(2000, 1, 1, 0, 0)
    >>> s = "2000-01-01T10:00:00"
    >>> check_datetime_string(s)
    datetime.datetime(2000, 1, 1, 10, 0)
    >>> s = "2000-01-01 10:00:00"
    >>> check_datetime_string(s)
    datetime.datetime(2000, 1, 1, 10, 0)
    >>> s = "2000-01-01T10:00:00.000001"
    >>> check_datetime_string(s)
    datetime.datetime(2000, 1, 1, 10, 0, 0, 1)
    >>> s = "2000-01-01 10:00:00.000001"
    >>> check_datetime_string(s)
    datetime.datetime(2000, 1, 1, 10, 0, 0, 1)

    # using native implementation, ignoring dateutil
    >>> s = "2000-01-01"
    >>> check_datetime_string(s, False)
    datetime.datetime(2000, 1, 1, 0, 0)
    >>> s = "2000-01-01T10:00:00"
    >>> check_datetime_string(s, False)
    datetime.datetime(2000, 1, 1, 10, 0)
    >>> s = "2000-01-01 10:00:00"
    >>> check_datetime_string(s, False)
    datetime.datetime(2000, 1, 1, 10, 0)
    >>> s = "2000-01-01T10:00:00.000001"
    >>> check_datetime_string(s, False)
    datetime.datetime(2000, 1, 1, 10, 0, 0, 1)
    >>> s = "2000-01-01 10:00:00.000001"
    >>> check_datetime_string(s, False)
    datetime.datetime(2000, 1, 1, 10, 0, 0, 1)

    """

    global has_dateutil

    if has_dateutil and use_dateutil is True:
        # First check if there is only a single number, which specifies
        # relative time. dateutil will interpret a single number as a valid
        # time string, so we have to catch this case beforehand
        try:
            int(time_string)
            return _("Time string seems to specify relative time")
        except ValueError:
            pass

        try:
            time_object = parser.parse(time_string)
        except Exception as inst:
            time_object = str(inst)
        return time_object

    # BC is not supported
    if "bc" in time_string:
        return _("Dates Before Christ (BC) are not supported")

    # Time zones are not supported
    if "+" in time_string:
        return _("Time zones are not supported")

    if ":" in time_string or "T" in time_string:
        # Check for microseconds
        if "." in time_string:
            if "T" in time_string:
                time_format = "%Y-%m-%dT%H:%M:%S.%f"
            else:
                time_format = "%Y-%m-%d %H:%M:%S.%f"
        else:  # noqa: PLR5501
            if "T" in time_string:
                time_format = "%Y-%m-%dT%H:%M:%S"
            else:
                time_format = "%Y-%m-%d %H:%M:%S"
    else:
        time_format = "%Y-%m-%d"

    try:
        return datetime.strptime(time_string, time_format)
    except (ValueError, TypeError):
        return _("Unable to parse time string: %s") % time_string


def string_to_datetime(time_string: str) -> datetime | None:
    """Convert a string into a datetime object

    In case datutil is not installed the supported ISO string formats are:

    - YYYY-mm-dd
    - YYYY-mm-dd HH:MM:SS
    - YYYY-mm-ddTHH:MM:SS
    - YYYY-mm-dd HH:MM:SS.s
    - YYYY-mm-ddTHH:MM:SS.s

    Time zones are not supported

    If dateutil is installed, all string formats of the dateutil module
    are supported, as well as time zones

    :param time_string: The time string to convert
    :return: datetime object or None in case the string
             could not be converted
    """

    if not isinstance(time_string, str):
        return None

    time_object = check_datetime_string(time_string)
    if not isinstance(time_object, datetime):
        msgr = get_tgis_message_interface()
        msgr.error(str(time_object))
        return None

    return time_object


def datetime_to_grass_datetime_string(dt: datetime | None) -> str:
    """Convert a python datetime object into a GRASS datetime string

    .. code-block:: python

        >>> import grass.temporal as tgis
        >>> import dateutil.parser as parser
        >>> dt = parser.parse("2011-01-01 10:00:00 +01:30")
        >>> tgis.datetime_to_grass_datetime_string(dt)
        '01 jan 2011 10:00:00 +0090'
        >>> dt = parser.parse("2011-01-01 10:00:00 +02:30")
        >>> tgis.datetime_to_grass_datetime_string(dt)
        '01 jan 2011 10:00:00 +0150'
        >>> dt = parser.parse("2011-01-01 10:00:00 +12:00")
        >>> tgis.datetime_to_grass_datetime_string(dt)
        '01 jan 2011 10:00:00 +0720'
        >>> dt = parser.parse("2011-01-01 10:00:00 -01:30")
        >>> tgis.datetime_to_grass_datetime_string(dt)
        '01 jan 2011 10:00:00 -0090'

    """
    # GRASS datetime month names
    month_names = [
        "",
        "jan",
        "feb",
        "mar",
        "apr",
        "may",
        "jun",
        "jul",
        "aug",
        "sep",
        "oct",
        "nov",
        "dec",
    ]

    if dt is None:
        msg = "Empty datetime object in datetime_to_grass_datetime_string"
        raise Exception(msg)

    # Check for time zone info in the datetime object
    if dt.tzinfo is not None:
        tz = dt.tzinfo.utcoffset(0)
        tz = (tz.seconds - 86400) / 60 if tz.seconds > 86400 / 2 else tz.seconds / 60

        string = "%.2i %s %.2i %.2i:%.2i:%.2i %+.4i" % (
            dt.day,
            month_names[dt.month],
            dt.year,
            dt.hour,
            dt.minute,
            dt.second,
            tz,
        )
    else:
        string = "%.2i %s %.4i %.2i:%.2i:%.2i" % (
            dt.day,
            month_names[dt.month],
            dt.year,
            dt.hour,
            dt.minute,
            dt.second,
        )

    return string


###############################################################################


suffix_units = {
    "years": "%Y",
    "year": "%Y",
    "months": "%Y_%m",
    "month": "%Y_%m",
    "weeks": "%Y_%m_%d",
    "week": "%Y_%m_%d",
    "days": "%Y_%m_%d",
    "day": "%Y_%m_%d",
    "hours": "%Y_%m_%d_%H",
    "hour": "%Y_%m_%d_%H",
    "minutes": "%Y_%m_%d_%H_%M",
    "minute": "%Y_%m_%d_%H_%M",
}


def create_suffix_from_datetime(start_time: datetime, granularity) -> str:
    """Create a datetime string based on a datetime object and a provided
    granularity that can be used as suffix for map names.

    dateteime=2001-01-01 00:00:00, granularity="1 month" returns "2001_01"

    :param start_time: The datetime object
    :param granularity: The granularity for example "1 month" or "100 seconds"
    :return: A string
    """
    global suffix_units
    return start_time.strftime(suffix_units[granularity.split(" ")[1]])


def create_time_suffix(mapp, end: bool = False):
    """Create a datetime string based on a map datetime object

    :param mapp: a temporal map dataset
    :param end: True if you want add also end time to the suffix
    """
    start = mapp.temporal_extent.get_start_time()
    sstring = start.isoformat().replace(":", "_").replace("-", "_")
    if end:
        end = mapp.temporal_extent.get_end_time()
        estring = end.isoformat().replace(":", "_").replace("-", "_")
        return "{st}_{en}".format(st=sstring, en=estring)
    return sstring


def create_numeric_suffix(base, count: int, zeros: str) -> str:
    """Create a string based on count and number of zeros decided by zeros

    :param base: the basename for new map
    :param count: a number
    :param zeros: a string containing the expected number, coming from suffix option
                  like "%05"
    """
    spli = zeros.split("%")
    if len(spli) == 2:
        suff = spli[1]
        if suff.isdigit():
            zero = suff if int(suff[0]) == 0 else "0{nu}".format(nu=suff)
        else:
            zero = "05"
    else:
        zero = "05"
    s = "{ba}_{i:" + zero + "d}"
    return s.format(ba=base, i=count)


if __name__ == "__main__":
    import doctest

    doctest.testmod()
