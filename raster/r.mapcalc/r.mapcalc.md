## DESCRIPTION

*r.mapcalc* performs arithmetic on raster map layers. New raster map
layers can be created which are arithmetic expressions involving
existing raster map layers, integer or floating point constants, and
functions.

### Program use

*r.mapcalc* expression have the form:

**result =** *expression*

where *result* is the name of a raster map layer to contain the result
of the calculation and **expression** is any legal arithmetic expression
involving existing raster map layers (except *result* itself), integer
or floating point constants, and functions known to the calculator.
Parentheses are allowed in the expression and may be nested to any
depth. *result* will be created in the user's current mapset.

As **expression=** is the first option, it is the default. This means
that passing an expression on the command line is possible as long as
the expression is quoted and a space is included before the first *=*
sign. Example ('foo' is the resulting map):

```sh
r.mapcalc "foo = 1"
```

or:

```sh
r.mapcalc 'foo = 1'
```

An unquoted expression (i.e. split over multiple arguments) won't work,
nor will omitting the space before the = sign:

```sh
r.mapcalc 'foo=1'
Sorry, <foo> is not a valid parameter
```

To read command from the file, use file= explicitly, e.g.:

```sh
r.mapcalc file=file
```

or:

```sh
r.mapcalc file=- < file
```

or:

```sh
r.mapcalc file=- <<EOF
foo = 1
EOF
```

The formula entered to *r.mapcalc* by the user is recorded both in the
*result* map title (which appears in the category file for *result*) and
in the history file for *result*.

Some characters have special meaning to the command shell. If the user
is entering input to *r.mapcalc* on the command line, expressions should
be enclosed within single quotes. See NOTES, below.

### Computational regions in r.mapcalc

By default *r.mapcalc* uses the current region as computational region
that was set with [g.region](g.region.md) for processing. Sometimes it
is necessary to use a region that is derived from the raster maps in the
expression to set the computational region. This is of high importance
for modules that use r.mapcalc internally to process time series of
satellite images that all have different spatial extents. A module that
requires this feature is [t.rast.algebra](t.rast.algebra.md). The
*region* option of *r.mapcalc* was implemented to address this
requirement. It allows computing and using a region based on all raster
maps in an expression. Three modes are supported:

- Setting the *region* parameter to *current* will result in the use of
  the current region as computational region. This is the default. The
  current region can be set with [g.region](g.region.md).
- The parameter *union* will force r.mapcalc to compute the disjoint
  union of all regions from raster maps specified in the expression.
  This computed region will then be used as computational region at
  runtime. The region of the mapset will not be modified. The smallest
  spatial resolution of all raster maps will be used for processing.
- The parameter *intersect* will force r.mapcalc to compute the
  intersection of all regions from raster maps specified in the
  expression. This computed region will then be used as computational
  region at runtime. The region of the mapset will not be modified. The
  smallest spatial resolution of all raster maps will be used for
  processing.

### Operators and order of precedence

The following operators are supported:

```sh
     Operator   Meaning                    Type        Precedence
     --------------------------------------------------------------
     -          negation                   Arithmetic  12
     ~          one's complement           Bitwise     12
     !          not                        Logical     12
     ^          exponentiation             Arithmetic  11
     %          modulus                    Arithmetic  10
     /          division                   Arithmetic  10
     *          multiplication             Arithmetic  10
     +          addition                   Arithmetic   9
     -          subtraction                Arithmetic   9
     <<         left shift                 Bitwise      8
     >>         right shift                Bitwise      8
     >>>        right shift (unsigned)     Bitwise      8
     >          greater than               Logical      7
     >=         greater than or equal      Logical      7
     <          less than                  Logical      7
     <=         less than or equal         Logical      7
     ==         equal                      Logical      6
     !=         not equal                  Logical      6
     &          bitwise and                Bitwise      5
     |          bitwise or                 Bitwise      4
     &&         logical and                Logical      3
     &&&        logical and[1]             Logical      3
     ||         logical or                 Logical      2
     |||        logical or[1]              Logical      2
     ?:         conditional                Logical      1
```

(modulus is the remainder upon division)

\[1\] The &&& and \|\|\| operators handle null values differently to
other operators. See the section entitled **NULL support** below for
more details.

The operators are applied from left to right, with those of higher
precedence applied before those with lower precedence. Division by 0 and
modulus by 0 are acceptable and give a NULL result. The logical
operators give a 1 result if the comparison is true, 0 otherwise.

### Raster map layer names

Anything in the expression which is not a number, operator, or function
name is taken to be a raster map layer name. Examples:

```sh
elevation
x3
3d.his
```

Most GRASS raster map layers meet this naming convention. However, if a
raster map layer has a name which conflicts with the above rule, it
should be quoted. For example, the expression

```sh
x = a-b
```

would be interpreted as: x equals a minus b, whereas

```sh
x = "a-b"
```

would be interpreted as: x equals the raster map layer named *a-b*

Also

```sh
x = 3107
```

would create *x* filled with the number 3107, while

```sh
x = "3107"
```

would copy the raster map layer *3107* to the raster map layer *x*.

Quotes are not required unless the raster map layer names look like
numbers or contain operators, OR unless the program is run
non-interactively. Examples given here assume the program is run
interactively. See NOTES, below.

*r.mapcalc* will look for the raster map layers according to the user's
current mapset search path. It is possible to override the search path
and specify the mapset from which to select the raster map layer. This
is done by specifying the raster map layer name in the form:

```sh
name@mapset
```

For example, the following is a legal expression:

```sh
result = x@PERMANENT / y@SOILS
```

The mapset specified does not have to be in the mapset search path.
(This method of overriding the mapset search path is common to all GRASS
commands, not just *r.mapcalc*.)

### The neighborhood modifier

Maps and images are data base files stored in raster format, i.e.,
two-dimensional matrices of integer values. In *r.mapcalc*, maps may be
followed by a *neighborhood* modifier that specifies a relative offset
from the current cell being evaluated. The format is *map\[r,c\]*, where
*r* is the row offset and *c* is the column offset. For example,
*map\[1,2\]* refers to the cell one row below and two columns to the
right of the current cell, *map\[-2,-1\]* refers to the cell two rows
above and one column to the left of the current cell, and *map\[0,1\]*
refers to the cell one column to the right of the current cell. This
syntax permits the development of neighborhood-type filters within a
single map or across multiple maps.

The neighborhood modifier cannot be used on maps generated within same
*r.mapcalc* command run (see "KNOWN ISSUES" section).

### Raster map layer values from the category file

Sometimes it is desirable to use a value associated with a category's
*label* instead of the category value itself. If a raster map layer name
is preceded by the **@** operator, then the labels in the category file
for the raster map layer are used in the expression instead of the
category value.

For example, suppose that the raster map layer *soil.ph* (representing
soil pH values) has a category file with labels as follows:

```sh
cat     label
------------------
0       no data
1       1.4
2       2.4
3       3.5
4       5.8
5       7.2
6       8.8
7       9.4
```

Then the expression:

```sh
result = @soils.ph
```

would produce a result with category values 0, 1.4, 2.4, 3.5, 5.8, 7.2,
8.8 and 9.4.

Note that this operator may only be applied to raster map layers and
produces a floating point value in the expression. Therefore, the
category label must start with a valid number. If the category label is
integer, it will be represented by a floating point number. I the
category label does not start with a number or is missing, it will be
represented by NULL (no data) in the resulting raster map.

### Grey scale equivalents and color separates

It is often helpful to manipulate the colors assigned to map categories.
This is particularly useful when the spectral properties of cells have
meaning (as with imagery data), or when the map category values
represent real quantities (as when category values reflect true
elevation values). Map color manipulation can also aid visual
recognition, and map printing.

The \# operator can be used to either convert map category values to
their grey scale equivalents or to extract the red, green, or blue
components of a raster map layer into separate raster map layers.

```sh
result = #map
```

converts each category value in *map* to a value in the range 0-255
which represents the grey scale level implied by the color for the
category. If the map has a grey scale color table, then the grey level
is what \#map evaluates to. Otherwise, it is computed as:

```sh
 0.10 * red + 0.81 * green + 0.01 * blue
```

Alternatively, you can use:

```sh
result = y#map
```

to use the NTSC weightings:

```sh
 0.30 * red + 0.59 * green + 0.11 * blue
```

Or, you can use:

```sh
result = i#map
```

to use equal weightings:

```sh
 0.33 * red + 0.33 * green + 0.33 * blue
```

The \# operator has three other forms: r#map, g#map, b#map. These
extract the red, green, or blue components in the named raster map,
respectively. The GRASS shell script *[r.blend](r.blend.md)* extracts
each of these components from two raster map layers, and combines them
by a user-specified percentage. These forms allow color separates to be
made. For example, to extract the red component from *map* and store it
in the new 0-255 map layer *red*, the user could type:

```sh
red = r#map
```

To assign this map grey colors type:

```sh
r.colors map=red color=rules
black
white
```

To assign this map red colors type:

```sh
r.colors map=red color=rules
black
red
```

### Functions

The functions currently supported are listed in the table below. The
type of the result is indicated in the last column. *F* means that the
functions always results in a floating point value, *I* means that the
function gives an integer result, and *\** indicates that the result is
float if any of the arguments to the function are floating point values
and integer if all arguments are integer.

```sh
function                description                                     type
---------------------------------------------------------------------------
abs(x)                  return absolute value of x                      *
acos(x)                 inverse cosine of x (result is in degrees)      F
asin(x)                 inverse sine of x (result is in degrees)        F
atan(x)                 inverse tangent of x (result is in degrees)     F
atan(x,y)               inverse tangent of y/x (result is in degrees)   F
ceil(x)                 the smallest integral value not less than x     *
cos(x)                  cosine of x (x is in degrees)                   F
double(x)               convert x to double-precision floating point    F
eval([x,y,...,]z)       evaluate values of listed expr, pass results to z
exp(x)                  exponential function of x                       F
exp(x,y)                x to the power y                                F
float(x)                convert x to single-precision floating point    F
floor(x)                the largest integral value not greater than x   *
graph(x,x1,y1[x2,y2..]) convert the x to a y based on points in a graph F
graph2(x,x1[,x2,..],y1[,y2..])
                        alternative form of graph()                     F
if                      decision options:                               *
if(x)                   1 if x not zero, 0 otherwise
if(x,a)                 a if x not zero, 0 otherwise
if(x,a,b)               a if x not zero, b otherwise
if(x,a,b,c)             a if x > 0, b if x is zero, c if x < 0
int(x)                  convert x to integer [ truncates ]              I
isnull(x)               check if x = NULL
log(x)                  natural log of x                                F
log(x,b)                log of x base b                                 F
max(x,y[,z...])         largest value of those listed                   *
median(x,y[,z...])      median value of those listed                    *
min(x,y[,z...])         smallest value of those listed                  *
mod(x,y)                return the modulus (the remainder) of x/y       *
mode(x,y[,z...])        mode value of those listed                      *
nmax(x,y[,z...])        largest value of those listed, excluding NULLs  *
nmedian(x,y[,z...])     median value of those listed, excluding NULLs   *
nmin(x,y[,z...])        smallest value of those listed, excluding NULLs *
nmode(x,y[,z...])       mode value of those listed, excluding NULLs     *
not(x)                  1 if x is zero, 0 otherwise
pow(x,y)                x to the power y                                *
rand(a,b)               random value x : a <= x < b                     *
round(x)                round x to nearest integer                      I
round(x,y)              round x to nearest multiple of y
round(x,y,z)            round x to nearest y*i+z for some integer i
sin(x)                  sine of x (x is in degrees)                     F
sqrt(x)                 square root of x                                F
tan(x)                  tangent of x (x is in degrees)                  F
xor(x,y)                exclusive-or (XOR) of x and y                   I
```

```sh
Internal variables:
 row()                  current row of moving window                    I
 col()                  current col of moving window                    I
 nrows()                number of rows in computation region            I
 ncols()                number of columns in computation region         I
 x()                    current x-coordinate of moving window           F
 y()                    current y-coordinate of moving window           F
 ewres()                current east-west resolution                    F
 nsres()                current north-south resolution                  F
 area()                 area of current cell in square meters           F
 null()                 NULL value
```

Note, that the row() and col() indexing starts with 1.

### Data types and their precision

There are three data types that can be used within the mapcalc
expression. They are defined with certain precision and within certain
value range. The following table reports precision and value range info
on AMD64 (x86-64) architectures compiled with GCC/CLANG. Note that
although this setting is the most frequent one, the ranges could differ
for different architectures.

```sh
data type               precision and value range info
-------------------------------------------------------------------------------
 int                    a 32-bit integer with a range from -2,147,483,647 to
                        +2,147,483,647. The value -2,147,483,648 is reserved
                        for NODATA.
 float                  a 32-bit float (Float32) with a range from -3.4E38 to
                        3.4E38. However, the integer precision can be only
                        ensured between -16,777,216 and 16,777,216. If your
                        raster overpasses this range, it is strongly suggested
                        to use the type double instead.
 double                 a 64-bit float (Float64) with a range from -1.79E308 to
                        1.79E308. It is 8 bytes, 15-17 digits precision.
 null                   NULL value. Refer to section "NULL support" below.
```

Note that the value counter wraps around when the value overflows its
range. E.g., if your expression is `a = int(2147483648)`, you will get
NULL value. For expression `a = int(2147483649)`, you will reach the
lowest value possible instead, i.e. -2147483647.

### Floating point values in the expression

Floating point numbers are allowed in the expression. A floating point
number is a number which contains a decimal point:

```sh
    2.3   12.0   12.   .81
```

Floating point values in the expression are handled in a special way.
With arithmetic and logical operators, if either operand is float, the
other is converted to float and the result of the operation is float.
This means, in particular that division of integers results in a
(truncated) integer, while division of floats results in an accurate
floating point value. With functions of type \* (see table above), the
result is float if any argument is float, integer otherwise.

Note: If you calculate with integer numbers, the resulting map will be
integer. If you want to get a float result, add the decimal point to
integer number(s).

If you want floating point division, at least one of the arguments has
to be a floating point value. Multiplying one of them by 1.0 will
produce a floating-point result, as will using float():

```sh
      r.mapcalc "ndvi = float(lsat.4 - lsat.3) / (lsat.4 + lsat.3)"
```

### NULL support

- Division by zero should result in NULL.

- Modulus by zero should result in NULL.

- NULL-values in any arithmetic or logical operation should result in
  NULL. (however, &&& and \|\|\| are treated specially, as described
  below).

- The &&& and \|\|\| operators observe the following axioms even when x
  is NULL:

  ```sh
      x &&& false == false
      false &&& x == false
      x ||| true == true
      true ||| x == true
  ```

- NULL-values in function arguments should result in NULL (however,
  if(), eval() and isnull() are treated specially, as described below).

- The eval() function always returns its last argument

- The situation for if() is:

  ```sh
  if(x)
      NULL if x is NULL; 0 if x is zero; 1 otherwise
  if(x,a)
      NULL if x is NULL; a if x is non-zero; 0 otherwise
  if(x,a,b)
      NULL if x is NULL; a if x is non-zero; b otherwise
  if(x,n,z,p)
      NULL if x is NULL; n if x is negative;
  z if x is zero; p if x is positive
  ```

- The (new) function isnull(x) returns: 1 if x is NULL; 0 otherwise. The
  (new) function null() (which has no arguments) returns an integer
  NULL.

- Non-NULL, but invalid, arguments to functions should result in NULL.

  ```sh
  Examples:
  log(-2)
  sqrt(-2)
  pow(a,b) where a is negative and b is not an integer
  ```

NULL support: Please note that any math performed with NULL cells always
results in a NULL value for these cells. If you want to replace a NULL
cell on-the-fly, use the isnull() test function in a if-statement.

Example: The users wants the NULL-valued cells to be treated like zeros.
To add maps A and B (where B contains NULLs) to get a map C the user can
use a construction like:

```sh
C = A + if(isnull(B),0,B)
```

**NULL and conditions:**

For the one argument form:

```sh
if(x) = NULL        if x is NULL
if(x) = 0        if x = 0
if(x) = 1        otherwise (i.e. x is neither NULL nor 0).
```

For the two argument form:

```sh
if(x,a) = NULL        if x is NULL
if(x,a) = 0        if x = 0
if(x,a) = a        otherwise (i.e. x is neither NULL nor 0).
```

For the three argument form:

```sh
if(x,a,b) = NULL    if x is NULL
if(x,a,b) = b        if x = 0
if(x,a,b) = a        otherwise (i.e. x is neither NULL nor 0).
```

For the four argument form:

```sh
if(x,a,b,c) = NULL    if x is NULL
if(x,a,b,c) = a        if x > 0
if(x,a,b,c) = b        if x = 0
if(x,a,b,c) = c        if x < 0
```

More generally, all operators and most functions return NULL if \*any\*
of their arguments are NULL.  
The functions if(), isnull() and eval() are exceptions.  
The function isnull() returns 1 if its argument is NULL and 0 otherwise.
If the user wants the opposite, the ! operator, e.g. "!isnull(x)" must
be used.

All forms of if() return NULL if the first argument is NULL. The 2, 3
and 4 argument forms of if() return NULL if the "selected" argument is
NULL, e.g.:

```sh
if(0,a,b) = b    regardless of whether a is NULL
if(1,a,b) = a    regardless of whether b is NULL
```

eval() always returns its last argument, so it only returns NULL if the
last argument is NULL.

**Note**: The user cannot test for NULL using the == operator, as that
returns NULL if either or both arguments are NULL, i.e. if x and y are
both NULL, then "x == y" and "x != y" are both NULL rather than 1 and 0
respectively.  
The behaviour makes sense if the user considers NULL as representing an
unknown quantity. E.g. if x and y are both unknown, then the values of
"x == y" and "x != y" are also unknown; if they both have unknown
values, the user doesn't know whether or not they both have the same
value.

## NOTES

### Usage from command line

Extra care must be taken if the expression is given on the command line.
Some characters have special meaning to the UNIX shell. These include,
among others:

```sh
* ( ) > & |
```

It is advisable to put single quotes around the expression; e.g.:

```sh
'result = elevation * 2'
```

Without the quotes, the `*`, which has special meaning to the UNIX
shell, would be altered and *r.mapcalc* would see something other than
the `*`.

### Multiple computations

In general, it's preferable to do as much as possible in each r.mapcalc
command. E.g. rather than:

```sh
        r.mapcalc "$GIS_OPT_OUTPUT.r = r#$GIS_OPT_FIRST * .$GIS_OPT_PERCENT + (1.0 - .$GIS_OPT_PERCENT) * r#$GIS_OPT_SECOND"
        r.mapcalc "$GIS_OPT_OUTPUT.g = g#$GIS_OPT_FIRST * .$GIS_OPT_PERCENT + (1.0 - .$GIS_OPT_PERCENT) * g#$GIS_OPT_SECOND"
        r.mapcalc "$GIS_OPT_OUTPUT.b = b#$GIS_OPT_FIRST * .$GIS_OPT_PERCENT + (1.0 - .$GIS_OPT_PERCENT) * b#$GIS_OPT_SECOND"
```

use:

```sh
    r.mapcalc <<EOF
        $GIS_OPT_OUTPUT.r = r#$GIS_OPT_FIRST * .$GIS_OPT_PERCENT + (1.0 - .$GIS_OPT_PERCENT) * r#$GIS_OPT_SECOND
        $GIS_OPT_OUTPUT.g = g#$GIS_OPT_FIRST * .$GIS_OPT_PERCENT + (1.0 - .$GIS_OPT_PERCENT) * g#$GIS_OPT_SECOND
        $GIS_OPT_OUTPUT.b = b#$GIS_OPT_FIRST * .$GIS_OPT_PERCENT + (1.0 - .$GIS_OPT_PERCENT) * b#$GIS_OPT_SECOND
        EOF
```

as the latter will read each input map only once.

### Backwards compatibility

For the backwards compatibility with GRASS 6, if no options are given,
it manufactures `file=-` (which reads from stdin), so you can continue
to use e.g.:

```sh
r.mapcalc < file
```

or:

```sh
r.mapcalc <<EOF
foo = 1
EOF
```

But unless you need compatibility with previous GRASS GIS versions, use
`file=` explicitly, as stated above.

When the map name contains uppercase letter(s) or a dot which are not
allowed to be in module option names, the *r.mapcalc* command will be
valid also without quotes:

```sh
r.mapcalc elevation_A=1
r.mapcalc elevation.1=1
```

However, this syntax is not recommended as quotes as stated above more
safe. Using quotes is both backwards compatible and valid in future.

### Interactive input in command line

For formulas that the user enters from standard input (rather than from
the command line), a line continuation feature now exists. If the user
adds a backslash to the end of an input line, *r.mapcalc* assumes that
the formula being entered by the user continues on to the next input
line. There is no limit to the possible number of input lines or to the
length of a formula.

If the *r.mapcalc* formula entered by the user is very long, the map
title will contain only some of it, but most (if not all) of the formula
will be placed into the history file for the *result* map.

### Raster mask handling

*r.mapcalc* follows the common GRASS behavior of raster mask handling,
so the mask is only applied when reading an existing GRASS raster map.
This implies that, for example, the command:

```sh
r.mapcalc "elevation_exaggerated = elevation * 3"
```

create a map with NULL cells for the masked-out cells if raster mask is
active.

However, when creating a map which is not based on any map, e.g. a map
from a constant:

```sh
r.mapcalc "base_height = 200.0"
```

the created raster map is limited only by a computation region but it is
not affected by an active raster mask. This is expected because, as
mentioned above, the mask is only applied when reading, not when writing
a raster map. If the raster mask should be applied in this case, an
`if()` function including the mask raster should be used, e.g. (assuming
the mask is called `MASK`):

```sh
r.mapcalc "base_height = if(MASK, 200.0, null())"
```

When testing expressions related to mask handling keep in mind that when
the raster mask is active, you don't see data in masked areas even if
they are not NULL. See *[r.mask](r.mask.md)* for details.

### eval function

If the output of the computation should be only one map but the
expression is so complex that it is better to split it to several
expressions, the `eval` function can be used:

```sh
r.mapcalc << EOF
eval(elev_200 = elevation - 200, \
     elev_5 = 5 * elevation, \
     elev_p = pow(elev_5, 2))
elevation_result = (0.5 * elev_200) + 0.8 * elev_p
EOF
```

This example uses unix-like `<< EOF` syntax to provide input to
*r.mapcalc*.

Note that the temporary variables (maps) are not created and thus it
does not matter whether they exists or not. In the example above, if map
`elev_200` exists it will not be overwritten and no error will be
generated. The reason is that the name `elev_200` now denotes the
temporary variable (map) and not the existing map. The following parts
of the expression will use the temporary `elev_200` and the existing
`elev_200` will be left intact and will not be used. If a user want to
use the existing map, the name of the temporary variable (map) must be
changed.

### Using the same map for input and output results

A map cannot be used both as an input and as an output as in this
invalid expression `oldmap = oldmap + 1`, instead a subsequent rename
using *[g.rename](g.rename.md)* is needed when the same name is desired:

```sh
r.mapcalc "newmap = oldmap + 1"
g.rename raster=newmap,oldmap
```

### Random number generator initialization

The pseudo-random number generator used by the rand() function can be
initialised to a specific value using the **seed** option. This can be
used to replicate a previous calculation.

Alternatively, it can be initialised from the system time and the PID
using the **-r** flag. This should result in a different seed being used
each time.

In either case, the seed will be written to the map's history, and can
be seen using *r.info*.

If you want other people to be able to verify your results, it's
preferable to use the **seed** option to supply a seed which is either
specified in the script or generated from a deterministic process such
as a pseudo-random number generator given an explicit seed.

Note that the rand() function will generate a fatal error if neither the
**seed** option nor the **-s** flag are given.

## EXAMPLES

To compute the average of two raster map layers *a* and *b*:

```sh
ave = (a + b)/2
```

To form a weighted average:

```sh
ave = (5*a + 3*b)/8.0
```

To produce a binary representation of the raster map layer *a* so that
category 0 remains 0 and all other categories become 1:

```sh
mapmask = a != 0
```

This could also be accomplished by:

```sh
mapmask = if(a)
```

To mask raster map layer *b* by raster map layer *a*:

```sh
result = if(a,b)
```

To change all values below 5 to NULL, keep 5 otherwise:

```sh
newmap = if(map<5, null(), 5)
```

To create a map with random values in a defined range (needs either the
usage of **-s** flag or the *seed* parameter). The precision of the
input values determines the output precision (the resulting [raster map
type](rasterintro.md#raster-format)):

```sh
# write result as integer map (CELL)
random_int   = rand(-100,100)

# write result as double precision floating point map (DCELL)
random_dcell = rand(-100.0,100.0)

# write result as single precision floating point map (FCELL)
random_fcell = float(rand(-100.0,100.0))
```

The graph() function allows users to specify a x-y conversion using
pairs of x,y coordinates. In some situations a transformation from one
value to another is not easily established mathematically, but can be
represented by a 2-D graph and then linearly interpolated. The graph()
function provides the opportunity to accomplish this. An x-axis value is
provided to the graph function along with the associated graph
represented by a series of x,y pairs. The x values must be monotonically
increasing (each larger than or equal to the previous). The graph
function linearly interpolates between pairs. Any x value lower the
lowest x value (i.e. first) will have the associated y value returned.
Any x value higher than the last will similarly have the associated y
value returned. Consider the request:

```sh
newmap = graph(map, 1,10, 2,25, 3,50)
```

X (map) values supplied and y (newmap) values returned:

```sh
0, 10
1, 10
1.5, 17.5
2.9, 47.5
4, 50
100, 50
```

## KNOWN ISSUES

The *result* variable on the left hand side of the equation should not
appear in the *expression* on the right hand side.

```sh
mymap = if( mymap > 0, mymap, 0)
```

Any maps generated by a *r.mapcalc* command only exist after the entire
command has completed. All maps are generated concurrently, row-by-row
(i.e. there is an implicit "for row in rows {...}" around the entire
expression). Thus the `#`, `@`, and `[ ]` operators cannot be used on a
map generated within same *r.mapcalc* command run. Consequently, the
following (strikethrough code) does not work:

```sh
newmap = oldmap * 3.14
othermap = newmap[-1, 0] / newmap[1, 0]
```

Continuation lines must end with a `\` and have *no* trailing white
space (blanks or tabs). If the user does leave white space at the end of
continuation lines, the error messages produced by *r.mapcalc* will be
meaningless and the equation will not work as the user intended. This is
particularly important for the `eval()` function.

Currently, there is no comment mechanism in *r.mapcalc*. Perhaps adding
a capability that would cause the entire line to be ignored when the
user inserted a \# at the start of a line as if it were not present,
would do the trick.

The function should require the user to type "end" or "exit" instead of
simply a blank line. This would make separation of multiple scripts
separable by white space.

*r.mapcalc* does not print a warning in case of operations on NULL
cells. It is left to the user to utilize the `isnull()` function.

## REFERENCES

**[r.mapcalc: An Algebra for GIS and Image
Processing](https://grass.osgeo.org/gdp/raster/mapcalc-algebra.pdf)**,
by Michael Shapiro and Jim Westervelt, U.S. Army Construction
Engineering Research Laboratory (March/1991).

**[Performing Map Calculations on GRASS Data: r.mapcalc Program
Tutorial](https://grass.osgeo.org/history_docs/mapcalc.pdf)**, by Marji
Larson, Michael Shapiro and Scott Tweddale, U.S. Army Construction
Engineering Research Laboratory (December 1991)

Grey scale conversion is based on the C.I.E. x,y,z system where y
represents luminance. See "Fundamentals of Digital Image Processing," by
Anil K. Jain (Prentice Hall, NJ, 1989; p 67).

## SEE ALSO

*[g.region](g.region.md),
[r.bitpattern](https://grass.osgeo.org/grass-stable/manuals/addons/r.bitpattern.html),
[r.blend](r.blend.md), [r.colors](r.colors.md),
[r.fillnulls](r.fillnulls.md), [r.mapcalc.simple](r.mapcalc.simple.md)*

## AUTHORS

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory

Glynn Clements
