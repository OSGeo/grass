## DESCRIPTION

*r.what.color* outputs the color associated with user-specified category
values in a raster input map.

Values may be specified either using the **values=** option, or by
specifying the **-i** flag and passing the values on `stdin`, one per
line.

For each value which is specified, a line of output will be generated
consisting of the category value followed by the color, e.g.:

```sh
r.what.color input=elevation.dem value=1500
1500: 223:127:31
```

If the input map is an integer (CELL) map, the category will be written
as an integer (no decimal point), otherwise it will be written in
floating point format (*printf("%.15g")* format).

If the lookup fails for a value, the color will be output as an
asterisk, e.g.:

```sh
r.what.color input=elevation.dem value=9999
9999: *
```

If a value cannot be parsed, both the value and the color will be output
as an asterisk, e.g.:

```sh
r.what.color input=elevation.dem value=bogus
*: *
```

The format can be changed using the **format=** option. The value should
be a *printf()*-style format string containing three conversion
specifiers for the red, green and blue values respectively, e.g.:

```sh
r.what.color input=elevation.dem value=1500 format='%02x:%02x:%02x'
1500: df:7f:1f
```

If your system supports the *%m\$* syntax, you can change the ordering
of the components, e.g.:

```sh
r.what.color input=elevation.dem value=1500 format='%3$02x:%2$02x:%1$02x'
1500: 1f:7f:df
```

Common formats:  

- Tcl/Tk: `format="#%02x%02x%02x"`
- WxPython: `format='"#%02x%02x%02x"'` or `format='"(%d,%d,%d)"'`

## SEE ALSO

*[r.what](r.what.md)*

## AUTHOR

Glynn Clements
