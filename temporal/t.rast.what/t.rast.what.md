## DESCRIPTION

*t.rast.what* is designed to sample space time raster datasets at
specific point coordinates using [r.what](r.what.md) internally. The
output of [r.what](r.what.md) is transformed to different output
layouts. The output layouts can be specified using the *layout* option.

Three layouts can be specified:

- *row* - Row order, one vector sample point value per row
- *col* - Column order, create a column for each vector sample point of
  a single time step/raster layer
- *timerow* - Time order, create a column for each time step, this order
  is the original *r.what* output, except that the column names are the
  timestamps

Please have a look at the example to see the supported layouts.

This module is designed to run several instances of *r.what* to sample
subsets of a space time raster dataset in parallel. Several intermediate
text files will be created that are merged into a single file at the end
of the processing.

Coordinates can be provided as vector map using the *points* option or
as comma separated coordinate list with the *coordinates* option.

An output file can be specified using the *output* option. Stdout will
be used if no output is specified or if the *output* option is set to
"-".

## EXAMPLES

### Data preparation

In the following examples we sample a space time raster dataset that
contains 4 raster map layers. First we create the STRDS that will be
sampled with *t.rast.what*.

```sh
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10

# Generate data
r.mapcalc expression="a_1 = 1" -s
r.mapcalc expression="a_2 = 2" -s
r.mapcalc expression="a_3 = 3" -s
r.mapcalc expression="a_4 = 4" -s

t.create type=strds output=A title="A test" descr="A test"

t.register -i type=raster input=A maps=a_1,a_2,a_3,a_4 \
    start='1990-01-01' increment="1 month"
```

### Example 1

The first approach uses text coordinates as input and stdout as output,
the layout is one coordinate(point per column:

```sh
t.rast.what strds=A coordinates="115,36,79,45" layout=col -n

start|end|115.0000000000;36.0000000000|79.0000000000;45.0000000000
1990-01-01 00:00:00|1990-02-01 00:00:00|1|1
1990-02-01 00:00:00|1990-03-01 00:00:00|2|2
1990-03-01 00:00:00|1990-04-01 00:00:00|3|3
1990-04-01 00:00:00|1990-05-01 00:00:00|4|4
```

### Example 2

A vector map layer can be used as input to sample the STRDS. All three
available layouts are demonstrated using the vector map for sampling.

```sh
# First create the vector map layer based on random points
v.random output=points n=3 seed=1

# Row layout using a text file as output
t.rast.what strds=A points=points output=result.txt layout=row -n

cat result.txt

115.0043586274|36.3593955783|1990-01-01 00:00:00|1990-02-01 00:00:00|1
115.0043586274|36.3593955783|1990-02-01 00:00:00|1990-03-01 00:00:00|2
115.0043586274|36.3593955783|1990-03-01 00:00:00|1990-04-01 00:00:00|3
115.0043586274|36.3593955783|1990-04-01 00:00:00|1990-05-01 00:00:00|4
79.6816763826|45.2391522853|1990-01-01 00:00:00|1990-02-01 00:00:00|1
79.6816763826|45.2391522853|1990-02-01 00:00:00|1990-03-01 00:00:00|2
79.6816763826|45.2391522853|1990-03-01 00:00:00|1990-04-01 00:00:00|3
79.6816763826|45.2391522853|1990-04-01 00:00:00|1990-05-01 00:00:00|4
97.4892579600|79.2347263950|1990-01-01 00:00:00|1990-02-01 00:00:00|1
97.4892579600|79.2347263950|1990-02-01 00:00:00|1990-03-01 00:00:00|2
97.4892579600|79.2347263950|1990-03-01 00:00:00|1990-04-01 00:00:00|3
97.4892579600|79.2347263950|1990-04-01 00:00:00|1990-05-01 00:00:00|4


# Column layout order using stdout as output
t.rast.what strds=A points=points layout=col -n

start|end|115.0043586274;36.3593955783|79.6816763826;45.2391522853|97.4892579600;79.2347263950
1990-01-01 00:00:00|1990-02-01 00:00:00|1|1|1
1990-02-01 00:00:00|1990-03-01 00:00:00|2|2|2
1990-03-01 00:00:00|1990-04-01 00:00:00|3|3|3
1990-04-01 00:00:00|1990-05-01 00:00:00|4|4|4

# Timerow layout, one time series per row
# using the where statement to select a subset of the STRDS
# and stdout as output
t.rast.what strds=A points=points \
    where="start_time >= '1990-03-01'" layout=timerow -n

x|y|1990-03-01 00:00:00;1990-04-01 00:00:00|1990-04-01 00:00:00;1990-05-01 00:00:00
115.004358627375|36.3593955782903|3|4
79.681676382576|45.2391522852909|3|4
97.4892579600048|79.2347263950131|3|4
```

## SEE ALSO

*[g.region](g.region.md), [r.mask](r.mask.md)
[r.neighbors](r.neighbors.md), [r.what](r.what.md), [t.info](t.info.md),
[t.rast.aggregate.ds](t.rast.aggregate.ds.md),
[t.rast.extract](t.rast.extract.md), [v.what.strds](v.what.strds.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture
