## DESCRIPTION

*v.db.join* joins the content of another table into the connected
attribute table of a vector map.

## NOTES

*v.db.join* is a front-end to *db.execute* to allow easier usage. The
vector attribute table must be stored in a SQL database (SQLite,
PostgreSQL, MySQL, ODBC, ...). The DBF backend is not supported. Tables
can be imported with *db.in.ogr*.

The vector map-database connection(s) can be verified with
*v.db.connect*.

## EXAMPLES

Exercise to join North Carolina geological classes from a CSV table to
the "geology" map of the North Carolina sample dataset (requires
download of legend CSV file
[nc_geology.csv](https://grassbook.org/wp-content/uploads/ncexternal/nc_geology.csv)
from [External data for NC sample
dataset](https://grassbook.org/wp-content/uploads/ncexternal/index.html)):

```sh
# check original map attributes
v.db.select geology column=GEO_NAME,SHAPE_area

# import of CSV table
db.in.ogr input=nc_geology.csv output=nc_geology

# work on copy of geology map in current mapset
g.copy vector=geology,mygeology

# check column names of vector map attributes
v.info -c mygeology

# check column names of legend table
db.describe -c nc_geology

# join table using key columns (map: "GEO_NAME"; table: "geol_id")
v.db.join map=mygeology column=GEO_NAME other_table=nc_geology other_column=geol_id

# verify result (here abbreviated)
v.db.select mygeology | head -3
cat|onemap_pro|PERIMETER|GEOL250_|GEOL250_ID|GEO_NAME|SHAPE_area|SHAPE_len|geol_id|longname|comment
1|963738.75|4083.97998|2|1|Zml|963738.608571|4083.979839|Zml|Metagraywacke|Interlayered with metaconglomerate, ...
2|22189124|26628.261719|3|2|Zmf|22189123.2296|26628.261112|Zmf|Metafelsite|Light-colored porphyritic extrusive rock
...
```

### Soil map table join

Joining the soil type explanations from table *soils_legend* into the
Spearfish soils map ([download
legend](https://grassbook.org/code-examples/code-examples-1st-edition/)):

```sh
g.copy vect=soils,mysoils

# import legend table
db.in.ogr soils_legend.csv out=soils_legend

# get join column names
v.info -c mysoils
db.describe -c soils_legend

# look at original table
v.db.select mysoils
cat|label
1|Aab
2|Ba
3|Bb
4|BcB
5|BcC
...

# look at legend
db.select table=soils_legend
db.select table=soils_legend | head -7
id|shortname|longname
0|no data|no data
0|AaB|Alice fine sandy loam, 0 to 6
0|Ba|Barnum silt loam
0|Bb|Barnum silt loam, channeled
0|BcB|Boneek silt loam, 2 to 6
0|BcC|Boneek silt loam, 6 to 9
...

# join soils_legend into mysoils attribute table
v.db.join mysoils col=label other_table=soils_legend ocol=shortname

# verification of join
v.db.select mysoils
cat|label|id|shortname|longname
1|Aab|||
2|Ba|2|Ba|Barnum silt loam
3|Bb|3|Bb|Barnum silt loam, channeled
4|BcB|4|BcB|Boneek silt loam, 2 to 6
5|BcC|5|BcC|Boneek silt loam, 6 to 9
...
```

## SEE ALSO

*[db.execute](db.execute.md), [db.in.ogr](db.in.ogr.md),
[db.select](db.select.md), [v.db.update](v.db.update.md)*

*[GRASS SQL interface](sql.md)*

## AUTHOR

Markus Neteler
