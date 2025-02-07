## DESCRIPTION

*r.coin* tabulates the mutual occurrence of two raster map layers'
categories with respect to one another. This analysis program respects
the current geographic region and mask settings.

*r.coin* tabulates the coincidence of category values among the two map
layers and prepares the basic table from which the report is to be
created. This tabulation is followed by an indication of how long the
coincidence table will be. If the table is extremely long, the user may
decide that viewing it is not so important after all, and may cancel the
request at this point. Assuming the user continues, *r.coin* then allows
the user to choose one of eight units of measure in which the report
results can be given. These units are:

- c: cells
- p: percent cover of region
- x: percent of \<map name\> category (column)
- y: percent of \<map name\> category (row)
- a: acres
- h: hectares
- k: square kilometers
- m: square miles

Note that three of these options give results as percentage values: "p"
is based on the grand total number of cells; "x" is based on only column
totals; and "y" is based on only row totals. Only one unit of measure
can be selected per report output. Type in just one of the letters
designating a unit of measure followed by a \<RETURN\>. The report will
be printed to the screen for review. After reviewing the report on the
screen, the user is given several options. The report may be saved to a
file and/or sent to a printer. If printed, it may be printed with either
80 or 132 columns. Finally, the user is given the option to rerun the
coincidence tabulation using a different unit of measurement.

## NOTES

It is **not** a good idea to run *r.coin* on a map layer which has a
monstrous number of categories (e.g., unreclassed elevation). Because
*r.coin* reports information for each and every category, it is better
to reclassify those categories (using *r.reclass*) into a more
manageable number prior to running *r.coin* on the reclassed raster map
layer.

## EXAMPLE

Below is a sample of tabular output produced by *r.coin*. Here, map
output is stated in units of square miles. The report tabulates the
coincidence of the Spearfish sample database's *owner* and *road* raster
map layers' categories. The *owner* categories in this case refer to
whether the land is in private hands (category 1) or is owned by the
U.S. Forest Service (category 2). The *roads* map layer categories refer
to various types of roads (with the exception of category value "0",
which indicates "no data"; i.e., map locations at which no roads exist).
*r.coin* does not report category labels. The user should run
*[r.report](r.report.md)* or *[r.category](r.category.md)* to obtain
this information.

The body of the report is arranged in panels. The map layer with the
most categories is arranged along the vertical axis of the table; the
other, along the horizontal axis. Each panel has a maximum of 5
categories (9 if printed) across the top. In addition, the last two
columns reflect a cross total of each column for each row. All of the
categories of the map layer arranged along the vertical axis are
included in each panel. There is a total at the bottom of each column
representing the sum of all the rows in that column. A second total
represents the sum of all the non-zero category rows. A cross total
(Table Row Total) of all columns for each row appears in a separate
panel.

Note how the following information may be obtained from the sample
report.

In the Spearfish data base, in area not owned by the Forest Service,
there are 50.63 square miles of land not used for roads. Roads make up
9.27 square miles of land in this area.

Of the total 102.70 square miles in Spearfish, 42.80 square miles is
owned by the Forest Service.  
In total, there are 14.58 square miles of roads.

There are more category 2 roads outside Forest Service land (2.92 mi.
sq.) than there are inside Forest land boundaries (0.72 mi. sq.).

Following is a sample report.

```sh
+------------------------------------------------------------+
|                    COINCIDENCE TABULATION REPORT           |
|------------------------------------------------------------|
|Location: spearfish    Mapset: PERMANENT   Date: Wed Jun 1  |
|                                                            |
| Layer 1: owner          -- Ownership                       |
| Layer 2: roads          -- Roads                           |
| Mask:    none                                              |
|                                                            |
| Units:   square miles                                      |
|------------------------------------------------------------|
| Window:                North: 4928000.00                   |
|          West: 590000.00               East: 609000.00     |
|                        South: 4914000.00                   |
+------------------------------------------------------------+

Panel #1 of 1
+--------------------------------------------------------+
|        | owner                 |    Panel Row Total    |
|   cat# |         1 |         2 |   w cat 0 | w/o cat 0 |
|--------------------------------------------------------|
|r     0 |     50.63 |     37.49 |     88.12 |     88.12 |
|o     1 |      1.53 |      0.68 |      2.21 |      2.21 |
|a     2 |      2.92 |      0.72 |      3.64 |      3.64 |
|d     3 |      3.97 |      2.57 |      6.54 |      6.54 |
|s     4 |      0.65 |      1.36 |      2.00 |      2.00 |
|      5 |      0.19 |      0.00 |      0.19 |      0.19 |
|--------------------------------------------------------|
|Total   |           |           |           |           |
|with 0  |     59.90 |     42.80 |    102.70 |    102.70 |
|--------------------------------------------------------|
|w/o 0   |      9.27 |      5.32 |     14.58 |     14.58 |
+--------------------------------------------------------+


+--------------------------------+
|        |    Table Row Total    |
|   cat# |   w cat 0 | w/o cat 0 |
|--------------------------------|
|r     0 |     88.12 |     88.12 |
|o     1 |      2.21 |      2.21 |
|a     2 |      3.64 |      3.64 |
|d     3 |      6.54 |      6.54 |
|s     4 |      2.00 |      2.00 |
|      5 |      0.19 |      0.19 |
|--------------------------------|
|Total   |           |           |
|with 0  |    102.70 |    102.70 |
|--------------------------------|
|w/o 0   |     14.58 |     14.58 |
+--------------------------------+
```

*r.coin* calculates the coincidence of two raster map layers. Although
*r.coin* allows the user to rerun the report using different units, it
is not possible to simply rerun the report with different map layers. In
order to choose new map layers, it is necessary to rerun *r.coin.*

## SEE ALSO

*[g.region](g.region.md), [r.category](r.category.md),
[r.describe](r.describe.md), [r.reclass](r.reclass.md),
[r.report](r.report.md), [r.stats](r.stats.md)*

## AUTHORS

Michael O'Shea,  
Michael Shapiro,  
U.S. Army Construction Engineering Research Laboratory
