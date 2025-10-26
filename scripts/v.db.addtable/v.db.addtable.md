## DESCRIPTION

*v.db.addtable* creates and adds a new attribute table to a given vector
map. It links the table to the specified layer of the vector map. If the
vector map is not yet linked to any table, new a database link is
established based on the MAPSET database settings (see *db.connect*).

## NOTES

*v.db.addtable* is a front-end to *db.execute* to allow easier usage.

*v.db.addtable* will only insert category values into the table for
those features which actually have a category value in the relevant
layer. The user can add category values automatically by using
*v.category* or manually with *[wxGUI vector
digitizer](wxGUI.vdigit.md)* before running v.db.addtable. Or one can
run v.db.addtable first and then use either a combinatino of
*v.category* + *v.to.db* or *[wxGUI vector digitizer](wxGUI.vdigit.md)*
to add the relevant lines to the table.

The supported types of columns depend on the database backend. However,
all backends should support VARCHAR, INT, DOUBLE PRECISION and DATE.

The existing database connection(s) can be verified with *v.db.connect*.

## EXAMPLE

Adding a new attribute table with a single column to default layer 1:  

```sh
g.copy vect=roadsmajor,myroads
v.db.addtable myroads columns="slope double precision"
v.db.connect -p myroads
v.info -c myroads
```

Adding a new attribute table with two columns to layer 2:  

```sh
g.copy vect=roadsmajor,myroads
v.db.addtable myroads columns="slope double precision, roadname varchar(15)" layer=2
v.db.connect -p myroads
v.info -c myroads
v.info -c myroads layer=2
```

## SEE ALSO

*[db.connect](db.connect.md), [db.droptable](db.droptable.md),
[db.execute](db.execute.md), [v.db.connect](v.db.connect.md),
[v.db.dropcolumn](v.db.dropcolumn.md),
[v.db.droptable](v.db.droptable.md), [v.db.select](v.db.select.md),
[v.db.update](v.db.update.md)*

*[GRASS SQL interface](sql.md)*

## AUTHOR

Markus Neteler
