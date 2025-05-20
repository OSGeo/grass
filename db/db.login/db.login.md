## DESCRIPTION

*db.login* sets login parameters such an user name and optionally also a
password, a hostname or a port for the connection to the selected
**database** through the DB **driver**.

## NOTE

Options **host** and **port** are related to only SQL database backends
like [PostgreSQL](grass-pg.md), [MySQL](grass-mysql.md) or
[ODBC](grass-odbc.md).

Note that the passwords are stored in a hidden, *unencrypted* file in
the user account, specifically

- in the 'home' directory, i.e. `$HOME/.grass8/dblogin` (Unix-like
  systems)
- `%APPDATA%\Roaming\GRASS8\dblogin` (MS-Windows)

Only the file owner can access this file.

## EXAMPLES

Only username specified (assuming locally accessible PostgreSQL DB
without password):

```sh
db.login driver=pg database=mydb
```

Username, password and hostname specified (note that the command lines
history will store the password in this way):

```sh
db.login driver=pg database=mydb user=bacava password=secret host=db.example.com
```

Username and empty password specified:

```sh
db.login driver=pg database=mydb user=bacava password=""
```

## SEE ALSO

*[db.connect](db.connect.md), [db.test](db.test.md),
[db.tables](db.tables.md)*

[SQL support in GRASS GIS](sql.md)

## AUTHOR

Radim Blazek  
Support for hostname and port by Martin Landa, OSGeoREL, Czech Technical
University in Prague, Czech Republic (GRASS 7.1)
