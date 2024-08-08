## DESCRIPTION

*db.dropcolumn* drops a column from an attribute table. If the **-f**
force flag is not given then nothing is removed, instead a preview of
the action to be taken is printed.

## NOTES

*db.dropcolumn* is a front-end to *db.execute* to allow easier usage
with a special workaround for the SQLite driver to support column drop
also for SQLite tables.

## EXAMPLE

Dropping a column (North Carolina sample dataset):

```
# work on own copy
g.copy vect=roadsmajor,myroads
db.describe -c myroads

# only shows what would happen:
db.dropcolumn myroads column=SHAPE_LEN

# actually drops the column
db.dropcolumn -f myroads column=SHAPE_LEN

db.describe -c myroads
```

## SEE ALSO

*[db.describe](db.describe.html), [db.droptable](db.droptable.html),
[db.execute](db.execute.html), [v.db.dropcolumn](v.db.dropcolumn.html),
[GRASS SQL interface](sql.html)*

## AUTHOR

Markus Neteler
