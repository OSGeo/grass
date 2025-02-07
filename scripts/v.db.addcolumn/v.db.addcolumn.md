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

Adding a single column:  

```sh
g.copy vect=roadsmajor,myroads
v.db.addcolumn myroads columns="slope double precision"
v.info -c myroads
```

Adding two columns:  

```sh
g.copy vect=roadsmajor,myroads
v.db.addcolumn myroads columns="slope double precision,myname varchar(15)"
v.info -c myroads
```

## SEE ALSO

*[db.connect](db.connect.md), [db.execute](db.execute.md),
[v.db.addtable](v.db.addtable.md), [v.db.connect](v.db.connect.md),
[v.db.dropcolumn](v.db.dropcolumn.md),
[v.db.droptable](v.db.droptable.md), [v.db.select](v.db.select.md),
[v.db.update](v.db.update.md)*

*[GRASS SQL interface](sql.md)*

## AUTHOR

Moritz Lennert (<mlennert@club.worldonline.be>)
