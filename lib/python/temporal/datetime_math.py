"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS datetime math functions to be used in library functions and modules.

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from datetime import datetime, date, time, timedelta
import grass.script.core as core
import copy

DAY_IN_SECONDS = 86400
SECOND_AS_DAY = 1.1574074074074073e-05

###############################################################################


def relative_time_to_time_delta(value):
    """!Convert the double value representing days
       into a timedelta object.
    """

    days = int(value)
    seconds = value % 1
    seconds = round(seconds * DAY_IN_SECONDS)

    return timedelta(days, seconds)

###############################################################################


def time_delta_to_relative_time(delta):
    """!Convert the time delta into a
       double value, representing days.
    """

    return float(delta.days) + float(delta.seconds * SECOND_AS_DAY)

###############################################################################


def increment_datetime_by_string(mydate, increment, mult=1):
    """!Return a new datetime object incremented with the provided
       relative dates specified as string.
       Additional a multiplier can be specified to multiply the increment
       before adding to the provided datetime object.
       
       Usage:
       
       @code
       
        >>> dt = datetime(2001, 9, 1, 0, 0, 0)
        >>> string = "60 seconds, 4 minutes, 12 hours, 10 days, 1 weeks, 5 months, 1 years"
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
        
        @endcode
       
       @param mydate A datetime object to incremented
       @param increment A string providing increment information:
                  The string may include comma separated values of type seconds,
                  minutes, hours, days, weeks, months and years
                  Example: Increment the datetime 2001-01-01 00:00:00
                  with "60 seconds, 4 minutes, 12 hours, 10 days, 1 weeks, 5 months, 1 years"
                  will result in the datetime 2003-02-18 12:05:00
       @param mult A multiplier, default is 1
    """

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
            if len(inc) < 2:
                core.error(_("Wrong increment format: %s") % (increment))
                return None
            if inc[1].find("seconds") >= 0 or inc[1].find("second") >= 0:
                seconds = mult * int(inc[0])
            elif inc[1].find("minutes") >= 0 or inc[1].find("minute") >= 0:
                minutes = mult * int(inc[0])
            elif inc[1].find("hours") >= 0 or inc[1].find("hour") >= 0:
                hours = mult * int(inc[0])
            elif inc[1].find("days") >= 0 or inc[1].find("day") >= 0:
                days = mult * int(inc[0])
            elif inc[1].find("weeks") >= 0 or inc[1].find("week") >= 0:
                weeks = mult * int(inc[0])
            elif inc[1].find("months") >= 0 or inc[1].find("month") >= 0:
                months = mult * int(inc[0])
            elif inc[1].find("years") >= 0 or inc[1].find("year") >= 0:
                years = mult * int(inc[0])
            else:
                core.error(_("Wrong increment format: %s") % (increment))
                return None

        return increment_datetime(mydate, years, months, weeks, days, hours, minutes, seconds)

    return mydate

###############################################################################


def increment_datetime(mydate, years=0, months=0, weeks=0, days=0, hours=0, 
                       minutes=0, seconds=0):
    """!Return a new datetime object incremented with the provided 
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

    if years > 0:
        # Make a deep copy of the datetime object
        dt1 = copy.copy(mydate)
        # Compute the number of days
        dt1 = dt1.replace(year=mydate.year + int(years))
        tdelta_years = dt1 - mydate

    return mydate + tdelta_seconds + tdelta_minutes + tdelta_hours + \
        tdelta_days + tdelta_weeks + tdelta_months + tdelta_years

###############################################################################


def adjust_datetime_to_granularity(mydate, granularity):
    """!Modify the datetime object to fit the given granularity    
    
        - Years will start at the first of Januar
        - Months will start at the first day of the month
        - Days will start at the first Hour of the day
        - Hours will start at the first minute of an hour
        - Minutes will start at the first second of a minute
        
        Usage:
        
        @code
        
        >>> dt = datetime(2001, 8, 8, 12,30,30)
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
        
        >>> adjust_datetime_to_granularity(dt, "2 years, 3 months, 5 days, 3 hours, 3 minutes, 2 seconds")
        datetime.datetime(2001, 8, 8, 12, 30, 30)
        
        >>> adjust_datetime_to_granularity(dt, "3 months, 5 days, 3 minutes")
        datetime.datetime(2001, 8, 8, 12, 30)
        
        >>> adjust_datetime_to_granularity(dt, "3 weeks, 5 days")
        datetime.datetime(2001, 8, 8, 0, 0)
        
        @endcode
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
                core.error(_("Wrong granularity format: %s") % (granularity))
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
                days = days - weekday  # this needs to be fixed
            else:
                days = days + weekday  # this needs to be fixed
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
        return dt.replace(year=years, month=months, day=days,
                            hour=hours, minute=minutes, second=seconds)

###############################################################################


def compute_datetime_delta(start, end):
    """!Return a dictionary with the accumulated delta in year, month, day, 
       hour, minute and second
       
        Usage:
        
        @code
        
        >>> start = datetime(2001, 1, 1, 00,00,00)
        >>> end = datetime(2001, 1, 1, 00,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'month': 0, 'second': 0, 'max_days': 0, 'year': 0, 'day': 0, 'minute': 0}
        
        >>> start = datetime(2001, 1, 1, 00,00,14)
        >>> end = datetime(2001, 1, 1, 00,00,44)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'month': 0, 'second': 30, 'max_days': 0, 'year': 0, 'day': 0, 'minute': 0}
        
        >>> start = datetime(2001, 1, 1, 00,00,44)
        >>> end = datetime(2001, 1, 1, 00,01,14)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'month': 0, 'second': 30, 'max_days': 0, 'year': 0, 'day': 0, 'minute': 1}
        
        >>> start = datetime(2001, 1, 1, 00,00,30)
        >>> end = datetime(2001, 1, 1, 00,05,30)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'month': 0, 'second': 300, 'max_days': 0, 'year': 0, 'day': 0, 'minute': 5}
        
        >>> start = datetime(2001, 1, 1, 00,00,00)
        >>> end = datetime(2001, 1, 1, 00,01,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'month': 0, 'second': 0, 'max_days': 0, 'year': 0, 'day': 0, 'minute': 1}
        
        >>> start = datetime(2011,10,31, 00,45,00)
        >>> end = datetime(2011,10,31, 01,45,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 1, 'second': 0, 'max_days': 0, 'year': 0, 'day': 0, 'minute': 60}
        
        >>> start = datetime(2011,10,31, 00,45,00)
        >>> end = datetime(2011,10,31, 01,15,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 1, 'second': 0, 'max_days': 0, 'year': 0, 'day': 0, 'minute': 30}
        
        >>> start = datetime(2011,10,31, 00,45,00)
        >>> end = datetime(2011,10,31, 12,15,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 12, 'second': 0, 'max_days': 0, 'year': 0, 'day': 0, 'minute': 690}
        
        >>> start = datetime(2011,10,31, 00,00,00)
        >>> end = datetime(2011,10,31, 01,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 1, 'second': 0, 'max_days': 0, 'year': 0, 'day': 0, 'minute': 0}
        
        >>> start = datetime(2011,10,31, 00,00,00)
        >>> end = datetime(2011,11,01, 01,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 25, 'second': 0, 'max_days': 1, 'year': 0, 'day': 1, 'minute': 0}
        
        >>> start = datetime(2011,10,31, 12,00,00)
        >>> end = datetime(2011,11,01, 06,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 18, 'second': 0, 'max_days': 0, 'year': 0, 'day': 0, 'minute': 0}
        
        >>> start = datetime(2011,11,01, 00,00,00)
        >>> end = datetime(2011,12,01, 01,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 721, 'month': 1, 'second': 0, 'max_days': 30, 'year': 0, 'day': 0, 'minute': 0}
        
        >>> start = datetime(2011,11,01, 00,00,00)
        >>> end = datetime(2011,11,05, 00,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'second': 0, 'max_days': 4, 'year': 0, 'day': 4, 'minute': 0}
        
        >>> start = datetime(2011,10,06, 00,00,00)
        >>> end = datetime(2011,11,05, 00,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'second': 0, 'max_days': 30, 'year': 0, 'day': 30, 'minute': 0}
        
        >>> start = datetime(2011,12,02, 00,00,00)
        >>> end = datetime(2012,01,01, 00,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'second': 0, 'max_days': 30, 'year': 1, 'day': 30, 'minute': 0}
        
        >>> start = datetime(2011,01,01, 00,00,00)
        >>> end = datetime(2011,02,01, 00,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'month': 1, 'second': 0, 'max_days': 31, 'year': 0, 'day': 0, 'minute': 0}
        
        >>> start = datetime(2011,12,01, 00,00,00)
        >>> end = datetime(2012,01,01, 00,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'month': 1, 'second': 0, 'max_days': 31, 'year': 1, 'day': 0, 'minute': 0}
        
        >>> start = datetime(2011,12,01, 00,00,00)
        >>> end = datetime(2012,06,01, 00,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'month': 6, 'second': 0, 'max_days': 183, 'year': 1, 'day': 0, 'minute': 0}
        
        >>> start = datetime(2011,06,01, 00,00,00)
        >>> end = datetime(2021,06,01, 00,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'month': 120, 'second': 0, 'max_days': 3653, 'year': 10, 'day': 0, 'minute': 0}
        
        >>> start = datetime(2011,06,01, 00,00,00)
        >>> end = datetime(2012,06,01, 12,00,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 8796, 'month': 12, 'second': 0, 'max_days': 366, 'year': 1, 'day': 0, 'minute': 0}
        
        >>> start = datetime(2011,06,01, 00,00,00)
        >>> end = datetime(2012,06,01, 12,30,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 8796, 'month': 12, 'second': 0, 'max_days': 366, 'year': 1, 'day': 0, 'minute': 527790}
        
        >>> start = datetime(2011,06,01, 00,00,00)
        >>> end = datetime(2012,06,01, 12,00,05)
        >>> compute_datetime_delta(start, end)
        {'hour': 8796, 'month': 12, 'second': 31665605, 'max_days': 366, 'year': 1, 'day': 0, 'minute': 0}
        
        >>> start = datetime(2011,06,01, 00,00,00)
        >>> end = datetime(2012,06,01, 00,30,00)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'month': 12, 'second': 0, 'max_days': 366, 'year': 1, 'day': 0, 'minute': 527070}
        
        >>> start = datetime(2011,06,01, 00,00,00)
        >>> end = datetime(2012,06,01, 00,00,05)
        >>> compute_datetime_delta(start, end)
        {'hour': 0, 'month': 12, 'second': 31622405, 'max_days': 366, 'year': 1, 'day': 0, 'minute': 0}
        
        @endcode
        
       @return A dictionary with year, month, day, hour, minute and second as keys()
    """
    comp = {}

    day_diff = (end - start).days

    comp["max_days"] = day_diff

    # Date
    # Count full years
    d = end.year - start.year
    comp["year"] = d

    # Count full months
    if start.month == 1 and end.month == 1:
        comp["month"] = 0
    elif   start.day == 1 and end.day == 1:
        d = end.month - start.month
        if d < 0:
            d = d + 12 * comp["year"]
        elif d == 0:
            d = 12 * comp["year"]
        comp["month"] = d

    # Count full days
    if  start.day == 1 and end.day == 1:
        comp["day"] = 0
    else:
        comp["day"] = day_diff

    # Time
    # Hours
    if start.hour == 0 and end.hour == 0:
        comp["hour"] = 0
    else:
        d = end.hour - start.hour
        if d < 0:
            d = d + 24 + 24 * day_diff
        else:
            d = d + 24 * day_diff
        comp["hour"] = d

    # Minutes
    if start.minute == 0 and end.minute == 0:
        comp["minute"] = 0
    else:
        d = end.minute - start.minute
        if d != 0:
            if comp["hour"]:
                d = d + 60 * comp["hour"]
            else:
                d = d + 24 * 60 * day_diff
        elif d == 0:
            if comp["hour"]:
                d = 60 * comp["hour"]
            else:
                d = 24 * 60 * day_diff

        comp["minute"] = d

    # Seconds
    if start.second == 0 and end.second == 0:
        comp["second"] = 0
    else:
        d = end.second - start.second
        if d != 0:
            if comp["minute"]:
                d = d + 60 * comp["minute"]
            elif comp["hour"]:
                d = d + 3600 * comp["hour"]
            else:
                d = d + 24 * 60 * 60 * day_diff
        elif d == 0:
            if comp["minute"]:
                d = 60 * comp["minute"]
            elif comp["hour"]:
                d = 3600 * comp["hour"]
            else:
                d = 24 * 60 * 60 * day_diff
        comp["second"] = d

    return comp

###############################################################################


def string_to_datetime(time_string):
    """!Convert a string into a datetime object 
    
        Supported ISO string formats are: 
        - YYYY-mm-dd
        - YYYY-mm-dd HH:MM:SS
        
        Time zones are not supported
        
        @param time_string: The time string to convert
        @return datetime object or None in case of an error
    """

    # BC is not supported
    if time_string.find("bc") > 0:
        core.error("Dates Before Christ are not supported "
                   "in the temporal database")
        return None

    # BC is not supported
    if time_string.find("+") > 0:
        core.error("Time zones are not supported "
                   "in the temporal database")
        return None
        
    if time_string.find(":") > 0:
        time_format = "%Y-%m-%d %H:%M:%S"
    else:
        time_format = "%Y-%m-%d"
  
    try:
        return  datetime.strptime(time_string, time_format)
    except:
        core.error("Unable to parse time string: %s"%time_string)
        return None


###############################################################################


def datetime_to_grass_datetime_string(dt):
    """!Convert a python datetime object into a GRASS datetime string"""

    # GRASS datetime month names
    month_names = ["", "jan", "feb", "mar", "apr", "may", "jun",
                   "jul", "aug", "sep", "oct", "nov", "dec"]

    # Check for time zone info in the datetime object
    if dt.tzinfo is not None:
        string = "%.2i %s %.2i %.2i:%.2i:%.2i %+.4i" % (dt.day, 
                 month_names[dt.month], dt.year,
                 dt.hour, dt.minute, dt.second, dt.tzinfo._offset.seconds / 60)
    else:
        string = "%.2i %s %.4i %.2i:%.2i:%.2i" % (dt.day, month_names[
            dt.month], dt.year, dt.hour, dt.minute, dt.second)

    return string

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()