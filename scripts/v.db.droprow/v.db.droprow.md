## DESCRIPTION

*v.db.droprow* removes vector object(s) (point, line, area, face etc.)
from a vector map through attribute selection in the table connected to
the given vector map. It automatically checks the connection for the
specified layer.

## NOTES

v.db.droprow is a front-end to *v.extract* (reverse selection) to allow
easier usage. The existing database connection(s) can be verified with
*v.db.connect*.

## EXAMPLES

Dropping all vector points without elevation attribute (North Carolina
data set):  

```sh
g.region raster=elevation -p
v.random output=rand5k_elev n=5000

v.db.addtable map=rand5k_elev column="elevation double precision"
v.what.rast vect=rand5k_elev rast=elevation column=elevation

# verify absence of some elevation attributes ("number of NULL attributes"):
v.univar rand5k_elev type=point column=elevation

# Remove all vector points lacking elevation attribute
v.db.droprow rand5k_elev output=rand5k_elev_filt where="elevation IS NULL"

# verify:
v.univar rand5k_elev_filt type=point column=elevation
```

## SEE ALSO

*[db.droptable](db.droptable.md), [db.execute](db.execute.md),
[v.db.addcolumn](v.db.addcolumn.md), [v.db.addtable](v.db.addtable.md),
[v.db.connect](v.db.connect.md), [v.db.dropcolumn](v.db.dropcolumn.md),
[v.db.droptable](v.db.droptable.md), [v.db.select](v.db.select.md),
[v.db.update](v.db.update.md)*

*[GRASS SQL interface](sql.md)*

## AUTHOR

Markus Neteler
