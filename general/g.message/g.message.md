## DESCRIPTION

*g.message* prints a message, warning, progress info, or fatal error in
the GRASS GIS way. This program is to be used in Shell/Perl/Python
scripts, so the author does not need to use the `echo` program. The
advantage of *g.message* is that it formats messages just like other
GRASS modules do and that its functionality is influenced by the
`GRASS_VERBOSE` and `GRASS_MESSAGE_FORMAT` environment variables.

The program can be used for standard informative messages as well as
warnings (**-w** flag) and fatal errors (**-e** flag). For debugging
purposes, the **-d** flag will cause *g.message* to print a debugging
message at the given level.

## NOTES

Messages containing "`=`" must use the full **message=** syntax so the
parser doesn't get confused.

If you want a long message (multi-line) to be dealt with as a single
paragraph, use a single call to *g.message* with text split in the
script using the backslash as the last character. (In shell scripts
don't close the "quote")

A blank line may be obtained with

```sh
g.message message=""
```

Redundant whitespace will be stripped away.

It's advisable to single quote the messages that are to be printed
literally. It prevents a number of characters (most notably, space and
the dollar sign '`$`') from being treated specifically by the shell.

When it is necessary to include, for example, a variable's value as part
of the message, the double quotes may be used, which do not deprive the
dollar sign of its special variable-expansion powers.

While it is known that the interactive Bash instances may treat the
exclamation mark '`!`' character specifically (making single quoting of
it necessary), it shouldn't be the case for the non-interactive
instances of Bash. Nonetheless, to avoid context-based confusion later
on you are encouraged to single-quote messages that do not require
`$VARIABLE` expansion.

### Usage in Python scripts

[GRASS Python Scripting
Library](https://grass.osgeo.org/grass-devel/manuals/libpython/) defines
special wrappers for *g.message*.

- `debug()` for `g.message -d`
- `error()` for `g.message -e`
- `fatal()` for `g.message -e` + `exit()`
- `info()` for `g.message -i`
- `message()` for `g.message`
- `verbose()` for `g.message -v`
- `warning()` for `g.message -w`

Note: The Python tab in the *wxGUI* can be used for entering the
following sample code:

```sh
import grass.script as gcore

gcore.warning("This is a warning")
```

is identical with

```sh
g.message -w message="This is a warning"
```

### VERBOSITY LEVELS

Controlled by the "`GRASS_VERBOSE`" environment variable. Typically this
is set using the **--quiet** or **--verbose** command line options.

- 0 - only errors and warnings are printed
- 1 - progress messages are printed
- 2 - all module messages are printed
- 3 - additional verbose messages are printed

### DEBUG LEVELS

Controlled by the "`DEBUG`" GRASS *gisenv* variable (set with
*[g.gisenv](g.gisenv.md)*).  
Recommended levels:

- 1 - message is printed once or few times per module
- 3 - each row (raster) or line (vector)
- 5 - each cell (raster) or point (vector)

## EXAMPLES

This basic example prints the message "hello" in the console:

```sh
g.message message="hello"
```

To print a message as an error message use the **-e** flag:

```sh
g.message -e message="my error"
```

To print a message highlighted as a debug message ("D0/0: debug") in the
console, use the **-d** flag. Optionally the debug level can be defined
(see also [g.gisenv](g.gisenv.md) for details):

```sh
# Levels: (recommended levels)
#   0 - silence
#   1 - message is printed once or few times per module
#   3 - each row (raster) or line (vector)
#   5 - each cell (raster) or point (vector)
g.message -d message="debug" debug=0
```

To print a message highlighted as a warning message ("WARNING: my
warning") in the console, use the **-w** flag:

```sh
g.message -w message="my warning"
```

## SEE ALSO

*[GRASS variables and environment variables](variables.md)*  
*[g.gisenv](g.gisenv.md), [g.parser](g.parser.md)*

## AUTHOR

Jachym Cepicky
