## SQL parser library

sqlp is the SQL parser library.

sqp is intended as library for simple dbmi drivers (like dbf, txt).
`yac.y` and `lex.l` was originally stolen from unixODBC 3/2001 and modified.

An input may be subset of SQL statements. Currently supported:

```sql
SELECT FROM WHERE
INSERT INTO
UPDATE WHERE
DELETE FROM WHERE
CREATE TABLE
DROP TABLE
[...]
```

New types have to be added in `yac.y`, `lex.l`, `print.c` and
`../../../include/sqlp.h`.

In `./test/` is a test program to the the SQL parser (see
README.md there).
