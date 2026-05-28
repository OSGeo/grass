## DESCRIPTION

The module *t.remove* removes space time datasets (STRDS, STR3DS, STVDS)
from the temporal database. In other words, by default it deletes the
relevant database entries. It can also unregister maps from temporal
database using the recursive mode **-r** (recursive).

Optionally, also the raster, 3D raster and vector maps of the space time
datasets can be removed from the current mapset using the **-d**
(delete) flag. All removals only work if **-f** (force) flag is used.

## EXAMPLE

In this example a space time raster dataset (STRDS) named
**precip_months_sum** will be created using a subset of the monthly
precipitation raster maps from the North Carolina climate sample data
set.  
In order to be able to show the case of recursive removal without
deleting the original sample data, we generate new data by means of
computing yearly precipitation sums. Eventually, all newly produced data
(STRDS and raster maps) are removed:

```sh
# Create new and empty STRDS
t.create output=precip_months_sum semantictype=mean \
  title="Monthly sum of precipitation" \
  description="Monthly sum of precipitation for the \
  North Carolina sample data"

# Register maps from sample dataset (selecting a subset with g.list)
t.register -i type=raster input=precip_months_sum \
  maps=$(g.list type=raster pattern="201*_precip" separator=comma) \
  start="2010-01-01" increment="1 months"

# Create some new data by aggregating with 1 years granularity
t.rast.aggregate input=precip_months_sum \
  output=precip_years_sum basename=precip_years_sum \
  granularity="1 years" method=sum

# Remove all newly produced data:

# a) the aggregated STRDS with 1 years granularity along with its raster maps
t.remove -df type=strds input=precip_years_sum

# b) the STRDS with 1 months granularity, but not the original sample data
t.remove -f type=strds input=precip_months_sum
```

## SEE ALSO

*[t.create](t.create.md), [t.info](t.info.md),
[t.register](t.register.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture
