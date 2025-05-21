## DESCRIPTION

The purpose of *t.sample* is to compute and to show spatio-temporal
relations between space time datasets of different type. Several input
space time datasets are sampled by a *sample* space time dataset using
temporal topological relations. The types of the input space time
datasets and the type of the sample space time dataset can be different.

This module is useful to analyze temporal relationships between space
time datasets using temporal topology. The flag *-s* enables a
spatio-temporal topology, so that only spatio-temporal related map
layers of space time datasets are considered in the analysis.

## NOTES

The temporal relation *start* means that the start time of an input map
layer is temporally located in an interval of a sample map layer.

The textual output at the command line shows the names of the maps,
start and end time as well as the *interval length* in days and the
temporal *distance from begin* in days.

The default *separator* is the pipe symbol.

Temporal gaps, if present, in the input and sampling space time datasets
will be used in the sampling process. Gaps have no map name, instead
*None* is printed.

## EXAMPLE

In the examples below we create a space time raster dataset *A* and a
space time vector dataset *P* that have different temporal layouts and
number of map layers. The space time vector dataset contains a gap, that
will be used in the sampling process.

We use *t.sample* to inspect the topological relations between the time
stamped map layers in *A* and *P*.

```sh
# Set an appropriate region
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

# Generate the raster map layer
r.mapcalc expression="a1 = rand(0, 550)" -s
r.mapcalc expression="a2 = rand(0, 450)" -s
r.mapcalc expression="a3 = rand(0, 320)" -s
r.mapcalc expression="a4 = rand(0, 510)" -s
r.mapcalc expression="a5 = rand(0, 300)" -s
r.mapcalc expression="a6 = rand(0, 650)" -s

# Generate the vector map layer
v.random -z output=pnts1 n=20 zmin=0 zmax=100 column=height
v.random -z output=pnts2 n=20 zmin=0 zmax=100 column=height

n1=`g.tempfile pid=1 -d`
n2=`g.tempfile pid=2 -d`

cat > "${n1}" << EOF
a1
a2
a3
a4
a5
a6
EOF

cat > "${n2}" << EOF
pnts1|2001-01-01|2001-03-01
pnts2|2001-05-01|2001-07-01
EOF

# Register the maps in new space time datasets
t.create type=strds temporaltype=absolute output=A \
    title="A test with raster input files" descr="A test with raster input files"

t.create type=stvds temporaltype=absolute output=P \
    title="A test with vector input files" descr="A test with vector input files"

t.register type=raster -i input=A file="${n1}" start="2001-01-01" increment="1 months"

# Raster map layer in A
t.rast.list A

name|mapset|start_time|end_time
a1|PERMANENT|2001-01-01 00:00:00|2001-02-01 00:00:00
a2|PERMANENT|2001-02-01 00:00:00|2001-03-01 00:00:00
a3|PERMANENT|2001-03-01 00:00:00|2001-04-01 00:00:00
a4|PERMANENT|2001-04-01 00:00:00|2001-05-01 00:00:00
a5|PERMANENT|2001-05-01 00:00:00|2001-06-01 00:00:00
a6|PERMANENT|2001-06-01 00:00:00|2001-07-01 00:00:00


t.register type=vector input=P file="${n2}"

# Vector map layer in P
t.vect.list P

name|layer|mapset|start_time|end_time
pnts1|None|PERMANENT|2001-01-01 00:00:00|2001-03-01 00:00:00
pnts2|None|PERMANENT|2001-05-01 00:00:00|2001-07-01 00:00:00

# Start time of maps in A located in maps in P
t.sample method=start input=A samtype=stvds sample=P -c

P@PERMANENT|A@PERMANENT|start_time|end_time|interval_length|distance_from_begin
pnts1@PERMANENT|a1@PERMANENT,a2@PERMANENT|2001-01-01 00:00:00|2001-03-01 00:00:00|59.0|0.0
None|a3@PERMANENT,a4@PERMANENT|2001-03-01 00:00:00|2001-05-01 00:00:00|61.0|59.0
pnts2@PERMANENT|a5@PERMANENT,a6@PERMANENT|2001-05-01 00:00:00|2001-07-01 00:00:00|61.0|120.0


# P contains A
t.sample method=contain input=A samtype=stvds sample=P -c

P@PERMANENT|A@PERMANENT|start_time|end_time|interval_length|distance_from_begin
pnts1@PERMANENT|a1@PERMANENT,a2@PERMANENT|2001-01-01 00:00:00|2001-03-01 00:00:00|59.0|0.0
None|a3@PERMANENT,a4@PERMANENT|2001-03-01 00:00:00|2001-05-01 00:00:00|61.0|59.0
pnts2@PERMANENT|a5@PERMANENT,a6@PERMANENT|2001-05-01 00:00:00|2001-07-01 00:00:00|61.0|120.0


# A during P
t.sample method=during intype=stvds input=P samtype=strds sample=A -c

A@PERMANENT|P@PERMANENT|start_time|end_time|interval_length|distance_from_begin
a1@PERMANENT|pnts1@PERMANENT|2001-01-01 00:00:00|2001-02-01 00:00:00|31.0|0.0
a2@PERMANENT|pnts1@PERMANENT|2001-02-01 00:00:00|2001-03-01 00:00:00|28.0|31.0
a3@PERMANENT|None|2001-03-01 00:00:00|2001-04-01 00:00:00|31.0|59.0
a4@PERMANENT|None|2001-04-01 00:00:00|2001-05-01 00:00:00|30.0|90.0
a5@PERMANENT|pnts2@PERMANENT|2001-05-01 00:00:00|2001-06-01 00:00:00|31.0|120.0
a6@PERMANENT|pnts2@PERMANENT|2001-06-01 00:00:00|2001-07-01 00:00:00|30.0|151.0


# No Overlapping
t.sample method=overlap input=A samtype=stvds sample=P -cs

P@PERMANENT|A@PERMANENT|start_time|end_time|interval_length|distance_from_begin
pnts1@PERMANENT|None|2001-01-01 00:00:00|2001-03-01 00:00:00|59.0|0.0
None|None|2001-03-01 00:00:00|2001-05-01 00:00:00|61.0|59.0
pnts2@PERMANENT|None|2001-05-01 00:00:00|2001-07-01 00:00:00|61.0|120.0


t.sample method=precedes input=A samtype=stvds sample=P -c

P@PERMANENT|A@PERMANENT|start_time|end_time|interval_length|distance_from_begin
pnts1@PERMANENT|a3@PERMANENT|2001-01-01 00:00:00|2001-03-01 00:00:00|59.0|0.0
None|a5@PERMANENT|2001-03-01 00:00:00|2001-05-01 00:00:00|61.0|59.0
pnts2@PERMANENT|None|2001-05-01 00:00:00|2001-07-01 00:00:00|61.0|120.0


t.sample method=follows  input=A samtype=stvds sample=P -c

P@PERMANENT|A@PERMANENT|start_time|end_time|interval_length|distance_from_begin
pnts1@PERMANENT|None|2001-01-01 00:00:00|2001-03-01 00:00:00|59.0|0.0
None|a2@PERMANENT|2001-03-01 00:00:00|2001-05-01 00:00:00|61.0|59.0
pnts2@PERMANENT|a4@PERMANENT|2001-05-01 00:00:00|2001-07-01 00:00:00|61.0|120.0


t.sample method=precedes,follows input=A samtype=stvds sample=P -c

P@PERMANENT|A@PERMANENT|start_time|end_time|interval_length|distance_from_begin
pnts1@PERMANENT|a3@PERMANENT|2001-01-01 00:00:00|2001-03-01 00:00:00|59.0|0.0
None|a5@PERMANENT,a2@PERMANENT|2001-03-01 00:00:00|2001-05-01 00:00:00|61.0|59.0
pnts2@PERMANENT|a4@PERMANENT|2001-05-01 00:00:00|2001-07-01 00:00:00|61.0|120.0
```

## SEE ALSO

*[t.create](t.create.md), [t.info](t.info.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture
