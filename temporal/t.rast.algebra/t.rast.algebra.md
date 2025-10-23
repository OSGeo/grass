## DESCRIPTION

*t.rast.algebra* performs temporal and spatial map algebra operations on
space time raster datasets (STRDS) using the temporal raster algebra.

The module expects an **expression** as input parameter in the following
form:

```sh
"result = expression"
```

The statement structure is similar to that of [r.mapcalc](r.mapcalc.md).
In this statement, **result** represents the name of the space time
raster dataset (STRDS) that will contain the result of the calculation
that is given as **expression** on the right side of the equality sign.
These expressions can be any valid or nested combination of temporal
operations and spatial overlay or buffer functions that are provided by
the temporal algebra.

The temporal raster algebra works only with space time raster datasets
(STRDS). The algebra provides methods for map selection based on their
temporal relations. It is also possible to temporally shift maps, to
create temporal buffer and to snap time instances to create a valid
temporal topology. Furthermore, expressions can be nested and evaluated
in conditional statements (if, else statements). Within if-statements,
the algebra provides temporal variables like start time, end time, day
of year, time differences or number of maps per time interval to build
up conditions.  
In addition the algebra provides a subset of the spatial operations from
[r.mapcalc](r.mapcalc.md). All these operations can be assigned to STRDS
or to the map lists resulting of operations between STRDS.

By default, only temporal topological relations among space time
datasets (STDS) are evaluated. The **-s** flag can be used to
additionally activate the evaluation of the spatial topology based on
the spatial extent of maps.

The expression option must be passed as **quoted** expression, for
example:

```sh
t.rast.algebra expression="C = A + B" basename=result
```

Where **C** is the new space time raster dataset that will contain maps
with the basename "result" and a numerical suffix separated by an
underscore that represent the sum of maps from the STRDS **A** and
temporally equal maps (i.e., maps with equal temporal topology relation)
from the STRDS **B**.

The map **basename** for the result STRDS must always be specified.

## TEMPORAL RASTER ALGEBRA

The temporal algebra provides a wide range of temporal operators and
functions that will be presented in the following section.

### TEMPORAL RELATIONS

Several temporal topology relations are supported between maps
registered in space time datasets:

```sh
equals            A ------
                  B ------

during            A  ----
                  B ------

contains          A ------
                  B  ----

starts            A ----
                  B ------

started           A ------
                  B ----

finishes          A   ----
                  B ------

finished          A ------
                  B   ----

precedes          A ----
                  B     ----

follows           A     ----
                  B ----

overlapped        A   ------
                  B ------

overlaps          A ------
                  B   ------

over              both overlaps and overlapped
```

The relations must be read as: A is related to B, like - A equals B - A
is during B - A contains B.

Topological relations must be specified with curly brackets {}.

### TEMPORAL OPERATORS

The temporal algebra defines temporal operators that can be combined
with other operators to perform spatio-temporal operations. The temporal
operators process the time instances and intervals of two temporally
related maps and calculate the resulting temporal extent in five
possible different ways.

```sh
LEFT REFERENCE     l       Use the time stamp of the left space time dataset
INTERSECTION       i       Intersection
DISJOINT UNION     d       Disjoint union
UNION              u       Union
RIGHT REFERENCE    r       Use the time stamp of the right space time dataset
```

### TEMPORAL SELECTION

The temporal selection simply selects parts of a space time dataset
without processing any raster or vector data. The algebra provides a
selection operator **:** that by default selects parts of a space time
dataset that are temporally equal to parts of a second space time
dataset. The following expression

```sh
C = A : B
```

means: select all parts of space time dataset A that are equal to B and
store them in space time dataset C. These parts are time stamped maps.

In addition, the inverse selection operator **!:** is defined as the
complement of the selection operator, hence the following expression

```sh
C = A !: B
```

means: select all parts of space time time dataset A that are not equal
to B and store them in space time dataset C.

To select parts of a STRDS using different topological relations
regarding to other STRDS, the temporal topology selection operator can
be used. This operator consists of the temporal selection operator, the
topological relations that must be separated by the logical OR operator
**\|** and, the temporal extent operator. All three parts are separated
by comma and surrounded by curly brackets as follows: {"temporal
selection operator", "topological relations", "temporal operator"}.

**Examples:**

```sh
C = A {:,equals} B
C = A {!:,equals} B
```

We can now define arbitrary topological relations using the OR operator
"\|" to connect them:

```sh
C = A {:,equals|during|overlaps} B
```

Select all parts of A that are equal to B, during B or overlaps B.  
In addition, we can define the temporal extent of the resulting STRDS by
adding the temporal operator.

```sh
C = A {:,during,r} B
```

Select all parts of A that are during B and use the temporal extents
from B for C.  
  
The selection operator is implicitly contained in the temporal topology
selection operator, so that the following statements are exactly the
same:

```sh
C = A : B
C = A {:} B
C = A {:,equal} B
C = A {:,equal,l} B
```

Same for the complementary selection:

```sh
C = A !: B
C = A {!:} B
C = A {!:,equal} B
C = A {!:,equal,l} B
```

### CONDITIONAL STATEMENTS

Selection operations can be evaluated within conditional statements as
showed below. Note that A and B can be either space time datasets or
expressions. The temporal relationship between the conditions and the
conclusions can be defined at the beginning of the if statement (third
and fourth examples below). The relationship between then and else
conclusion must be always equal.

```sh
if statement                        decision option                        temporal relations
  if(if, then, else)
  if(conditions, A)                   A if conditions are True;              temporal topological relation between if and then is equal.
  if(conditions, A, B)                A if conditions are True, B otherwise; temporal topological relation between if, then and else is equal.
  if(topologies, conditions, A)       A if conditions are True;              temporal topological relation between if and then is explicitly specified by topologies.
  if(topologies, conditions, A, B)    A if conditions are True, B otherwise; temporal topological relation between if, then and else is explicitly specified by topologies.
```

The conditions are comparison expressions that are used to evaluate
space time datasets. Specific values of temporal variables are compared
by logical operators and evaluated for each map of the STRDS.  
**Important:** The conditions are evaluated from left to right.

#### Logical operators

```sh
Symbol  description

  ==    equal
  !=    not equal
  >     greater than
  >=    greater than or equal
  <     less than
  <=    less than or equal
  &&    and
  ||    or
```

#### Temporal functions

The following temporal functions are evaluated only for the STDS that
must be given in parenthesis.

```sh
td(A)                    Returns a list of time intervals of STDS A

start_time(A)            Start time as HH::MM:SS
start_date(A)            Start date as yyyy-mm-DD
start_datetime(A)        Start datetime as yyyy-mm-DD HH:MM:SS
end_time(A)              End time as HH:MM:SS
end_date(A)              End date as yyyy-mm-DD
end_datetime(A)          End datetime as  yyyy-mm-DD HH:MM

start_doy(A)             Day of year (doy) from the start time [1 - 366]
start_dow(A)             Day of week (dow) from the start time [1 - 7], the start of the week is Monday == 1
start_year(A)            The year of the start time [0 - 9999]
start_month(A)           The month of the start time [1 - 12]
start_week(A)            Week of year of the start time [1 - 54]
start_day(A)             Day of month from the start time [1 - 31]
start_hour(A)            The hour of the start time [0 - 23]
start_minute(A)          The minute of the start time [0 - 59]
start_second(A)          The second of the start time [0 - 59]
end_doy(A)               Day of year (doy) from the end time [1 - 366]
end_dow(A)               Day of week (dow) from the end time [1 - 7], the start of the week is Monday == 1
end_year(A)              The year of the end time [0 - 9999]
end_month(A)             The month of the end time [1 - 12]
end_week(A)              Week of year of the end time [1 - 54]
end_day(A)               Day of month from the start time [1 - 31]
end_hour(A)              The hour of the end time [0 - 23]
end_minute(A)            The minute of the end time [0 - 59]
end_second(A)            The second of the end time [0 - 59]
```

In order to use the numbers returned by the functions in the last block
above, an offset value needs to be added. For example, start_doy(A, 0)
would return the DOY of the current map in STDS A. end_hour(A, -1) would
return the end hour of the previous map in STDS A.

#### Comparison operator

As mentioned above, the conditions are comparison expressions that are
used to evaluate space time datasets. Specific values of temporal
variables are compared by logical operators and evaluated for each map
of the STDS and (optionally) related maps. For complex relations, the
comparison operator can be used to combine conditions.  
The structure is similar to the select operator with the addition of an
aggregation operator: {"comparison operator", "topological relations",
aggregation operator, "temporal operator"}  
This aggregation operator (\| or &) defines the behaviour when a map is
related to more than one map, e.g. for the topological relation
'contains'. Should all (&) conditions for the related maps be true or is
it sufficient to have any (\|) condition that is true. The resulting
boolean value is then compared to the first condition by the comparison
operator (\|\| or &&). By default, the aggregation operator is related
to the comparison operator:  
comparison operator -\> aggregation operator:

```sh
|| -> | and && -> &
```

**Examples:**

```sh
Condition 1 {||, equal, r} Condition 2
Condition 1 {&&, equal|during, l} Condition 2
Condition 1 {&&, equal|contains, |, l} Condition 2
Condition 1 {&&, equal|during, l} Condition 2 && Condition 3
Condition 1 {&&, equal|during, l} Condition 2 {&&,contains, |, r} Condition 3
```

#### Hash operator

Additionally, the number of maps in intervals can be computed and used
in conditional statements with the hash (#) operator.

```sh
A {#, contains} B
```

This expression computes the number of maps from space time dataset B
which are during the time intervals of maps from space time dataset A.  
A list of integers (scalars) corresponding to the maps of A that contain
maps from B will be returned.

```sh
C = if({equal}, A {#, contains} B > 2, A {:, contains} B)
```

This expression selects all maps from A that temporally contain at least
2 maps from B and stores them in space time dataset C. The leading equal
statement in the if condition specifies the temporal relation between
the if and then part of the if expression. This is very important, so we
do not need to specify a global time reference (a space time dataset)
for temporal processing.

Furthermore, the temporal algebra allows temporal buffering, shifting
and snapping with the functions buff_t(), tshift() and tsnap(),
respectively.

```sh
buff_t(A, size)         Buffer STDS A with granule ("1 month" or 5)
tshift(A, size)         Shift STDS A with granule ("1 month" or 5)
tsnap(A)                Snap time instances and intervals of STDS A
```

#### Single map with temporal extent

The temporal algebra can also handle single maps with time stamps in the
tmap() function.

```sh
tmap()
```

For example:

```sh
C = A {:, during} tmap(event)
```

This statement selects all maps from space time data set A that are
during the temporal extent of the single map 'event'

### Spatial raster operators

The module supports the following raster operations:

```sh
Symbol  description     precedence

  %     modulus         1
  /     division        1
  *     multiplication  1
  +     addition        2
  -     subtraction     2
```

And raster functions:

```sh
abs(x)                  return absolute value of x
float(x)                convert x to foating point
int(x)                  convert x to integer [ truncates ]
log(x)                  natural log of x
sqrt(x)                 square root of x
tan(x)                  tangent of x (x is in degrees)
round(x)                round x to nearest integer
sin(x)                  sine of x (x is in degrees)
isnull(x)               check if x = NULL
isntnull(x)             check if x is not NULL
null                    set null value
exist(x)                Check if x is in the current mapset
```

#### Single raster map

The temporal raster algebra features also a function to integrate single
raster maps without time stamps into the expressions.

```sh
map()
```

For example:

```sh
C = A * map(constant_value)
```

This statement multiplies all raster maps from space time raster data
set A with the raster map 'constant_value'

### Combinations of temporal, raster and select operators

The user can combine the temporal topology relations, the temporal
operators and the spatial/select operators to create spatio-temporal
operators as follows:

```sh
{"spatial or select operator", "list of temporal relations", "temporal operator"}
```

For multiple topological relations or several related maps the
spatio-temporal operators feature implicit aggregation. The algebra
evaluates the stated STDS by their temporal topologies and apply the
given spatio-temporal operators in a aggregated form. If we have two
STDS A and B, B has three maps: b1, b2, b3 that are all during the
temporal extent of the single map a1 of A, then the following arithmetic
calculations would implicitly aggregate all maps of B into one result
map for a1 of A:

```code
 C = A {+, contains} B --> c1 = a1 + b1 + b2 + b3
```

**Important**: the aggregation behaviour is not symmetric

```code
 C = B {+, during} A --> c1 = b1 + a1
                         c2 = b2 + a1
                         c3 = b3 + a1
```

### Temporal neighbourhood modifier

The neighbourhood modifier of *r.mapcalc* is extended for the temporal
raster algebra with the temporal dimension. The format is
strds\[t,r,c\], where t is the temporal offset, r is the row offset and
c is the column offset. A single neighborhood modifier is interpreted as
temporal offset \[t\], while two neighborhood modifiers are interpreted
as row and column offsets \[r,c\].

```code
strds[2]
```

refers to the second successor of the current map.

```code
strds[1,2]
```

refers to the cell one row below and two columns to the right of the
current cell in the current map.

```code
strds[1,-2,-1]
```

refers to the cell two rows above and one column to the left of the
current cell of the first successor map.

```code
strds[-2,0,1]
```

refers to the cell one column to the right of the current cell in the
second predecessor map.

## EXAMPLES

### Computation of NDVI

```sh
# Sentinel-2 bands are stored separately in two STDRS "S2_b4" and "S2_b8"
g.region raster=sentinel2_B04_10m -p
t.rast.list S2_b4
t.rast.list S2_b8
t.rast.algebra basename=ndvi expression="ndvi = float(S2_b8 - S2_b4) / ( S2_b8 + S2_b4 )"
t.rast.colors input=ndvi color=ndvi
```

### Sum of space-time raster datasets

Sum maps from STRDS A with maps from STRDS B which have equal time
stamps and are temporally before Jan. 1. 2005 and store them in STRDS D:

```sh
D = if(start_date(A) < "2005-01-01", A + B)
```

Create the sum of all maps from STRDS A and B that have equal time
stamps and store the new maps in STRDS C:

```sh
C = A + B
```

### Sum of space-time raster datasets with temporal topology relation

Same expression with explicit definition of the temporal topology
relation and temporal operators:

```sh
C = A {+,equal,l} B
```

### Selection of raster cells

Select all cells from STRDS B with equal temporal relations to STRDS A,
if the cells of A are in the range \[100.0, 1600\] of time intervals
that have more than 30 days (Jan, Mar, May, Jul, Aug, Oct, Dec):

```sh
C = if(A > 100 && A < 1600 && td(A) > 30, B)
```

### Selection of raster cells with temporal topology relation

Same expression with explicit definition of the temporal topology
relation and temporal operators:

```sh
C = if({equal}, A > 100 && A < 1600 {&&,equal} td(A) > 30, B)
```

### Conditional computation

Compute the recharge in meters per second for all cells of precipitation
STRDS "Prec" if the mean temperature specified in STRDS "Temp" is higher
than 10 degrees. Computation is performed if STRDS "Prec" and "Temp"
have equal time stamps. The number of days or fraction of days per
interval is computed using the td() function that has as argument the
STRDS "Prec":

```sh
C = if(Temp > 10.0, Prec / 3600.0 / 24.0 / td(Prec))
```

### Conditional computation with temporal topology relation

Same expression with explicit definition of the temporal topology
relation and temporal operators:

```sh
C = if({equal}, Temp > 10.0, Prec / 3600.0 / 24.0 {/,equal,l} td(Prec))
```

### Computation with time intervals

Compute the mean value of all maps from STRDS A that are located during
time intervals of STRDS B if more than one map of A is contained in an
interval of B, use A otherwise. The resulting time intervals are either
from B or A:

```sh
C = if(B {#,contain} A > 1, (B {+,contain,l} A - B) / (B {#,contain} A), A)
```

### Computation with time intervals with temporal topology relation

Same expression with explicit definition of the temporal topology
relation and temporal operators:

```sh
C = if({equal}, B {#,contain} A > 1, (B {+,contain,l} A {-,equal,l} B) {equal,=/} (B {#,contain} A), A)
```

### Compute DOY for spatio-temporal conditions

Compute the DOY for all maps from STRDS A where conditions are met at
three consecutive time intervals (e.g. temperature \> 0):

```sh
B = if(A > 0.0 && A[-1] > 0.0 && A[-2] > 0.0, start_doy(A, -1), 0)"
```

## SEE ALSO

*[r.mapcalc](r.mapcalc.md), [t.vect.algebra](t.vect.algebra.md),
[t.rast3d.algebra](t.rast3d.algebra.md), [t.select](t.select.md),
[t.rast3d.mapcalc](t.rast3d.mapcalc.md),
[t.rast.mapcalc](t.rast.mapcalc.md)*

[Temporal data processing
Wiki](https://grasswiki.osgeo.org/wiki/Temporal_data_processing)

## REFERENCES

The use of this module requires the following software to be installed:
[PLY(Python-Lex-Yacc)](https://www.dabeaz.com/ply/)

```sh
# Ubuntu/Debian
sudo apt-get install python3-ply

# Fedora
sudo dnf install python3-ply

# MS-Windows (OSGeo4W: requires "python3-pip" package to be installed)
python3-pip install ply
```

Related publications:

- Gebbert, S., Pebesma, E. 2014. *TGRASS: A temporal GIS for field based
  environmental modeling*. Environmental Modelling & Software 53, 1-12
  ([DOI](https://doi.org/10.1016/j.envsoft.2013.11.001)) - [preprint
  PDF](http://ifgi.uni-muenster.de/~epebe_01/tgrass.pdf)
- Gebbert, S., Pebesma, E. 2017. *The GRASS GIS temporal framework*.
  International Journal of Geographical Information Science 31,
  1273-1292 ([DOI](https://doi.org/10.1080/13658816.2017.1306862))
- Gebbert, S., Leppelt, T., Pebesma, E., 2019. *A topology based
  spatio-temporal map algebra for big data analysis*. Data 4, 86.
  ([DOI](https://doi.org/10.3390/data4020086))

## AUTHORS

Thomas Leppelt, Sören Gebbert, Thünen Institute of Climate-Smart
Agriculture
