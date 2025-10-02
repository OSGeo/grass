## DESCRIPTION

*r.what.color* outputs the color associated with user-specified category
values in a raster input map.

Values may be specified either using the **value=** option, or by
specifying the **-i** flag and passing the values on `stdin`, one per
line.

For each specified value, an output will be generated consisting of the
category value along with the color, e.g.:

```sh
r.what.color input=elevation.dem value=1500
1500: 223:127:31

# In plain format using the triplet color format:
r.what.color input=elevation.dem value=1500 format=plain color_format=triplet
1500: 223:127:31

# In JSON format using the triplet color format:
r.what.color input=elevation.dem value=1500 format=json color_format=triplet
[
    {
        "value": 1500,
        "color": "223:127:31"
    }
]
```

Similarly, other `color_format` options available with `format=json` and
`format=plain` are `hex`, `hsv`, `triplet`, and `rgb`, with `hex` being the
default color format.

If the input map is an integer (CELL) map, the category will be written
as an integer (no decimal point), otherwise it will be written in
floating point format (*printf("%.15g")* format).

If the lookup fails for a value, the color will be output as an
asterisk (or as `null` in JSON), e.g.:

```sh
r.what.color input=elevation.dem value=9999
9999: *

# In plain format:
r.what.color input=elevation.dem value=9999 format=plain
9999: *

# In JSON format:
r.what.color input=elevation.dem value=9999 format=json
[
    {
        "value": 9999,
        "color": null
    }
]
```

If a value cannot be parsed, both the value and the color will be output
as an asterisk (or as `null` in JSON), e.g.:

```sh
r.what.color input=elevation.dem value=bogus
*: *

# In plain format:
r.what.color input=elevation.dem value=bogus format=plain
*: *

# In JSON format:
r.what.color input=elevation.dem value=bogus format=json
[
    {
        "value": null,
        "color": null
    }
]
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

NOTE:

Please note that the *printf()*-style output format is deprecated and will be
removed in a future release. Use the `color_format` option instead,
together with `format=plain` or `format=json`.

## Using r.what.color JSON output with python

Print color associated with user-specified category value in JSON format using
Python:

```python
import grass.script as gs

# Run the r.what.color command with rgb option for JSON output format
items = gs.parse_command(
    "r.what.color",
    input="elevation",
    value=[100, 135, 156],
    format="json",
    color_format="rgb",
)

for item in items:
    print(f"{item['value']}: {item['color']}")
```

```text
100: rgb(255, 229, 0)
135: rgb(195, 127, 59)
156: rgb(23, 22, 21)
```

## SEE ALSO

*[r.what](r.what.md)*

## AUTHOR

Glynn Clements
