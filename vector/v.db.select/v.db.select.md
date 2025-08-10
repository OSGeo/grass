## DESCRIPTION

*v.db.select* prints attributes of a vector map from one or several user
selected attribute table columns.

### Output formats

Four different formats can be used depending on the circumstances using
the **format** option: plain text, CSV, JSON, and vertical plain text.

#### Plain text

The plain text is the default output which is most suitable for reading
by humans, e.g., when working in the command line or obtaining specific
values from the attribute table using the *v.db.select* GUI dialog.

The individual fields (attribute values) are separated by a pipe (`|`)
which can be customized using the **separator** option. The records
(rows) are separated by newlines.

Example with a pipe as a separator (the default):

```sh
cat|road_name|multilane|year|length
1|NC-50|no|2001|4825.369405
2|NC-50|no|2002|14392.589058
3|NC-98|no|2003|3212.981242
4|NC-50|no|2004|13391.907552
```

When escaping is enabled, the following characters in the fields are
escaped: backslash (`\\`), carriage return (`\r`), line feed (`\n`),
tabulator (`\t`), form feed (`\f`), and backslash (`\b`).

No quoting or escaping is performed by default, so if these characters
are in the output, they look just like the separators. This is usually
not a problem for humans looking at the output to get a general idea
about query result or attribute table content.

Consequently, this format is not recommended for computers, e.g., for
reading attribute data in Python scripts. It works for further parsing
in limited cases when the values don't contain separators or when the
separators are set to one of the escaped characters.

#### CSV

CSV (comma-separated values) has many variations. This module by default
produces CSV with comma (`,`) as the field separator (delimiter). All
text fields (based on the type) are quoted with double quotes. Double
quotes in fields are represented as two double quotes. Newline
characters in the fields are present as-is in the output. Header is
included by default containing column names.

All full CSV parsers such as the ones in LibreOffice or Python are able
to parse this format when configured to the above specification.

Example with default settings:

```csv
cat,road_name,multilane,year,length
1,"NC-50","no",2001,4825.369405
2,"NC-50","no",2002,14392.589058
3,"NC-98","no",2003,3212.981242
4,"NC-50","no",2004,13391.907552
```

If desired, the separator can be customized and escaping can be enabled
with the same characters being escaped as for the plain text. Notably,
newlines and tabs are escaped, double quotes are not, and the separator
is not escaped either (unless it is a tab). However, the format is
guaranteed only for the commonly used separators such as comma,
semicolon, pipe, and tab.

Note that using multi-character separator is allowed, but not
recommended as it is not generally supported by CSV readers.

CSV is the recommended format for further use in another analytical
applications, especially for use with spreadsheet applications. For
scripting, it is advantageous when tabular data is needed (rather than
key-value pairs).

#### JSON

JSON (JavaScript Object Notation) format is produced according to the
specification so it is readily readable by JSON parsers. The standard
JSON escapes are performed (backslash, carriage return, line feed,
tabulator, form feed, backslash, and double quote) for string values.
Numbers in the database such as integers and doubles are represented as
numbers, while texts (TEXT, VARCHAR, etc.) and dates in the database are
represented as strings in JSON. NULL values in database are represented
as JSON `null`. Indentation and newlines in the output are minimal and
not guaranteed.

Records which are the result of the query are stored under key `records`
as an array (list) of objects (collections of key-value pairs). The keys
for attributes are lowercase or uppercase depending on how the columns
were defined in the database.

The JSON also contains information about columns stored under key
`info`. Column names and types are under key `columns`. Each column has
SQL data type under `sql_type` in all caps. A boolean `is_number`
specifies whether the value is a number, i.e., integer or floating point
number. The `is_number` value is added for convenience and it is
recommended to rely on the types derived from the JSON representation or
the SQL types. The definition of `is_number` may change in the future.

Example with added indentation:

```json
{
  "info": {
    "columns": [
      {
        "name": "road_name",
        "sql_type": "CHARACTER",
        "is_number": false
      },
      {
        "name": "year",
        "sql_type": "INTEGER",
        "is_number": true
      },
      {
        "name": "length",
        "sql_type": "DOUBLE PRECISION",
        "is_number": true
      }
    ]
  },
  "records": [
    {
      "road_name": "NC-50",
      "year": 2001,
      "length": 4825.369405
    },
    {
      "road_name": "NC-50",
      "year": 2001,
      "length": 14392.589058
    }
  ]
}
```

JSON is the recommended format for reading the data in Python and for
any uses and environments where convenient access to individual values
is desired and JSON parser is available.

#### Vertical plain text

In the vertical plain text format, each value is on a single line and is
preceded by the name of the attribute (column) which is separated by
separator. The individual records can be separated by the vertical
separator (**vertical_separator** option).

Example with (horizontal) separator `=` and vertical separator
`newline`:

```sh
cat=1
road_name=NC-50
multilane=no
year=2001
length=4825.369405

cat=2
road_name=NC-50
multilane=no
year=2002
length=14392.589058
```

Newline is automatically added after a vertical separator unless it is a
newline which allows for separating the records, e.g., by multiple
dashes. The escaping (**-e**) need to should be enabled in case the
output is meant for reading by a computer rather than just as a data
overview for humans. Escaping will ensure that values with newlines will
be contained to a single line. This format is for special uses in
scripting, for example, in combination with **columns** option set to
one column only and escaping (**-e**) and no column names flags
(**-c**). It is also advantageous when you need implement the parsing
yourself.

## NOTES

- CSV and JSON were added in version 8.0 as new primary formats for
  further consumption by scripts and other applications.
- Escaping of plain and vertical formats was extended from just
  backslash and newlines to all escapes from JSON except for double
  quote character.

## EXAMPLES

All examples are based on the North Carolina sample dataset.

### Select and show entire table

```sh
v.db.select map=roadsmajor
cat|MAJORRDS_|ROAD_NAME|MULTILANE|PROPYEAR|OBJECTID|SHAPE_LEN
1|1|NC-50|no|0|1|4825.369405
2|2|NC-50|no|0|2|14392.589058
3|3|NC-98|no|0|3|3212.981242
4|4|NC-50|no|0|4|13391.907552
...
```

### Select and show single column from table

Note: multiple columns can be specified as comma separated list.

```sh
v.db.select map=roadsmajor column=ROAD_NAME
NC-50
NC-50
NC-98
NC-50
NC-98
...
```

### Print region extent of selected vector features

```sh
v.db.select -r map=roadsmajor where="ROAD_NAME = 'NC-98'"
n=248425.389891
s=245640.640081
w=635906.517653
e=661979.801880
```

### Select empty vector features (no data entries)

```sh
v.db.select geonames_wake where="ALTERNATEN IS NULL"
cat|GEONAMEID|NAME|ASCIINAME|ALTERNATEN|FEATURECLA|FEATURECOD|...
8|4498303|West Raleigh|West Raleigh||P|PPL|US||NC|338759|123|...
14|4459467|Cary|Cary||P|PPL|US||NC|103945|146|152|America/Iqaluit|...
31|4452808|Apex|Apex||P|PPL|US||NC|30873|167|134|America/Iqaluit|...
...
```

### Select not empty vector features (no data entries)

```sh
v.db.select geonames_wake where="ALTERNATEN IS NOT NULL"
cat|GEONAMEID|NAME|ASCIINAME|ALTERNATEN|FEATURECLA|FEATURECOD|...
9|4487042|Raleigh|Raleigh|Raleigh,...
31299|4487056|Raleigh-Durham Airport|Raleigh-Durham Airport|...
...
```

### Select features with distinct road names

```sh
v.db.select map=roadsmajor columns=ROAD_NAME group=ROAD_NAME
ROAD_NAME

I-40
I-440
I-540
NC-231
NC-39
NC-42
...
```

It is also possible to combine with *where* option

```sh
v.db.select map=roadsmajor columns=ROAD_NAME,MULTILANE group=ROAD_NAME where='ROAD_NAME is not null'
ROAD_NAME|MULTILANE
I-40|yes
I-440|yes
I-540|yes
NC-231|no
NC-39|no
NC-42|no
NC-50|no
NC-54|no
NC-55|no
NC-96|no
NC-97|no
NC-98|no
US-1|
US-401|no
US-64|yes
US-70|yes
```

It can also use more columns in *group* option

```sh
v.db.select map=roadsmajor columns=ROAD_NAME,MULTILANE group=ROAD_NAME,MULTILANE where='ROAD_NAME is not null'
ROAD_NAME|MULTILANE
I-40|yes
I-440|yes
I-540|yes
NC-231|no
NC-39|no
NC-42|no
NC-50|no
NC-54|no
NC-55|no
NC-96|no
NC-97|no
NC-98|no
US-1|
US-1|yes
US-401|no
US-401|yes
US-64|yes
US-70|yes
```

### Read results in Python

The *json* package in the standard Python library can load a JSON string
obtained as output from the *v.db.select* module through the
*read_command* function:

```python
import json
import grass.script as gs

text = gs.read_command("v.db.select", map="roadsmajor", format="json")
data = json.loads(text)
for row in data["records"]:
    print(row["ROAD_NAME"])
```

## SEE ALSO

*[db.select](db.select.md)*

*[GRASS SQL interface](sql.md)*

## AUTHORS

Radim Blazek, ITC-Irst, Trento, Italy  
Minimal region extent added by Martin Landa, FBK-irst (formerly
ITC-irst), Trento, Italy  
Group option added by Luca Delucchi, Fondazione Edmund Mach, Trento,
Italy  
Huidae Cho (JSON output, escaping and features-only flags)  
Vaclav Petras (true CSV output, format option and documentation)
