## DESCRIPTION

*v.db.renamecolumn* renames a column in the attribute table connected to
a given vector map. It automatically checks the connection for the
specified layer.

## NOTES

If the map table is connected through the DBF or SQLite drivers, the
renaming is internally done by adding a new column with new name,
transferring the contents of the old column to the new column and
dropping the old column. This is needed as DBF or SQLite do not support
"ALTER TABLE" command to rename columns. Due to this the renamed column
is found as last column of the table, it's original position cannot be
maintained.

The SQLite driver will exit with an error if the column rename involves
only a change of case, i.e., upper-to-lowercase, or lower-to-uppercase.
The SQLite protocol considers "NAME" and "name" to be identical column
names. In cases like these, the user should rename the original column
to an intermediary name, then rename the intermediary to the final name.

## EXAMPLES

Renaming a column:  

```sh
g.copy vect=roadsmajor,myroads
v.info -c myroads
v.db.renamecolumn myroads column=ROAD_NAME,roadname
v.info -c myroads
```

## SEE ALSO

*[db.execute](db.execute.md), [v.db.addcolumn](v.db.addcolumn.md),
[v.db.addtable](v.db.addtable.md), [v.db.connect](v.db.connect.md),
[v.db.dropcolumn](v.db.dropcolumn.md),
[v.db.droptable](v.db.droptable.md), [v.db.select](v.db.select.md),
[v.db.update](v.db.update.md)*

*[GRASS SQL interface](sql.md)*

## AUTHOR

Markus Neteler
