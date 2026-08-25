## DESCRIPTION

**t.rast.accdetect** is designed to detect accumulation pattern in
temporally accumulated space time raster datasets created by
[t.rast.accumulate](t.rast.accumulate.md). This module's input is a
space time raster dataset resulting from a
[t.rast.accumulate](t.rast.accumulate.md) run.

The **start** time and the **end** time of the pattern detection process
must be set, eg. **start="2000-03-01" end="2011-01-01"**. The **start**
and **end** time do not need to be the same as for the accumulation run
that produced the input space time raster dataset. In addition a
**cycle**, eg. "8 months", can be specified, that defines after which
time interval the accumulation pattern detection process restarts. The
**offset** option specifies the time that should be skipped between two
cycles, eg. "4 months". The **cycle** and **offset** options must be
exactly the same that were used in the accumulation process that
generated the input space time raster dataset, otherwise the
accumulation pattern detection will produce wrong results.

The **minimum** and **maximum** values for the pattern detection process
can be set either by using space time raster datasets or by using fixed
values for all raster cells and time steps.

Using space time raster datasets allows specifying minimum and maximum
values for each raster cell and each time step. For example, we want to
detect the germination (minimum value) and harvesting (maximum value)
dates for different crops in Germany using the growing-degree-day (GDD)
method for several years. Different crops may grow in different raster
cells and change with time because of crop rotation. Hence we need to
specify different GDD germination/harvesting (minimum/maximum) values
for different raster cells and different years.

The raster maps that specify the minimum and maximum values of the
actual granule will be detected using the following temporal relations:
equals, during, overlaps, overlapped and contains. First, all maps with
time stamps *equal* to the current granule of the input STRDS will be
detected, the first minimum map and the first maximum map that are found
will be used as range definitions. If no equal maps are found, then maps
with a temporal *during* relation will be detected, then maps that
temporally *overlap* the actual granules and finally, maps that have a
temporal *contain* relation will be detected. If no maps are found or
minimum/maximum STRDS are not set, then the **range** option is used,
eg. **range=480,730**.

The **base** name of of the generated maps must always be set.

This module produces two output space time raster datasets: occurrence
and indicator. The **occurrence** output STRDS stores the time in days
from the beginning of a given cycle for each raster cell and time step
that has a value within the minimum and maximum definition. These values
can be used to compute the duration of the recognized accumulation
pattern. The **indicator** output STRDS uses three integer values to
mark raster cells as beginning, intermediate state or end of an
accumulation pattern. By default, the module uses 1 to indicate the
start, 2 for the intermediate state and 3 to mark the end of the
accumulation pattern in a cycle. These default values can be changed
using the **staend** option.

## EXAMPLE

Please have a look at the [t.rast.accumulate](t.rast.accumulate.md)
example.

## SEE ALSO

*[t.rast.accumulate](t.rast.accumulate.md),
[t.rast.aggregate](t.rast.aggregate.md),
[t.rast.mapcalc](t.rast.mapcalc.md), [t.info](t.info.md),
[r.series.accumulate](r.series.accumulate.md), [g.region](g.region.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture
