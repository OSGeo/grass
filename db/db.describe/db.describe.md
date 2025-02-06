<h2>DESCRIPTION</h2>

<em>db.describe</em> displays table information. If parameter <b>-c</b>
is used only column names instead of full column descriptions is given.

<h2>NOTE</h2>

If parameters for database connection are already set with
<a href="db.connect.html">db.connect</a>, they are taken as default values and
do not need to be spcified each time.

<h2>EXAMPLES</h2>

<em>List column descriptions of table in SQLite database (note that this
is the default setting)</em><br>

<div class="code"><pre>
db.describe driver=sqlite table=hospitals \
   database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db'

# or simply
db.describe myarchsites
</pre></div>

<h3>DBF example</h3>
<div class="code"><pre>
db.describe -c table=hospitals database='$GISDBASE/$LOCATION_NAME/PERMANENT/dbf/' \
            driver=dbf
ncols: 16
nrows: 160
Column 1: cat:INTEGER:11
Column 2: OBJECTID:INTEGER:11
Column 3: AREA:DOUBLE PRECISION:20
[...]
</pre></div>

<div class="code"><pre>
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
</pre></div>

<h3>JSON Output</h3>
<div class="code"><pre>
db.describe table=hospitals format=json
</pre></div>

<div class="code"><pre>
{
    "table": "hospitals",
    "description": "",
    "insert": null,
    "delete": null,
    "ncols": 16,
    "nrows": 160,
    "columns": [
        {
            "position": 1,
            "column": "cat",
            "description": "",
            "type": "INTEGER",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 2,
            "column": "OBJECTID",
            "description": "",
            "type": "INTEGER",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 3,
            "column": "AREA",
            "description": "",
            "type": "DOUBLE PRECISION",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 4,
            "column": "PERIMETER",
            "description": "",
            "type": "DOUBLE PRECISION",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 5,
            "column": "HLS_",
            "description": "",
            "type": "INTEGER",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 6,
            "column": "HLS_ID",
            "description": "",
            "type": "INTEGER",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 7,
            "column": "NAME",
            "description": "",
            "type": "CHARACTER",
            "length": 45,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 8,
            "column": "ADDRESS",
            "description": "",
            "type": "CHARACTER",
            "length": 35,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 9,
            "column": "CITY",
            "description": "",
            "type": "CHARACTER",
            "length": 16,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 10,
            "column": "ZIP",
            "description": "",
            "type": "CHARACTER",
            "length": 5,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 11,
            "column": "COUNTY",
            "description": "",
            "type": "CHARACTER",
            "length": 12,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 12,
            "column": "PHONE",
            "description": "",
            "type": "CHARACTER",
            "length": 14,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 13,
            "column": "CANCER",
            "description": "",
            "type": "CHARACTER",
            "length": 4,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 14,
            "column": "POLYGONID",
            "description": "",
            "type": "INTEGER",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 15,
            "column": "SCALE",
            "description": "",
            "type": "DOUBLE PRECISION",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        },
        {
            "position": 16,
            "column": "ANGLE",
            "description": "",
            "type": "DOUBLE PRECISION",
            "length": 20,
            "scale": 0,
            "precision": 0,
            "default": null,
            "nullok": true,
            "select": null,
            "update": null
        }
    ]
}
</pre></div>

<h2>SEE ALSO</h2>

<em>
<a href="db.columns.html">db.columns</a>,
<a href="db.droptable.html">db.droptable</a>,
<a href="db.execute.html">db.execute</a>,
<a href="db.login.html">db.login</a>,
<a href="db.tables.html">db.tables</a>,
<a href="sql.html">GRASS SQL interface</a>
</em>

<h2>AUTHOR</h2>

Radim Blazek, ITC-Irst, Trento, Italy
