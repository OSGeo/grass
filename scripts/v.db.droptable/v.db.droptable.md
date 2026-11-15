## DESCRIPTION

*v.db.droptable* removes an existing attribute table from a given vector
map linked at given layer. If the **-f** force flag is not given then
nothing is removed.

## NOTES

*v.db.droptable* is a front-end to *db.execute* to allow easier usage.

The existing database connection(s) can be verified with *v.db.connect*.

## EXAMPLE

Removing attribute table connected to layer 1:  

```sh
g.copy vect=roadsmajor,myroads
v.db.droptable myroads
v.db.droptable myroads -f
v.db.connect -p myroads
```

## SEE ALSO

*[db.connect](db.connect.md), [db.execute](db.execute.md),
[v.db.addtable](v.db.addtable.md), [v.db.connect](v.db.connect.md),
[v.db.dropcolumn](v.db.dropcolumn.md), [v.db.select](v.db.select.md),
[v.db.update](v.db.update.md)*

*[GRASS SQL interface](sql.md)*

## AUTHOR

Markus Neteler
