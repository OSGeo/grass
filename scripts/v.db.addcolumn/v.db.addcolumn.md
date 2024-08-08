## DESCRIPTION

*v.db.addcolumn* adds one or more column(s) to the attribute table
connected to a given vector map. It automatically checks the connection
for the specified layer.

## NOTES

*v.db.addcolumn* is a front-end to *db.execute* to allow easier usage.
The supported types of columns depend on the database backend. However,
all backends should support VARCHAR, INT, DOUBLE PRECISION and DATE.

The existing database connection(s) can be verified with *v.db.connect*.

## EXAMPLES

Adding a single column:\

```
g.copy vect=roadsmajor,myroads
v.db.addcolumn myroads columns="slope double precision"
v.info -c myroads
```

Adding two columns:\

```
g.copy vect=roadsmajor,myroads
v.db.addcolumn myroads columns="slope double precision,myname varchar(15)"
v.info -c myroads
```

## SEE ALSO

*[db.connect](db.connect.html), [db.execute](db.execute.html),
[v.db.addtable](v.db.addtable.html), [v.db.connect](v.db.connect.html),
[v.db.dropcolumn](v.db.dropcolumn.html),
[v.db.droptable](v.db.droptable.html), [v.db.select](v.db.select.html),
[v.db.update](v.db.update.html)*

*[GRASS SQL interface](sql.html)*

## AUTHOR

Moritz Lennert (mlennert@club.worldonline.be)
