## DESCRIPTION

This command has 2 modes of operation. If no **date** argument is
supplied, then the current timestamp for the vector map is printed. If a
date argument is specified, then the timestamp for the vector map is set
to the specified date(s). See examples below.

See [TIMESTAMP FORMAT](r.timestamp.html#timestamp-format) description
for details.

## NOTES

Strings containing spaces should be quoted. For specifying a range of
time, the two timestamps should be separated by a forward slash. To
remove the timestamp from a vector map, use **date=none**.

## EXAMPLES

Prints the timestamp for the \"lidar\" vector map. If there is no
timestamp for \"lidar\", nothing is printed. If there is a timestamp,
one or two time strings are printed, depending on if the timestamp for
the map consists of a single date or two dates (ie start and end dates).

```
v.timestamp map=lidar
```

Sets the timestamp for \"lidar\" to the single date \"15 sep 1987\".

```
v.timestamp map=lidar date='15 sep 1987'
```

Sets the timestamp for \"lidar\" to have the start date \"15 sep 1987\"
and the end date \"20 feb 1988\".

```
v.timestamp map=lidar date='15 sep 1987/20 feb 1988'
```

Removes the timestamp for the \"lidar\" vector map.

```
v.timestamp map=lidar date=none
```

## KNOWN ISSUES

Spaces in the timestamp value are required.

## SEE ALSO

*[v.info](v.info.html), [r.timestamp](r.timestamp.html),
[r3.timestamp](r3.timestamp.html)*

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory
