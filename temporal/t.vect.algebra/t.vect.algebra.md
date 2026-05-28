## DESCRIPTION

*t.vect.algebra* performs temporal and spatial overlay and buffer
functions on space time vector datasets (STVDS) by using the temporal
vector algebra. New STVDS can be created, which are expressions of
existing STVDS.

The module expects an **expression** as input parameter in the following
form:

```sh
"result = expression"
```

The statement structure is similar to r.mapcalc, see
[r.mapcalc](r.mapcalc.md). Where **result** represents the name of a
space time dataset (STVDS) that will contain the result of the
calculation that is given as **expression** on the right side of the
equality sign. These expression can be any valid or nested combination
of temporal operations and functions that are provided by the temporal
vector algebra.  
The algebra provides methods for map selection from STDS based on their
temporal relations. It is also possible to temporally shift maps, to
create temporal buffer and to snap time instances to create a valid
temporal topology. Furthermore expressions can be nested and evaluated
in conditional statements (if, else statements). Within if-statements
the algebra provides temporal variables like start time, end time, day
of year, time differences or number of maps per time interval to build
up conditions. These operations can be assigned to space time datasets
or to the results of operations between space time datasets.

As default, topological relationships between space time datasets will
be evaluated only temporal. Use the **s** flag to activate the
additionally spatial topology evaluation.

The expression option must be passed as **quoted** expression, for
example:  

```sh
t.select expression="C = A : B"
```

Where **C** is the new space time raster dataset that will contain maps
from **A** that are selected by equal temporal relationships to the
existing dataset **B** in this case.

## TEMPORAL VECTOR ALGEBRA

The temporal algebra provides a wide range of temporal operators and
functions that will be presented in the following section.

### TEMPORAL RELATIONS

Several temporal topology relations between registered maps of space
time datasets are supported:  

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

over              booth overlaps and overlapped
```

The relations must be read as: A is related to B, like - A equals B - A
is during B - A contains B

Topological relations must be specified in {} parentheses.  

### TEMPORAL OPERATORS

The temporal algebra defines temporal operators that can be combined
with other operators to perform spatio-temporal operations. The temporal
operators process the time instances and intervals of two temporal
related maps and calculate the result temporal extent by five different
possibilities.

```sh
LEFT REFERENCE     l       Use the time stamp of the left space time dataset
INTERSECTION       i       Intersection
DISJOINT UNION     d       Disjoint union
UNION              u       Union
RIGHT REFERENCE    r       Use the time stamp of the right space time dataset
```

### TEMPORAL SELECTION

The temporal selection simply selects parts of a space time dataset
without processing raster or vector data. The algebra provides a
selection operator **:** that selects parts of a space time dataset that
are temporally equal to parts of a second one by default. The following
expression

```sh
C = A : B
```

means: Select all parts of space time dataset A that are equal to B and
store it in space time dataset C. The parts are time stamped maps.

In addition the inverse selection operator **!:** is defined as the
complement of the selection operator, hence the following expression

```sh
C = A !: B
```

means: select all parts of space time time dataset A that are not equal
to B and store it in space time dataset (STDS) C.

To select parts of a STDS by different topological relations to other
STDS, the temporal topology selection operator can be used. The operator
consists of the temporal selection operator, the topological relations,
that must be separated by the logical OR operator **\|** and the
temporal extent operator. All three parts are separated by comma and
surrounded by curly braces:

```sh
{"temporal selection operator", "topological relations", "temporal operator"}
```

Examples:

```sh
C = A {:, equals} B
C = A {!:, equals} B
```

We can now define arbitrary topological relations using the OR operator
"\|" to connect them:

```sh
C = A {:,equals|during|overlaps} B
```

Select all parts of A that are equal to B, during B or overlaps B.  
In addition we can define the temporal extent of the result STDS by
adding the temporal operator.

```sh
C = A {:, during,r} B
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

Selection operations can be evaluated within conditional statements.  
Note A and B can either be space time datasets or expressions. The
temporal relationship between the conditions and the conclusions can be
defined at the beginning of the if statement. The relationship between
then and else conclusion must be always equal.

```sh
if statement                           decision option                        temporal relations
  if(if, then, else)
  if(conditions, A)                    A if conditions are True;              temporal topological relation between if and then is equal.
  if(conditions, A, B)                 A if conditions are True, B otherwise; temporal topological relation between if, then and else is equal.
  if(topologies, conditions, A)        A if conditions are True;              temporal topological relation between if and then is explicit specified by topologies.
  if(topologies, conditions, A, B)     A if conditions are True, B otherwise; temporal topological relation between if, then and else is explicit specified by topologies.
```

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

The following temporal function are evaluated only for the STDS that
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

#### Comparison operator

The conditions are comparison expressions that are used to evaluate
space time datasets. Specific values of temporal variables are compared
by logical operators and evaluated for each map of the STDS and the
related maps. For complex relations the comparison operator can be used
to combine conditions:  
The structure is similar to the select operator with the extension of an
aggregation operator:

```sh
{"comparison operator", "topological relations", aggregation operator, "temporal operator"}
```

This aggregation operator (\| or &) define the behaviour if a map is
related the more than one map, e.g for the topological relations
'contains'. Should all (&) conditions for the related maps be true or is
it sufficient to have any (\|) condition that is true. The resulting
boolean value is then compared to the first condition by the comparison
operator (\|\| or &&). As default the aggregation operator is related to
the comparison operator:  
Comparison operator -\> aggregation operator:

```sh
|| -> | and && -> &
```

Examples:

```sh
Condition 1 {||, equal, r} Condition 2
Condition 1 {&&, equal|during, l} Condition 2
Condition 1 {&&, equal|contains, |, l} Condition 2
Condition 1 {&&, equal|during, l} Condition 2 && Condition 3
Condition 1 {&&, equal|during, l} Condition 2 {&&,contains, |, r} Condition 3
```

#### Hash operator

Additionally the number of maps in intervals can be computed and used in
conditional statements with the hash (#) operator.  

```sh
A{#, contains}B
```

This expression computes the number of maps from space time dataset B
which are during the time intervals of maps from space time dataset A.  
A list of integers (scalars) corresponding to the maps of A that contain
maps from B will be returned.

```sh
C = if({equal}, A {#, contains} B > 2, A {:, contains} B)
```

This expression selects all maps from A that temporally contains at
least 2 maps from B and stores them in space time dataset C. The leading
equal statement in the if condition specifies the temporal relation
between the if and then part of the if expression. This is very
important, so we do not need to specify a global time reference (a space
time dataset) for temporal processing.

Furthermore the temporal algebra allows temporal buffering, shifting and
snapping with the functions buff_t(), tshift() and tsnap() respectively.

```sh
buff_t(A, size)         Buffer STDS A with granule ("1 month" or 5)
tshift(A, size)         Shift STDS A with granule ("1 month" or 5)
tsnap(A)                Snap time instances and intervals of STDS A
```

#### Single map with temporal extent

The temporal algebra can also handle single maps with time stamps in the
tmap function.

```sh
tmap()
```

For example:

```sh
 C = A {:,during} tmap(event)
```

This statement select all maps from space time data set A that are
during the temporal extent of single map 'event'

### Spatial vector operators

The module supports the following boolean vector operations:  

```sh
 Boolean Name   Operator Meaning         Precedence   Correspondent function
----------------------------------------------------------------------------------
 AND            &        Intersection          1      (v.overlay operator=and)
 OR             |        Union                 1      (v.overlay operator=or)
 DISJOINT OR    +        Disjoint union        1      (v.patch)
 XOR            ^        Symmetric difference  1      (v.overlay operator=xor)
 NOT            ~        Complement            1      (v.overlay operator=not)
```

And vector functions:

```sh
 buff_p(A, size)          Buffer the points of vector map layer A with size
 buff_l(A, size)          Buffer the lines of vector map layer A with size
 buff_a(A, size)          Buffer the areas of vector map layer A with size
```

### Combinations of temporal, vector and select operators

We combine the temporal topology relations, the temporal operators and
the spatial/select operators to create spatio-temporal vector operators:

```code
{"spatial or select operator" , "list of temporal relations", "temporal operator" }
```

For multiple topological relations or several related maps the
spatio-temporal operators feature implicit aggregation. The algebra
evaluates the stated STDS by their temporal topologies and apply the
given spatio temporal operators in a aggregated form. If we have two
STDS A and B, B has three maps: b1, b2, b3 that are all during the
temporal extent of the single map a1 of A, then the following overlay
calculations would implicitly aggregate all maps of B into one result
map for a1 of A:

```sh
C = A {&, contains} B --> c1 = a1 & b1 & b2 & b3
```

Keep attention that the aggregation behaviour is not symmetric:

```sh
C = B {&, during} A --> c1 = b1 & a1
                        c2 = b2 & a1
                        c3 = b3 & a1
```

### Examples

Spatio-temporal intersect all maps from space time dataset A with all
maps from space time dataset B which have equal time stamps and are
temporary before Jan. 1. 2005 and store them in space time dataset D.

```sh
D = if(start_date(A) < "2005-01-01", A & B)
```

Buffer all vector points from space time vector dataset A and B with a
distance of one and intersect the results with overlapping, containing,
during and equal temporal relations to store the result in space time
vector dataset D with intersected time stamps.

```sh
D = buff_p(A, 1) {&,overlaps|overlapped|equal|during|contains,i} buff_p(B, 1)
```

Select all maps from space time dataset B which are during the temporal
buffered space time dataset A with a map interval of three days, else
select maps from C and store them in space time dataset D.

```sh
D = if(contains, td(buff_t(A, "1 days")) == 3, B, C)
```

## REFERENCES

[PLY(Python-Lex-Yacc)](https://www.dabeaz.com/ply/)

## SEE ALSO

*[t.select](t.select.md)*

## AUTHORS

Thomas Leppelt, Soeren Gebbert, Thünen Institute of Climate-Smart
Agriculture
