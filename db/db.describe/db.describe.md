## DESCRIPTION

*db.describe* displays table information. If parameter **-c** is used
only column names instead of full column descriptions is given.

## NOTE

If parameters for database connection are already set with
[db.connect](db.connect.html), they are taken as default values and do
not need to be spcified each time.

## EXAMPLES

*List column descriptions of table in SQLite database (note that this is
the default setting)*\

::: code
    db.describe driver=sqlite table=hospitals \
       database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'

    # or simply
    db.describe myarchsites
:::

### DBF example

::: code
    db.describe -c table=hospitals database='$GISDBASE/$LOCATION_NAME/PERMANENT/dbf/' \
                driver=dbf
    ncols: 16
    nrows: 160
    Column 1: cat:INTEGER:11
    Column 2: OBJECTID:INTEGER:11
    Column 3: AREA:DOUBLE PRECISION:20
    [...]
:::

::: code
    db.describe table=hospitals database='$GISDBASE/$LOCATION_NAME/PERMANENT/dbf/' \
                driver=dbf
    table:hospitals
    description:
    insert:yes
    delete:yes
    ncols:16
    nrows:160

    column:cat
    description:
    type:INTEGER
    len:11
    scale:0
    precision:10
    default:
    nullok:yes
    select:yes
    update:yes

    column:OBJECTID
    description:
    type:INTEGER
    [...]
:::

## SEE ALSO

*[db.columns](db.columns.html), [db.droptable](db.droptable.html),
[db.execute](db.execute.html), [db.login](db.login.html),
[db.tables](db.tables.html), [GRASS SQL interface](sql.html)*

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy
