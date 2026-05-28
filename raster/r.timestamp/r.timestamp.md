## DESCRIPTION

*r.timestamp* has two modes of operation. If no **date** argument is
supplied, then the current timestamp for the raster map is printed. If a
date argument is specified, then the timestamp for the raster map is set
to the specified date(s). See examples below.

## NOTES

Strings containing spaces should be quoted. For specifying a range of
time, the two timestamps should be separated by a forward slash. To
remove the timestamp from a raster map, use **date=none**.

## TIMESTAMP FORMAT

The timestamp values must use the format as described in the *GRASS
Datetime Library*. The source tree for this library should have a
description of the format. For convenience, the formats are reproduced
here:

There are two types of datetime values:

- *absolute* and
- *relative*.

Absolute values specify exact dates and/or times. Relative values
specify a span of time.

### Absolute

The general format for absolute values is:

```sh
  day month year [bc] hour:minute:seconds timezone

         day is 1-31
         month is jan,feb,...,dec
         year is 4 digit year
         [bc] if present, indicates dates is BC
         hour is 0-23 (24 hour clock)
         minute is 0-59
         second is 0-59.9999 (fractions of second allowed)
         timezone is +hhmm or -hhmm (eg, -0600)
```

Some parts can be missing, for example

```sh
         1994 [bc]
         Jan 1994 [bc]
         15 jan 1000 [bc]
         15 jan 1994 [bc] 10 [+0000]
         15 jan 1994 [bc] 10:00 [+0100]
         15 jan 1994 [bc] 10:00:23.34 [-0500]
```

### Relative

There are two types of relative datetime values, year-month and
day-second. The formats are:

```sh
         [-] # years # months
         [-] # days # hours # minutes # seconds
```

The words years, months, days, hours, minutes, seconds are literal
words, and the \# are the numeric values. Examples:

```sh
         2 years
         5 months
         2 years 5 months
         100 days
         15 hours 25 minutes 35.34 seconds
         100 days 25 minutes
         1000 hours 35.34 seconds
```

The following are *illegal* because it mixes year-month and day-second
(because the number of days in a month or in a year vary):

```sh
         3 months 15 days
         3 years 10 days
```

## EXAMPLES

Prints the timestamp for the "soils" raster map. If there is no
timestamp for "soils", nothing is printed. If there is a timestamp, one
or two time strings are printed, depending on if the timestamp for the
map consists of a single date or two dates (ie start and end dates).

```sh
r.timestamp map=soils
```

Sets the timestamp for "soils" to the single date "15 sep 1987".

```sh
r.timestamp map=soils date='15 sep 1987'
```

Sets the timestamp for "soils" to have the start date "15 sep 1987" and
the end date "20 feb 1988".

```sh
r.timestamp map=soils date='15 sep 1987/20 feb 1988'
```

Sets the timestamp for "soils" to have the start date "18 feb 2005
10:30:00" and the end date "20 jul 2007 20:30:00".

```sh
r.timestamp map=soils date='18 feb 2005 10:30:00/20 jul 2007 20:30:00'
```

Removes the timestamp for the "soils" raster map.

```sh
r.timestamp map=soils date=none
```

## KNOWN ISSUES

Spaces in the timestamp value are required.

## SEE ALSO

*[r.info](r.info.md), [r3.timestamp](r3.timestamp.md),
[v.timestamp](v.timestamp.md)*

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory
