## DESCRIPTION

*g.findfile* is designed for Bourne shell or Python scripts that need to
search for mapset *elements*, including: raster, vector maps, region
definitions and *[imagery](i.group.md)* groups.

The list of **element** names to search for is not fixed; any
subdirectory of the mapset directory is a valid **element** name.

However, the user can find the list of standard GRASS **element** names
in the file `$GISBASE/etc/element_list`. This is the file which
*[g.remove](g.remove.md)*, *[g.rename](g.rename.md)* and
*[g.copy](g.copy.md)* use to determine which files need to be
deleted/renamed/copied for a given entity type.

## NOTES

*g.findfile* writes four lines to standard output:

```sh
name='file_name'
mapset='mapset_name'
file='unix_filename'
fullname='grass_fullname'
```

The output is *Bash* commands to set the variable *name* to the GRASS
data base file name, *mapset* to the mapset in which the file resides,
and *file* to the full UNIX path name for the named file. These
variables may be set in the *Bash* as follows:

```sh
eval `g.findfile element=name mapset=name file=name`
```

## EXAMPLES

### SHELL

**Raster map example:**

```sh
eval `g.findfile element=cell file=elevation`
```

If the specified file (here: raster map) does not exist, the variables
will be set as follows:

```sh
name=
mapset=
fullname=
file=
```

The following is a way to test for this case:

```sh
if [ ! "$file" ]
then
    exit 1
fi
```

**Vector map example (including error message):**

```sh
eval `g.findfile element=vector file="$G_OPT_V_INPUT"`
if [ ! "$file" ] ; then
   g.message -e "Vector map <$G_OPT_V_INPUT> not found"
   exit 1
fi
```

### PYTHON

See *[Python Scripting
Library](https://grass.osgeo.org/grass-devel/manuals/libpython/)* for
more info.

Note: The Python tab in the *wxGUI* can be used for entering the
following code:

```python
import grass.script as gs

gs.find_file('elevation', element = 'cell')
```

## SEE ALSO

*[g.filename](g.filename.md), [g.gisenv](g.gisenv.md),
[g.mapsets](g.mapsets.md), [g.parser](g.parser.md)*

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory
