## DESCRIPTION

*v.what* outputs the category number value(s) associated with
user-specified location(s) in user-specified vector map layer(s). This
module was derived from the *d.what.vect* module by removing all
interactive code and modification of the output for easy parsing. Using
the *-g* flag generates script-style output which is easily parsable.

## EXAMPLE

North Carolina sample dataset example:

Query polygon at given position:

```sh
v.what zipcodes_wake coordinates=637502.25,221744.25
```

Find closest hospital to given position within given distance (search
radius):

```sh
v.what hospitals coordinates=542690.4,204802.7 distance=2000000
```

Extracting categories from JSON output using Python:

```python
import json
import grass.script as gs

result = gs.read_command(
    "v.what",
    map="hospitals",
    coordinates=[542690.4, 204802.7],
    distance=2000000,
    flags="ja",
)

data = json.loads(result)
print(data[0]["categories"])
```

Possible output:

```text
[{'layer': 1, 'category': 22, 'driver': 'sqlite', 'database': '/grassdata/nc_spm_08_grass7/PERMANENT/sqlite/sqlite.db', 'table': 'hospitals', 'key_column': 'cat', 'attributes': {'cat': '22', 'OBJECTID': '22', 'AREA': '0', 'PERIMETER': '0', 'HLS_': '22', 'HLS_ID': '22', 'NAME': 'Randolph Hospital', 'ADDRESS': '364 White Oak St', 'CITY': 'Asheboro', 'ZIP': '27203', 'COUNTY': 'Randolph', 'PHONE': '(336) 625-5151', 'CANCER': 'yes', 'POLYGONID': '0', 'SCALE': '1', 'ANGLE': '1'}}]
```

The whole JSON may look like this:

```json
[
    {
        "coordinate": {
            "easting": 542690.40000000002,
            "northing": 204802.70000000001
        },
        "map": "hospitals",
        "mapset": "PERMANENT",
        "type": "Point",
        "id": 22,
        "categories": [
            {
                "layer": 1,
                "category": 22,
                "driver": "sqlite",
                "database": "/grassdata/nc_spm_08_grass7/PERMANENT/sqlite/sqlite.db",
                "table": "hospitals",
                "key_column": "cat",
                "attributes": {
                    "cat": "22",
                    "OBJECTID": "22",
                    "AREA": "0",
                    "PERIMETER": "0",
                    "HLS_": "22",
                    "HLS_ID": "22",
                    "NAME": "Randolph Hospital",
                    "ADDRESS": "364 White Oak St",
                    "CITY": "Asheboro",
                    "ZIP": "27203",
                    "COUNTY": "Randolph",
                    "PHONE": "(336) 625-5151",
                    "CANCER": "yes",
                    "POLYGONID": "0",
                    "SCALE": "1",
                    "ANGLE": "1"
                }
            }
        ]
    }
]
```

## SEE ALSO

*[d.what.rast](d.what.rast.md), [d.what.vect](d.what.vect.md),
[v.rast.stats](v.rast.stats.md), [v.vect.stats](v.vect.stats.md),
[v.what.rast](v.what.rast.md), [v.what.rast3](v.what.rast3.md),
[v.what.vect](v.what.vect.md)*

## AUTHOR

Trevor Wiens  
Edmonton, Alberta, Canada
