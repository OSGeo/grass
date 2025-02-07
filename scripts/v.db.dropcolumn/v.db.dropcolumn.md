## DESCRIPTION

*v.db.dropcolumn* drops a column from the attribute table connected to a
given vector map. It automatically checks the connection for the
specified layer. *v.db.dropcolumn* omits to delete the 'cat' column
which is relevant to keep the connection between vector map and table.

## NOTES

*v.db.dropcolumn* is a front-end to *db.execute* to allow easier usage.

The existing database connection(s) can be verified with *v.db.connect*.

## EXAMPLES

Dropping a column:  

```sh
g.copy vect=roadsmajor,myroads
v.info -c myroads
v.db.dropcolumn myroads column=SHAPE_LEN
v.info -c myroads
```

## SEE ALSO

*[db.connect](db.connect.md), [db.dropcolumn](db.dropcolumn.md),
[db.execute](db.execute.md), [v.db.addcolumn](v.db.addcolumn.md),
[v.db.connect](v.db.connect.md), [v.db.droptable](v.db.droptable.md),
[v.db.select](v.db.select.md), [v.db.update](v.db.update.md)*

*[GRASS SQL interface](sql.md)*

## AUTHOR

Markus Neteler
