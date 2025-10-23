## DESCRIPTION

*t.rast.aggregate.ds* works like [t.rast.aggregate](t.rast.aggregate.md)
but instead of defining a fixed granularity for temporal aggregation the
time intervals of all maps registered in a second space time dataset
(can be STRDS, STR3DS or STVDS) are used to aggregate the maps of the
input space time raster dataset.

## NOTES

The sampling method must be specified from the sampler dataset point of
view. It defines the temporal relationships between intervals of the
sampling dataset and the input space time raster dataset.

## EXAMPLES

### Precipitation aggregation

In this example we create 7 raster maps that will be registered in a
single space time raster dataset named *precipitation_daily* using a
daily temporal granularity. The names of the raster maps are stored in a
text file that is used for raster map registration.

A space time vector dataset is created out of two vector maps with
different temporal resolution. The maps are created using v.random. The
first map has a granule of 3 days the second a granule of 4 days.

The space time raster dataset *precipitation_daily* with daily temporal
granularity will be aggregated using the space time vector dataset
resulting in the output space time raster dataset *precipitation_agg*.
The aggregation method is set to *sum* to accumulate the precipitation
values of all intervals in the space time vector dataset. The sampling
option assures that only raster maps that are temporally during the time
intervals of the space time vector dataset are considered for
computation. Hence the option is set to contains (time stamped vector
map layers temporally **contain** the raster map layers):

```sh
MAPS="map_1 map_2 map_3 map_4 map_5 map_6 map_7"

for map in ${MAPS} ; do
    r.mapcalc expression="${map} = 1"
    echo ${map} >> map_list.txt
done

t.create type=strds temporaltype=absolute \
         output=precipitation_daily \
         title="Daily precipitation" \
         description="Test dataset with daily precipitation"

t.register -i type=raster input=precipitation_daily \
           file=map_list.txt start="2012-08-20" increment="1 days"

t.info type=strds input=precipitation_daily

 +-------------------- Space Time Raster Dataset -----------------------------+
 |                                                                            |
 +-------------------- Basic information -------------------------------------+
 | Id: ........................ precipitation_daily@PERMANENT
 | Name: ...................... precipitation_daily
 | Mapset: .................... PERMANENT
 | Creator: ................... soeren
 | Temporal type: ............. absolute
 | Creation time: ............. 2014-11-23 16:48:17.686979
 | Modification time:.......... 2014-11-23 16:48:18.302978
 | Semantic type:.............. mean
 +-------------------- Absolute time -----------------------------------------+
 | Start time:................. 2012-09-10 00:00:00
 | End time:................... 2012-09-17 00:00:00
 | Granularity:................ 1 day
 | Temporal type of maps:...... interval
 +-------------------- Spatial extent ----------------------------------------+
 | North:...................... 80.0
 | South:...................... 0.0
 | East:.. .................... 120.0
 | West:....................... 0.0
 | Top:........................ 0.0
 | Bottom:..................... 0.0
 +-------------------- Metadata information ----------------------------------+
 | Raster register table:...... raster_map_register_3225725979b14b5db343a00835b882c7
 | North-South resolution min:. 10.0
 | North-South resolution max:. 10.0
 | East-west resolution min:... 10.0
 | East-west resolution max:... 10.0
 | Minimum value min:.......... 1.0
 | Minimum value max:.......... 1.0
 | Maximum value min:.......... 1.0
 | Maximum value max:.......... 1.0
 | Aggregation type:........... None
 | Number of registered maps:.. 7
 |
 | Title:
 | Daily precipitation
 | Description:
 | Test dataset with daily precipitation
 | Command history:
 | # 2014-11-23 16:48:17
 | t.create type="strds" temporaltype="absolute"
 |     output="precipitation_daily" title="Daily precipitation"
 |     description="Test dataset with daily precipitation"
 | # 2014-11-23 16:48:18
 | t.register -i type="rast" input="precipitation_daily"
 |     file="map_list.txt" start="2012-08-20" increment="1 days"
 |
 +----------------------------------------------------------------------------+


v.random output=points_1 n=20
v.random output=points_2 n=20

t.create type=stvds temporaltype=absolute \
         output=points \
         title="Points" \
         description="Points for aggregation"

t.register -i type=vector input=points \
           map=points_1 start="2012-08-20" increment="3 days"
t.register -i type=vector input=points \
           map=points_2 start="2012-08-23" increment="4 days"

t.info type=stvds input=points

 +-------------------- Space Time Vector Dataset -----------------------------+
 |                                                                            |
 +-------------------- Basic information -------------------------------------+
 | Id: ........................ points@PERMANENT
 | Name: ...................... points
 | Mapset: .................... PERMANENT
 | Creator: ................... soeren
 | Temporal type: ............. absolute
 | Creation time: ............. 2014-11-23 16:48:49.193903
 | Modification time:.......... 2014-11-23 16:48:50.185671
 | Semantic type:.............. mean
 +-------------------- Absolute time -----------------------------------------+
 | Start time:................. 2012-08-20 00:00:00
 | End time:................... 2012-08-27 00:00:00
 | Granularity:................ 1 day
 | Temporal type of maps:...... interval
 +-------------------- Spatial extent ----------------------------------------+
 | North:...................... 79.283411
 | South:...................... 5.724954
 | East:.. .................... 118.881168
 | West:....................... 0.016755
 | Top:........................ 0.0
 | Bottom:..................... 0.0
 +-------------------- Metadata information ----------------------------------+
 | Vector register table:...... vector_map_register_6f02d33e0ee243d1a521aaaca39ecb31
 | Number of points ........... 40
 | Number of lines ............ 0
 | Number of boundaries ....... 0
 | Number of centroids ........ 0
 | Number of faces ............ 0
 | Number of kernels .......... 0
 | Number of primitives ....... 40
 | Number of nodes ............ 0
 | Number of areas ............ 0
 | Number of islands .......... 0
 | Number of holes ............ 0
 | Number of volumes .......... 0
 | Number of registered maps:.. 2
 |
 | Title:
 | Points
 | Description:
 | Points for aggregation
 | Command history:
 | # 2014-11-23 16:48:49
 | t.create type="stvds" temporaltype="absolute"
 |     output="points" title="Points" description="Points for aggregation"
 | # 2014-11-23 16:48:49
 | t.register -i type="vect" input="points"
 |     map="points_1" start="2012-08-20" increment="3 days"
 | # 2014-11-23 16:48:50
 | t.register -i type="vect" input="points"
 |     map="points_2" start="2012-08-23" increment="4 days"
 |
 +----------------------------------------------------------------------------+

t.rast.aggregate.ds input=precipitation_daily \
                    output=precipitation_agg \
                    sample=points type=stvds \
                    basename=prec_agg \
                    method=sum sampling=contains

t.support input=precipitation_agg \
          title="Aggregated precipitation" \
          description="Aggregated precipitation dataset"

t.info type=strds input=precipitation_agg

 +-------------------- Space Time Raster Dataset -----------------------------+
 |                                                                            |
 +-------------------- Basic information -------------------------------------+
 | Id: ........................ precipitation_agg@PERMANENT
 | Name: ...................... precipitation_agg
 | Mapset: .................... PERMANENT
 | Creator: ................... soeren
 | Temporal type: ............. absolute
 | Creation time: ............. 2014-11-23 16:53:23.488799
 | Modification time:.......... 2014-11-23 16:53:28.714886
 | Semantic type:.............. mean
 +-------------------- Absolute time -----------------------------------------+
 | Start time:................. 2012-08-20 00:00:00
 | End time:................... 2012-08-27 00:00:00
 | Granularity:................ 1 day
 | Temporal type of maps:...... interval
 +-------------------- Spatial extent ----------------------------------------+
 | North:...................... 80.0
 | South:...................... 0.0
 | East:.. .................... 120.0
 | West:....................... 0.0
 | Top:........................ 0.0
 | Bottom:..................... 0.0
 +-------------------- Metadata information ----------------------------------+
 | Raster register table:...... raster_map_register_7b025eb7431747c98c5c1ad971e8c282
 | North-South resolution min:. 10.0
 | North-South resolution max:. 10.0
 | East-west resolution min:... 10.0
 | East-west resolution max:... 10.0
 | Minimum value min:.......... 3.0
 | Minimum value max:.......... 4.0
 | Maximum value min:.......... 3.0
 | Maximum value max:.......... 4.0
 | Aggregation type:........... sum
 | Number of registered maps:.. 2
 |
 | Title:
 | Aggregated precipitation
 | Description:
 | Aggregated precipitation dataset
 | Command history:
 | # 2014-11-23 16:53:23
 | t.rast.aggregate.ds input="precipitation_daily"
 |     output="precipitation_agg" sample="points" type="stvds" basename="prec_agg"
 |     method="sum" sampling="contains"
 | # 2014-11-23 16:53:28
 | t.support input="precipitation_agg"
 |     title="Aggregated precipitation"
 |     description="Aggregated precipitation dataset"
 |
 +----------------------------------------------------------------------------+
```

### MODIS satellite sensor daily data aggregation to 8 days

In this example the aggregation from daily data to eight days is shown.
This "eight-day week" is used in some MODIS satellite sensor products.

```sh
# NOTE: the example is written in shell language

# create maps every 8 days as seed maps
for year in `seq 2000 2001` ; do
   for doy in `seq -w 1 8 365` ; do
      r.mapcalc -s expression="8day_${year}_${doy} = rand(0.0,40.0)"
   done
done

# From de name of each map, we take year and doy, and convert it
# to a YYYY-MM-DD date for start and end, and create a file with
# mapnames, start date and end date

g.list type=raster pattern=8day_20??_* > names_list

for NAME in `cat names_list` ; do

   # Parse
   YEAR=`echo $NAME | cut -d'_' -f2`
   DOY=`echo $NAME | cut -d'_' -f3`

   # convert YYYY_DOY to YYYY-MM-DD
   DOY=`echo "$DOY" | sed 's/^0*//'`
   doy_end=0

   if [ $DOY -le "353" ] ; then
      doy_end=$(( $DOY + 8 ))
   elif [ $DOY -eq "361" ] ; then
      if [ $[$YEAR % 4] -eq 0 ] && [ $[$YEAR % 100] -ne 0 ] || [ $[$YEAR % 400] -eq 0 ] ; then
         doy_end=$(( $DOY + 6 ))
      else
            doy_end=$(( $DOY + 5 ))
      fi
   fi

   DATE_START=`date -d "${YEAR}-01-01 +$(( ${DOY} - 1 ))days" +%Y-%m-%d`
   DATE_END=`date -d "${YEAR}-01-01 +$(( ${doy_end} -1 ))days" +%Y-%m-%d`

   # text file with mapnames, start date and end date
   echo "$NAME|$DATE_START|$DATE_END" >> list_map_start_end_time.txt

done

# check the list created.
cat list_map_start_end_time.txt
8day_2000_001|2000-01-01|2000-01-09
8day_2000_009|2000-01-09|2000-01-17
...
8day_2000_353|2000-12-18|2000-12-26
8day_2000_361|2000-12-26|2001-01-01
8day_2001_001|2001-01-01|2001-01-09
8day_2001_009|2001-01-09|2001-01-17
...
8day_2001_345|2001-12-11|2001-12-19
8day_2001_353|2001-12-19|2001-12-27
8day_2001_361|2001-12-27|2002-01-01

# all maps except for the last map in each year represent 8-days
# intervals. But the aggregation starts all over again every
# January 1st.

# create 8-day MODIS-like strds
t.create type=strds temporaltype=absolute \
   output=8day_ts title="8 day time series" \
   description="STRDS with MODIS like 8 day aggregation"

# register maps
t.register type=raster input=8day_ts \
   file=list_map_start_end_time.txt

# check
t.info input=8day_ts
t.rast.list input=8day_ts

# finally, copy the aggregation to a daily time series
t.rast.aggregate.ds input=daily_ts sample=8day_ts \
   output=8day_agg basename=8day_agg method=average \
   sampling=contains suffix=gran

# add metadata
t.support input=8day_agg \
   title="8 day aggregated ts" \
   description="8 day MODIS-like aggregated dataset"

# check map list in newly created aggregated strds
t.rast.list input=8day_agg
name|mapset|start_time|end_time
8day_agg_2000_01_01|modis|2000-01-01 00:00:00|2000-01-09 00:00:00
8day_agg_2000_01_09|modis|2000-01-09 00:00:00|2000-01-17 00:00:00
8day_agg_2000_01_17|modis|2000-01-17 00:00:00|2000-01-25 00:00:00
...
8day_agg_2000_12_18|modis|2000-12-18 00:00:00|2000-12-26 00:00:00
8day_agg_2000_12_26|modis|2000-12-26 00:00:00|2001-01-01 00:00:00
8day_agg_2001_01_01|modis|2001-01-01 00:00:00|2001-01-09 00:00:00
...
8day_agg_2001_12_11|modis|2001-12-11 00:00:00|2001-12-19 00:00:00
8day_agg_2001_12_19|modis|2001-12-19 00:00:00|2001-12-27 00:00:00
8day_agg_2001_12_27|modis|2001-12-27 00:00:00|2002-01-01 00:00:00
```

## SEE ALSO

*[t.rast.aggregate](t.rast.aggregate.md), [t.create](t.create.md),
[t.info](t.info.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture
