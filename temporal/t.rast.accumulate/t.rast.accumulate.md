## DESCRIPTION

*t.rast.accumulate* is designed to perform temporal accumulations of
space time raster datasets. This module expects a space time raster
dataset as input that will be sampled by a given **granularity**. All
maps that have the start time during the actual granule will be
accumulated with the predecessor granule accumulation result using the
raster module [r.series.accumulate](r.series.accumulate.md). The default
granularity is 1 day, but any temporal granularity can be set.

The **start** time and the **end** time of the accumulation process must
be set, eg. **start="2000-03-01" end="2011-01-01"**. In addition, a
**cycle**, eg. **cycle="8 months"**, can be specified, that defines
after which interval of time the accumulation process restarts. The
**offset** option specifies the time that should be skipped between two
cycles, eg. **offset="4 months"**.

The **lower** and **upper** **limits** of the accumulation process can
be set, either by using space time raster datasets or by using fixed
values for all raster cells and time steps. The raster maps that specify
the lower and upper limits of the actual granule will be detected using
the following temporal relations: equals, during, overlaps, overlapped
and contains. First, all maps with time stamps equal to the current
granule will be detected, the first lower map and the first upper map
found will be used as limit definitions. If no equal maps are found,
then maps with a temporal during relation are detected, then maps that
temporally overlap the actual granules, until maps that have a temporal
contain relation are detected. If no maps are found or lower/upper STRDS
are not defined, then the **limits** option is used, eg.
**limits=10,30**.

The **upper** **limit** is only used in the Biologically Effective
Degree Days calculation.

The options **shift**, **scale** and **method** are passed to
[r.series.accumulate](r.series.accumulate.md). Please refer to the
manual page of [r.series.accumulate](r.series.accumulate.md) for
detailed option description.

The **output** is a new space time raster dataset with the provided
start time, end time and granularity containing the accumulated raster
maps. The **base** name of the generated maps must always be set. The
**output** space time raster dataset can then be analyzed using
[t.rast.accdetect](t.rast.accdetect.md) to detect specific accumulation
patterns.

## EXAMPLE

This is an example how to accumulate the daily mean temperature of
Europe from 1990 to 2000 using the growing-degree-day method to detect
grass hopper reproduction cycles that are critical to agriculture.

```sh
# Get the temperature data
wget http://www-pool.math.tu-berlin.de/~soeren/grass/temperature_mean_1990_2000_daily_celsius.tar.gz

# Create a temporary project directory
mkdir -p /tmp/grassdata/LL

# Start GRASS and create a new project with PERMANENT mapset
grass -c EPSG:4326 /tmp/grassdata/LL/PERMANENT

# Import the temperature data
t.rast.import input=temperature_mean_1990_2000_daily_celsius.tar.gz \
    output=temperature_mean_1990_2000_daily_celsius directory=/tmp

# We need to set the region correctly
g.region -p raster=`t.rast.list input=temperature_mean_1990_2000_daily_celsius column=name | tail -1`
# We can zoom to the raster map
g.region -p zoom=`t.rast.list input=temperature_mean_1990_2000_daily_celsius column=name | tail -1`

#############################################################################
#### ACCUMULATION USING GDD METHOD ##########################################
#############################################################################
# The computation of grashopper pest control cycles is based on:
#
#   Using Growing Degree Days For Insect Management
#   Nancy E. Adams
#   Extension Educator, Agricultural Resources
#
# available here: http://extension.unh.edu/agric/gddays/docs/growch.pdf

# Now we compute the Biologically Effective Degree Days
# from 1990 - 2000 for each year (12 month cycle) with
# a granularity of one day. Base temperature is 10°C, upper limit is 30°C.
# Hence the accumulation starts at 10°C and does not accumulate values above 30°C.
t.rast.accumulate input="temperature_mean_1990_2000_daily_celsius" \
    output="temperature_mean_1990_2000_daily_celsius_accumulated_10_30" \
    limits="10,30" start="1990-01-01" stop="2000-01-01" cycle="12 months" \
    basename="temp_acc_daily_10_30" method="bedd"

#############################################################################
#### ACCUMULATION PATTERN DETECTION #########################################
#############################################################################
# Now we detect the three grasshopper pest control cycles

# First cycle at 325°C - 427°C GDD
t.rast.accdetect input=temperature_mean_1990_2000_daily_celsius_accumulated_10_30@PERMANENT \
    occ=leafhopper_occurrence_c1_1990_2000 start="1990-01-01" stop="2000-01-01" \
    cycle="12 months" range=325,427 basename=lh_c1 indicator=leafhopper_indicator_c1_1990_2000

# Second cycle at 685°C - 813°C GDD
t.rast.accdetect input=temperature_mean_1990_2000_daily_celsius_accumulated_10_30@PERMANENT \
    occ=leafhopper_occurrence_c2_1990_2000 start="1990-01-01" stop="2000-01-01" \
    cycle="12 months" range=685,813 basename=lh_c2 indicator=leafhopper_indicator_c2_1990_2000

# Third cycle at 1047°C - 1179°C GDD
t.rast.accdetect input=temperature_mean_1990_2000_daily_celsius_accumulated_10_30@PERMANENT \
    occ=leafhopper_occurrence_c3_1990_2000 start="1990-01-01" stop="2000-01-01" \
    cycle="12 months" range=1047,1179 basename=lh_c3 indicator=leafhopper_indicator_c3_1990_2000


#############################################################################
#### YEARLY SPATIAL OCCURRENCE COMPUTATION OF ALL CYCLES ####################
#############################################################################

# Extract the areas that have full cycles
t.rast.aggregate input=leafhopper_indicator_c1_1990_2000 gran="1 year" \
    output=leafhopper_cycle_1_1990_2000_yearly method=maximum basename=li_c1

t.rast.mapcalc input=leafhopper_cycle_1_1990_2000_yearly basename=lh_clean_c1 \
    output=leafhopper_cycle_1_1990_2000_yearly_clean \
    expression="if(leafhopper_cycle_1_1990_2000_yearly == 3, 1, null())"

t.rast.aggregate input=leafhopper_indicator_c2_1990_2000 gran="1 year" \
    output=leafhopper_cycle_2_1990_2000_yearly method=maximum basename=li_c2

t.rast.mapcalc input=leafhopper_cycle_2_1990_2000_yearly basename=lh_clean_c2 \
    output=leafhopper_cycle_2_1990_2000_yearly_clean \
    expression="if(leafhopper_cycle_2_1990_2000_yearly == 3, 2, null())"

t.rast.aggregate input=leafhopper_indicator_c3_1990_2000 gran="1 year" \
    output=leafhopper_cycle_3_1990_2000_yearly method=maximum basename=li_c3

t.rast.mapcalc input=leafhopper_cycle_3_1990_2000_yearly basename=lh_clean_c3 \
    output=leafhopper_cycle_3_1990_2000_yearly_clean \
    expression="if(leafhopper_cycle_3_1990_2000_yearly == 3, 3, null())"


t.rast.mapcalc input=leafhopper_cycle_1_1990_2000_yearly_clean,leafhopper_cycle_2_1990_2000_yearly_clean,leafhopper_cycle_3_1990_2000_yearly_clean \
    basename=lh_cleann_all_cycles \
    output=leafhopper_all_cycles_1990_2000_yearly_clean \
    expression="if(isnull(leafhopper_cycle_3_1990_2000_yearly_clean), \
      if(isnull(leafhopper_cycle_2_1990_2000_yearly_clean), \
      if(isnull(leafhopper_cycle_1_1990_2000_yearly_clean), \
      null() ,1),2),3)"

cat > color.table << EOF
3 yellow
2 blue
1 red
EOF

t.rast.colors input=leafhopper_cycle_1_1990_2000_yearly_clean rules=color.table
t.rast.colors input=leafhopper_cycle_2_1990_2000_yearly_clean rules=color.table
t.rast.colors input=leafhopper_cycle_3_1990_2000_yearly_clean rules=color.table
t.rast.colors input=leafhopper_all_cycles_1990_2000_yearly_clean rules=color.table

#############################################################################
################ DURATION COMPUTATION #######################################
#############################################################################

# Extract the duration in days of the first cycle
t.rast.aggregate input=leafhopper_occurrence_c1_1990_2000 gran="1 year" \
    output=leafhopper_min_day_c1_1990_2000 method=minimum basename=occ_min_day_c1
t.rast.aggregate input=leafhopper_occurrence_c1_1990_2000 gran="1 year" \
    output=leafhopper_max_day_c1_1990_2000 method=maximum basename=occ_max_day_c1
t.rast.mapcalc input=leafhopper_min_day_c1_1990_2000,leafhopper_max_day_c1_1990_2000 \
    basename=occ_duration_c1 \
    output=leafhopper_duration_c1_1990_2000 \
    expression="leafhopper_max_day_c1_1990_2000 - leafhopper_min_day_c1_1990_2000"


# Extract the duration in days of the second cycle
t.rast.aggregate input=leafhopper_occurrence_c2_1990_2000 gran="1 year" \
    output=leafhopper_min_day_c2_1990_2000 method=minimum basename=occ_min_day_c2
t.rast.aggregate input=leafhopper_occurrence_c2_1990_2000 gran="1 year" \
    output=leafhopper_max_day_c2_1990_2000 method=maximum basename=occ_max_day_c2
t.rast.mapcalc input=leafhopper_min_day_c2_1990_2000,leafhopper_max_day_c2_1990_2000 \
    basename=occ_duration_c2 \
    output=leafhopper_duration_c2_1990_2000 \
    expression="leafhopper_max_day_c2_1990_2000 - leafhopper_min_day_c2_1990_2000"


# Extract the duration in days of the third cycle
t.rast.aggregate input=leafhopper_occurrence_c3_1990_2000 gran="1 year" \
    output=leafhopper_min_day_c3_1990_2000 method=minimum basename=occ_min_day_c3
t.rast.aggregate input=leafhopper_occurrence_c3_1990_2000 gran="1 year" \
    output=leafhopper_max_day_c3_1990_2000 method=maximum basename=occ_max_day_c3
t.rast.mapcalc input=leafhopper_min_day_c3_1990_2000,leafhopper_max_day_c3_1990_2000 \
    basename=occ_duration_c3 \
    output=leafhopper_duration_c3_1990_2000 \
    expression="leafhopper_max_day_c3_1990_2000 - leafhopper_min_day_c3_1990_2000"

t.rast.colors input=leafhopper_duration_c1_1990_2000 color=rainbow
t.rast.colors input=leafhopper_duration_c2_1990_2000 color=rainbow
t.rast.colors input=leafhopper_duration_c3_1990_2000 color=rainbow

#############################################################################
################ MONTHLY CYCLES OCCURRENCE ##################################
#############################################################################

# Extract the monthly indicator that shows the start and end of a cycle

# First cycle
t.rast.aggregate input=leafhopper_indicator_c1_1990_2000 gran="1 month" \
    output=leafhopper_indi_min_month_c1_1990_2000 method=minimum basename=occ_indi_min_month_c1
t.rast.aggregate input=leafhopper_indicator_c1_1990_2000 gran="1 month" \
    output=leafhopper_indi_max_month_c1_1990_2000 method=maximum basename=occ_indi_max_month_c1
t.rast.mapcalc input=leafhopper_indi_min_month_c1_1990_2000,leafhopper_indi_max_month_c1_1990_2000 \
    basename=indicator_monthly_c1 \
    output=leafhopper_monthly_indicator_c1_1990_2000 \
    expression="if(leafhopper_indi_min_month_c1_1990_2000 == 1, 1, if(leafhopper_indi_max_month_c1_1990_2000 == 3, 3, 2))"

# Second cycle
t.rast.aggregate input=leafhopper_indicator_c2_1990_2000 gran="1 month" \
    output=leafhopper_indi_min_month_c2_1990_2000 method=minimum basename=occ_indi_min_month_c2
t.rast.aggregate input=leafhopper_indicator_c2_1990_2000 gran="1 month" \
    output=leafhopper_indi_max_month_c2_1990_2000 method=maximum basename=occ_indi_max_month_c2
t.rast.mapcalc input=leafhopper_indi_min_month_c2_1990_2000,leafhopper_indi_max_month_c2_1990_2000 \
    basename=indicator_monthly_c2 \
    output=leafhopper_monthly_indicator_c2_1990_2000 \
    expression="if(leafhopper_indi_min_month_c2_1990_2000 == 1, 1, if(leafhopper_indi_max_month_c2_1990_2000 == 3, 3, 2))"

# Third cycle
t.rast.aggregate input=leafhopper_indicator_c3_1990_2000 gran="1 month" \
    output=leafhopper_indi_min_month_c3_1990_2000 method=minimum basename=occ_indi_min_month_c3
t.rast.aggregate input=leafhopper_indicator_c3_1990_2000 gran="1 month" \
    output=leafhopper_indi_max_month_c3_1990_2000 method=maximum basename=occ_indi_max_month_c3
t.rast.mapcalc input=leafhopper_indi_min_month_c3_1990_2000,leafhopper_indi_max_month_c3_1990_2000 \
    basename=indicator_monthly_c3 \
    output=leafhopper_monthly_indicator_c3_1990_2000 \
    expression="if(leafhopper_indi_min_month_c3_1990_2000 == 1, 1, if(leafhopper_indi_max_month_c3_1990_2000 == 3, 3, 2))"

cat > color.table << EOF
3 red
2 yellow
1 green
EOF

t.rast.colors input=leafhopper_monthly_indicator_c1_1990_2000 rules=color.table
t.rast.colors input=leafhopper_monthly_indicator_c2_1990_2000 rules=color.table
t.rast.colors input=leafhopper_monthly_indicator_c3_1990_2000 rules=color.table

#############################################################################
################ VISUALIZATION ##############################################
#############################################################################
# Now we use g.gui.animation to visualize the yearly occurrence, the duration and the monthly occurrence

# Yearly occurrence of all reproduction cycles
g.gui.animation strds=leafhopper_all_cycles_1990_2000_yearly_clean

# Yearly duration of reproduction cycle 1
g.gui.animation strds=leafhopper_duration_c1_1990_2000
# Yearly duration of reproduction cycle 2
g.gui.animation strds=leafhopper_duration_c2_1990_2000
# Yearly duration of reproduction cycle 3
g.gui.animation strds=leafhopper_duration_c3_1990_2000

# Monthly occurrence of reproduction cycle 1
g.gui.animation strds=leafhopper_monthly_indicator_c1_1990_2000
# Monthly occurrence of reproduction cycle 2
g.gui.animation strds=leafhopper_monthly_indicator_c2_1990_2000
# Monthly occurrence of reproduction cycle 3
g.gui.animation strds=leafhopper_monthly_indicator_c3_1990_2000
```

## REFERENCES

- Jones, G.V., Duff, A.A., Hall, A., Myers, J.W., 2010. Spatial Analysis
  of Climate in Winegrape Growing Regions in the Western United States.
  Am. J. Enol. Vitic. 61, 313-326.

## SEE ALSO

*[t.rast.accdetect](t.rast.accdetect.md),
[t.rast.aggregate](t.rast.aggregate.md),
[t.rast.mapcalc](t.rast.mapcalc.md), [t.info](t.info.md),
[g.region](g.region.md), [r.series.accumulate](r.series.accumulate.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture
