## DESCRIPTION

The *r.mask.status* reports information about the 2D raster mask and its
status. The tool reports whether the mask is present or not. For both
active and inactive mask, the tool reports a full name of the raster
(name including the mapset) which represents or would represent the
mask. It can also report full name of the underlying raster if the mask
is reclassified from another raster. The tool can be used to check if
the mask is currently set (`present` boolean in JSON), what is raster
name used to represent the mask (`name` string in JSON), and whether the
raster is reclassifed from another (`is_reclass_of` string or null in
JSON). YAML and shell script style outputs are following the JSON output
if possible. The plain text format outputs multi-line human-readable
information in natural language.

With the **-t** flag, no output is printed, instead a return code is
used to indicate presence or absence. The convention is the same same
the POSIX *test* utility, so *r.mask.status* returns 0 when the mask is
present and 1 otherwise.

## EXAMPLES

### Generate JSON output

To generate JSON output in Bash, use the **format** option:

```sh
r.mask.status format=json
```

In Python, use:

```python
import grass.script as gs
gs.parse_command("r.mask.status", format="json")
```

This returns a dictionary with keys `present`, `full_name`, and
`is_reclass_of`.

### Use as the test utility

The POSIX *test* utility uses return code 0 to indicate presence and 1
to indicate absence of a file, so testing existence of a file with
`test -f` gives return code 0 when the file exists. *r.mask.status* can
be used in the same with the **-t** flag:

```sh
r.mask.status -t
```

In a Bash script:

```sh
# Bash
if r.mask.status -t; then
    echo "Masking is active"
else
    echo "Masking is not active"
fi
```

## SEE ALSO

*[r.mask](r.mask.md), [g.region](g.region.md)*

## AUTHORS

Vaclav Petras, NC State University, Center for Geospatial Analytics
