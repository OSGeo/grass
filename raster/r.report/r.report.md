## DESCRIPTION

*r.report* allows the user to set up a series of report parameters to be
applied to a raster map, and creates a report. The report will print out
to the standard output if **output** parameter is not given.

The report itself consists of two parts, a header section and the main
body of the report.

The header section of the report identifies the raster map(s) (by map
name and title), project, mapset, report date, and the region of
interest. The area of interest is described in two parts: the user\'s
current geographic region is presented, and the mask is presented (if
any is used).

The main body of the report consists of from one to three tables which
present the statistics for each category and the totals for each unit
column. Note that the statistics is always computed in the current
geographical region.

When multiple (typically two) raster maps are specified,
cross-tabulation table for each combination of categories in the raster
maps will be computed and formatted in a human-readable way (see
example).

## NOTES

Note that, unlike *[r.stats](r.stats.html)*, *r.report* allows the user
to select the specific units of measure in which statistics will be
reported. To output computer-friendly data suitable for importing into a
spreadsheet use the *[r.stats](r.stats.html)* module. In fact *r.report*
is running *[r.stats](r.stats.html)* in the background and reformatting
the results to be more human-friendly.

## EXAMPLE

Report sorted areas in square miles and acres for each category. No-data
are not reported (see **-n** flag).

::: code
    r.report -n map=geology_30m units=mi,a sort=desc
:::

::: code
    +-----------------------------------------------------------------------------+
    |                         RASTER MAP CATEGORY REPORT                          |
    |PROJECT: nc_spm_08_grass7                            Fri Dec  6 17:00:21 2013|
    |-----------------------------------------------------------------------------|
    |          north: 279073.97546639    east: 798143.31179672                    |
    |REGION    south: 113673.97546639    west: 595143.31179672                    |
    |          res:               200    res:              200                    |
    |-----------------------------------------------------------------------------|
    |MASK: none                                                                   |
    |-----------------------------------------------------------------------------|
    |MAP: South-West Wake county: geology derived from vector map (geology_30m in |
    |-----------------------------------------------------------------------------|
    |                 Category Information                  |    square|          |
    |  #|description                                        |     miles|     acres|
    |-----------------------------------------------------------------------------|
    |217|CZfg . . . . . . . . . . . . . . . . . . . . . . . | 27.783911|17,781.703|
    |270|CZig . . . . . . . . . . . . . . . . . . . . . . . | 26.162282|16,743.861|
    |405|CZbg . . . . . . . . . . . . . . . . . . . . . . . |  9.698886|  6207.287|
    |262|CZlg . . . . . . . . . . . . . . . . . . . . . . . |  7.629379|  4882.802|
    |862|CZam . . . . . . . . . . . . . . . . . . . . . . . |  2.532830|  1621.011|
    |910|CZbg . . . . . . . . . . . . . . . . . . . . . . . |  1.683405|  1077.379|
    |583|CZve . . . . . . . . . . . . . . . . . . . . . . . |  0.972977|   622.706|
    |921|Km . . . . . . . . . . . . . . . . . . . . . . . . |  0.463323|   296.526|
    |766|CZg. . . . . . . . . . . . . . . . . . . . . . . . |  0.324326|   207.569|
    |720|CZam . . . . . . . . . . . . . . . . . . . . . . . |  0.185329|   118.611|
    |946|CZam . . . . . . . . . . . . . . . . . . . . . . . |  0.138997|    88.958|
    |948|CZam . . . . . . . . . . . . . . . . . . . . . . . |  0.030888|    19.768|
    |-----------------------------------------------------------------------------|
    |TOTAL                                                  | 77.606534|49,668.182|
    +-----------------------------------------------------------------------------+
:::

Report areas for each category of land use for each zipcode (included
only part of the table):

::: code
    r.report map=zipcodes@PERMANENT,landclass96@PERMANENT units=h,p
:::

::: code
    +-----------------------------------------------------------------------------+
    |                         RASTER MAP CATEGORY REPORT                          |
    |PROJECT: nc_spm_08_latest                            Tue Feb 11 10:10:46 2014|
    |-----------------------------------------------------------------------------|
    |          north: 228527.25    east: 644971                                   |
    |REGION    south: 215018.25    west: 629980                                   |
    |          res:        28.5    res:    28.5                                   |
    |-----------------------------------------------------------------------------|
    |MASK: none                                                                   |
    |-----------------------------------------------------------------------------|
    |MAPS: South West Wake: Zipcode areas derived from vector map (zipcodes@PERMAN|
    |        South-West Wake county: Simplified landuse classes (landclass96@PERMA|
    |-----------------------------------------------------------------------------|
    |                   Category Information                    |          |   %  |
    |    #|description                                          |  hectares| cover|
    |-----------------------------------------------------------------------------|
    |27511|CARY                                                 |  1053.813|  5.20|
    |     |-----------------------------------------------------|----------|------|
    |     |1|developed. . . . . . . . . . . . . . . . . . . . . |   197.214| 18.71|
    |     |3|herbaceous . . . . . . . . . . . . . . . . . . . . |    25.017|  2.37|
    |     |4|shrubland. . . . . . . . . . . . . . . . . . . . . |    58.563|  5.56|
    |     |5|forest . . . . . . . . . . . . . . . . . . . . . . |   771.313| 73.19|
    |     |6|water. . . . . . . . . . . . . . . . . . . . . . . |     1.625|  0.15|
    |     |*|no data. . . . . . . . . . . . . . . . . . . . . . |     0.081|  0.01|
    |-----------------------------------------------------------|----------|------|
    |27513|CARY                                                 |   204.525|  1.01|
    |     |-----------------------------------------------------|----------|------|
    |     |1|developed. . . . . . . . . . . . . . . . . . . . . |    32.571| 15.93|
    |     |3|herbaceous . . . . . . . . . . . . . . . . . . . . |     6.011|  2.94|
    |     |4|shrubland. . . . . . . . . . . . . . . . . . . . . |    51.659| 25.26|
    |     |5|forest . . . . . . . . . . . . . . . . . . . . . . |   114.284| 55.88|
    |-----------------------------------------------------------|----------|------|
    ...
    |-----------------------------------------------------------------------------|
    |TOTAL                                                      |22,968.900|100.00|
    +-----------------------------------------------------------------------------+
:::

## SEE ALSO

*[r.stats](r.stats.html), [g.region](g.region.html),
[r.coin](r.coin.html), [r.describe](r.describe.html),
[r.info](r.info.html), [r.univar](r.univar.html)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory\
Sort option by Martin Landa, Czech Technical University in Prague, 2013
