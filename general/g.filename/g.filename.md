## DESCRIPTION

*g.filename* is designed for Bourne shell scripts that need to know the
full file name, including it's path, for mapset elements, like raster,
vector and site maps, region definitions and imagery groups.

The list of element names to search for is not fixed; any subdirectory
of the mapset directory is a valid element name.

However, the user can find the list of standard GRASS GIS element names
in the file `$GISBASE/etc/element_list`. This is the file which
g.remove/g.rename/g.copy use to determine which files need to be
deleted/renamed/copied for a given entity type.

## OUTPUT

*g.filename* writes one line to standard output:

file='*full_file_pathname'*

The output is a */bin/sh* command to set the variable specified by the
file *name* to the full UNIX path name for the data base file. This
variable may be set in the */bin/sh* as follows:

```sh
eval `g.filename element=name mapset=name file=name`
```

## NOTES

This module generates the filename, but does not care if the file (or
mapset or element) exists or not. This feature allows shell scripts to
create new data base files as well as use existing ones.

If the mapset is the current mapset, *g.filename* can automatically
create the *element* specified if it doesn't already exist when **-c**
flag is used. This makes it easy to add new files to the data base
without having to worry about the existence of the required data base
directories. (This program will not create a new mapset, however, if
that specified does not currently exist.)

This module should not be used to create directories which are at the
level of what this module refer to as files, i.e., directory which
carries a name specified by a user (such as vector map directories)
should not be created using this module. Standard library functions
coming with any given language are a more appropriate tool for that.

The program exits with a 0 if everything is ok; it exits with a non-zero
value if there is an error, in which case file=*'full_file_pathname'* is
not output.

## SEE ALSO

*[g.findfile](g.findfile.md), [g.gisenv](g.gisenv.md)*

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory
