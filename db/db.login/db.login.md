## DESCRIPTION

*db.login* sets login parameters such an user name and optionally also a
password, a hostname or a port for the connection to the selected
**database** through the DB **driver**.

## NOTE

Options **host** and **port** are related to only SQL database backends
like [PostgreSQL](grass-pg.html), [MySQL](grass-mysql.html) or
[ODBC](grass-odbc.html).

Note that the passwords are stored in a hidden, *unencrypted* file in
the user account, specifically

-   in the \'home\' directory, i.e. `$HOME/.grass8/dblogin` (Unix-like
    systems)
-   `%APPDATA%\Roaming\GRASS8\dblogin` (MS-Windows)

Only the file owner can access this file.

## EXAMPLES

Only username specified (assuming locally accessible PostgreSQL DB
without password):

::: code
    db.login driver=pg database=mydb
:::

Username, password and hostname specified (note that the command lines
history will store the password in this way):

::: code
    db.login driver=pg database=mydb user=bacava password=secret host=db.example.com
:::

Username and empty password specified:

::: code
    db.login driver=pg database=mydb user=bacava password=""
:::

## SEE ALSO

*[db.connect](db.connect.html), [db.test](db.test.html),
[db.tables](db.tables.html)*

[SQL support in GRASS GIS](sql.html)

## AUTHOR

Radim Blazek\
Support for hostname and port by Martin Landa, OSGeoREL, Czech Technical
University in Prague, Czech Republic (GRASS 7.1)
