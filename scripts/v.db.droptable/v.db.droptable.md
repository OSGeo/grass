## DESCRIPTION

*v.db.droptable* removes an existing attribute table from a given vector
map linked at given layer. If the **-f** force flag is not given then
nothing is removed.

## NOTES

*v.db.droptable* is a front-end to *db.execute* to allow easier usage.

The existing database connection(s) can be verified with *v.db.connect*.

## EXAMPLE

Removing attribute table connected to layer 1:\

```
g.copy vect=roadsmajor,myroads
v.db.droptable myroads
v.db.droptable myroads -f
v.db.connect -p myroads
```

## SEE ALSO

*[db.connect](db.connect.html), [db.execute](db.execute.html),
[v.db.addtable](v.db.addtable.html), [v.db.connect](v.db.connect.html),
[v.db.dropcolumn](v.db.dropcolumn.html),
[v.db.select](v.db.select.html), [v.db.update](v.db.update.html)*

*[GRASS SQL interface](sql.html)*

## AUTHOR

Markus Neteler
