## DESCRIPTION

*v.db.update* assigns a new value to a column in the attribute table
connected to a given map. The *value* parameter allows updating with a
literal value. Alternatively, with the *qcol* parameter values can be
copied from another column in the table or be the result of a
combination or transformation of other columns.

## NOTES

*v.db.update* is just a front-end to *db.execute* to allow easier usage.

For complex SQL UPDATE statements, *db.execute* should be used.

## EXAMPLES

### Replacing of NULL values

In this example, selectively display lakes without (blue) and with NULL
(red) are shown to find out which type is undefined. In the original map
there are lakes missing FTYPE attribute which are wetlands along
streams. These NULL attributes are replaced with the landuse type
WETLAND:

```sh
g.copy vect=lakes,mylakes
v.db.select mylakes
v.db.select mylakes where="FTYPE IS NULL"

# display the lakes, show undefined FTYPE lakes in red
g.region vector=mylakes
d.mon wx0
d.vect mylakes where="FTYPE NOT NULL" type=area col=blue
d.vect mylakes where="FTYPE IS NULL" type=area col=red

# replace NULL with FTYPE WETLAND
v.db.update mylakes col=FTYPE value=WETLAND \
            where="FTYPE IS NULL"
v.db.select mylakes
```

### Updating of columns with on the fly calculation

Spearfish example: adding new column, copying values from another table
column with on the fly calculation:

```sh
g.copy vect=fields,myfields
v.db.addcolumn myfields col="polynum integer"
v.db.update myfields col=polynum qcol="cat*2"
v.db.select myfields
```

### Type casting

Type cast (type conversion) of strings to double precision (unsupported
by DBF driver):

```sh
g.copy vect=geodetic_pts,mygeodetic_pts
v.db.update mygeodetic_pts col=zval qcol="CAST(z_value AS double precision)" \
            where="z_value <> 'N/A'"
```

### Updating of columns with on the fly calculation (SQLite extended functions)

Note: this requires SQLite extended functions. For details see the GRASS
GIS Wiki (compilation of
[libsqlitefunctions.so](https://grasswiki.osgeo.org/wiki/Build_SQLite_extension_on_Linux)
and
[libsqlitefunctions.dll](https://grasswiki.osgeo.org/wiki/Build_SQLite_extension_on_windows)).

North Carolina data set example: adding new column, copying values from
another table column with on the fly calculation:

```sh
g.copy vect=precip_30ynormals,myprecip_30ynormals
v.db.addcolumn myprecip_30ynormals column="logjuly double precision"
v.db.update myprecip_30ynormals column="logjuly" query_column="log(jul)" \
  sqliteextra=$HOME/sqlite_extensions/libsqlitefunctions.so

v.db.select myprecip_30ynormals columns=jul,logjuly
jul|logjuly
132.842|4.88916045210132
127|4.84418708645859
124.206|4.82194147751127
104.648|4.65060233738593
98.298|4.58800368106618
...
```

## SEE ALSO

*[db.execute](db.execute.md), [v.db.addcolumn](v.db.addcolumn.md),
[v.db.addtable](v.db.addtable.md), [v.db.connect](v.db.connect.md),
[v.db.droptable](v.db.droptable.md), [v.db.join](v.db.join.md),
[v.db.select](v.db.select.md)*

*[GRASS SQL interface](sql.md)*

## AUTHOR

Moritz Lennert (<mlennert@club.worldonline.be>)
