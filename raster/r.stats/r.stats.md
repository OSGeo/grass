## DESCRIPTION

*r.stats* calculates the area present in each of the categories or
floating-point intervals of user-selected **input** raster map. Area
statistics are given in units of square meters and/or cell counts. This
analysis uses the current geographic region (*[g.region](g.region.md)*)
and mask settings (*[r.mask](r.mask.md)*). The output statistics can be
saved to a **output** file.

Area statistics is printed in square meters for each category when
**-a** is given. Similarly if **-c** flag is chosen, areas will be
stated also in number of cells.

## NOTES

If a single raster map is specified, a list of categories will be
printed. The **-x** flag will print x and y (column and row) starting
with 1 (both first row and first column are indexed with 1). If multiple
raster maps are specified, a cross-tabulation table for each combination
of categories in the raster maps will be printed.

For example, if one raster map was specified, the output would look
like:

```sh
1 1350000.00
2 4940000.00
3 8870000.00
```

If three raster maps were specified, the output would look like:

```sh
0 0 0 8027500.00
0 1 0 1152500.00
1 0 0 164227500.00
1 0 1 2177500.00
1 1 0 140092500.00
1 1 1 3355000.00
2 0 0 31277500.00
2 0 1 2490000.00
2 1 0 24207500.00
2 1 1 1752500.00
3 0 0 17140000.00
3 1 0 11270000.00
3 1 1 2500.00
```

Within each grouping, the first field represents the category value of
first raster map, the second represents the category values associated
with second raster map, the third represents category values for third
raster map, and the last field gives the area in square meters for the
particular combination of these three raster maps' categories. For
example, above, combination 3,1,1 covered 2500 square meters. Fields are
separated by the **separator** option. The output from *r.stats* is
sorted by category or category intervals (for floating-point raster
maps).

Note that the user has only the option of printing out cell statistics
in terms of cell counts and/or area totals. Users wishing to use
different units than are available here should use
*[r.report](r.report.md)*.

## EXAMPLES

### Report sorted number of cells and area for each category

Report sorted number of cells and area for each category
in a single raster map:

<!-- markdownlint-disable MD046 -->
=== "Command line"

    ```sh
    g.region raster=geology_30m
    r.stats -ac input=geology_30m sort=desc sep=tab
    ```

    Output (category, area, cell count):


    ```text
    217 72556200.000000 80618
    270 68937300.000000 76597
    405 25371000.000000 28190
    262 19868400.000000 22076
    862 6172200.000000  6858
    910 4496400.000000  4996
    583 2160900.000000  2401
    921 1252800.000000  1392
    766 707400.000000   786
    720 482400.000000   536
    946 406800.000000   452
    948 87300.000000    97
    945 900.000000      1
    ```

=== "Python (grass.script)"

    ```python
    import grass.script as gs

    gs.run_command("g.region", raster="geology_30m")
    data = gs.parse_command("r.stats", flags="a", input="geology_30m", format="json")
    for record in data:
        cat = record['categories'][0]['category']
        print(f"Category {cat}: {record['count']} cells and {record['area'] / 1e6} km2")
    ```

    Output:

    ```text
    Category 217: 80618 cells and 72.5562 km2
    Category 270: 76597 cells and 68.9373 km2
    Category 405: 28190 cells and 25.371 km2
    Category 262: 22076 cells and 19.8684 km2
    Category 862: 6858 cells and 6.1722 km2
    Category 910: 4996 cells and 4.4964 km2
    Category 583: 2401 cells and 2.1609 km2
    Category 921: 1392 cells and 1.2528 km2
    Category 766: 786 cells and 0.7074 km2
    Category 720: 536 cells and 0.4824 km2
    Category 946: 452 cells and 0.4068 km2
    Category 948: 97 cells and 0.0873 km2
    Category 945: 1 cells and 0.0009 km2
    ```

    The JSON output looks like:

    ```json
    [
        {
            "categories": [
                {
                    "category": 217
                }
            ],
            "area": 72556200,
            "count": 80618
        },
        {
            "categories": [
                {
                    "category": 270
                }
            ],
            "area": 68937300,
            "count": 76597
        },
    ...
    ]
    ```

### Report area, number of cells, and percents in multiple raster maps

Report area, number of cells, and percents (separated by tabs) for each
category in multiple raster maps (suppress NULL data):

=== "Command line"

    ```sh
    g.region raster=towns
    r.stats -nacp input=towns,urban separator=tab
    ```

    Output (town, urban, area, number of cells, percent):

    ```text
    1 55 23475900.000000 234759 11.65%
    2 55 14142700.000000 141427 7.02%
    3 55 1519700.000000  15197  0.75%
    4 55 16051400.000000 160514 7.97%
    5 55 99004400.000000 990044 49.14%
    6 55 19888500.000000 198885 9.87%
    ```

=== "Python (grass.script)"

    ```python
    import grass.script as gs

    gs.run_command("g.region", raster="towns")
    data = gs.parse_command("r.stats", flags="nacp", input=["towns", "urban"], format="json")
    for record in data:
        categories = [str(cat["category"]) for cat in record["categories"]]
        print(f"Categories {' and '.join(categories)}: {record['percent']:.2f} %")
    ```

    Output:

    ```text
    Categories 1 and 55: 11.65 %
    Categories 2 and 55: 7.02 %
    Categories 3 and 55: 0.75 %
    Categories 4 and 55: 7.97 %
    Categories 5 and 55: 49.14 %
    Categories 6 and 55: 9.87 %
    ```

    The JSON output looks like:

    ```json
    [
        {
            "categories": [
                {
                    "category": 1
                },
                {
                    "category": 55
                }
            ],
            "area": 23475900,
            "count": 234759,
            "percent": 11.651235678463038
        },
        {
            "categories": [
                {
                    "category": 2
                },
                {
                    "category": 55
                }
            ],
            "area": 14142700,
            "count": 141427,
            "percent": 7.0191102718021128
        },
    ...
    ]
    ```

### Report sorted area intervals of floating-point raster map

Report sorted area for each interval of floating-point input raster map.
Number of intervals are given by **nsteps** option.

=== "Command line"

    ```sh
    g.region raster=elevation
    r.stats -an input=elevation nsteps=5 sort=desc separator=tab
    ```

    Output:

    ```text
    95.879221-116.029436  67133500.000000
    116.029436-136.17965  54757900.000000
    75.729007-95.879221   47817700.000000
    136.17965-156.329865  26061600.000000
    55.578793-75.729007   6729300.000000
    ```

=== "Python (grass.script)"

    ```python
    import grass.script as gs

    gs.run_command("g.region", raster="elevation")
    data = gs.parse_command("r.stats", flags="an", input="elevation", nsteps=5, sort="desc", format="json")
    for record in data:
        from_to = record['categories'][0]['range']
        print(f"Elevation {from_to['from']:.2f}-{from_to['to']:.2f}: {record['area'] / 1e6:.2f} km2")

    ```

    Output: 

    ```text
    Elevation 95.88-116.03: 67.13 km2
    Elevation 116.03-136.18: 54.76 km2
    Elevation 75.73-95.88: 47.82 km2
    Elevation 136.18-156.33: 26.06 km2
    Elevation 55.58-75.73: 6.73 km2
    ```

    The JSON output looks like:

    ```json
    [
        {
            "categories": [
                {
                    "range": {
                        "from": 95.879221343994146,
                        "to": 105.9543285369873
                    }
                }
            ],
            "area": 37063800
        },
        {
            "categories": [
                {
                    "range": {
                        "from": 85.804114151000974,
                        "to": 95.879221343994146
                    }
                }
            ],
            "area": 30826100
        },
    ...
    ]
    ```

### Report grid coordinates and category values of a raster map

Report category and east, north and row, column of a rasterized firestations map:

=== "Command line"

    ```sh
    g.region region=wake_30m
    v.to.rast input=firestations output=firestations use=cat
    r.stats input=firestations -gxn
    ```

    Output (east, north, column, row, category):

    ```text
    641835 253485 1033 155 42
    644715 247785 1129 345 41
    654045 246915 1440 374 38
    641865 243615 1034 484 40
    638055 243285 907 495 43
    ...
    ```

=== "Python (grass.script)"

    ```python
    import grass.script as gs

    gs.run_command("g.region", region="wake_30m")
    gs.run_command("v.to.rast", input="firestations", output="firestations", use="cat")
    data = gs.parse_command("r.stats", flags="gxn", input="firestations", format="json")
    print(data[0])
    ```

    Output:

    ```text
    {'east': 641835, 'north': 253485, 'col': 1033, 'row': 155, 'categories': [{'category': 42}]}
    ```

    The JSON output looks like:

    ```json
    [
        {
            "east": 641835,
            "north": 253485,
            "col": 1033,
            "row": 155,
            "categories": [
                {
                    "category": 42
                }
            ]
        },
        {
            "east": 644715,
            "north": 247785,
            "col": 1129,
            "row": 345,
            "categories": [
                {
                    "category": 41
                }
            ]
        },
    ...
    ```

### Report raster cell counts in multiple raster maps

Report raster cell counts of landuse and geological categories within
zipcode areas:

=== "Command line"

    ```sh
    g.region raster=zipcodes
    # landuse/landcover, geology and zipcodes with category labels
    r.stats -c input=landclass96,zipcodes,geology_30m separator=comma -l
    ```

    Output:

    ```text
    1,developed,27511,CARY,405,CZbg,18410
    1,developed,27511,CARY,583,CZve,1298
    1,developed,27511,CARY,862,CZam,86
    1,developed,27513,CARY,405,CZbg,2287
    1,developed,27513,CARY,583,CZve,971
    1,developed,27518,CARY,217,CZfg,5724
    1,developed,27518,CARY,405,CZbg,3910
    1,developed,27518,CARY,862,CZam,1012
    1,developed,27529,GARNER,270,CZig,61497
    1,developed,27539,APEX,921,Km,246
    ...
    ```

=== "Python (grass.script)"

    ```python
    import grass.script as gs

    gs.run_command("g.region", raster="zipcodes")
    # landuse/landcover, geology and zipcodes with category labels
    data = gs.parse_command("r.stats", flags="c", input=["landclass96", "zipcodes", "geology_30m"], format="json")
    print(data[0])
    ```

    Output:

    ```text
    {'categories': [{'category': 1}, {'category': 27511}, {'category': 405}], 'count': 18410}
    ```

    The JSON output looks like:

    ```json
    [
        {
            "categories": [
                {
                    "category": 1,
                    "label": "developed"
                },
                {
                    "category": 27511,
                    "label": "CARY"
                },
                {
                    "category": 405,
                    "label": "CZbg"
                }
            ],
            "count": 18410
        },
    ...
    ]
    ```

### Read r.stats results into a Pandas DataFrame

```python
import pandas as pd
import grass.script as gs


gs.run_command("g.region", raster="zipcodes")
maps = ["landclass96", "zipcodes", "geology_30m"]
data = gs.parse_command("r.stats", flags="an", input=maps, format="json")

# Map to raster names
for item in data:
    for i, name in enumerate(maps):
        item[name] = item["categories"][i]["category"]

# Create DataFrame and drop the original 'categories'
df = pd.DataFrame(data).drop(columns="categories")
print(df)
```

Output:

```text
        area  landclass96  zipcodes  geology_30m
0    1841000            1     27511          405
1     129800            1     27511          583
2       8600            1     27511          862
3     228700            1     27513          405
4      97100            1     27513          583
..       ...          ...       ...          ...
165    64400            6     27607          217
166     7000            6     27607          262
167    19200            6     27610          270
168   106600            7     27603          270
169    54400            7     27607          217

[170 rows x 4 columns]
```

<!-- markdown-restore -->

## SEE ALSO

*[g.region](g.region.md), [r.report](r.report.md), [r.coin](r.coin.md),
[r.describe](r.describe.md), [r.stats.quantile](r.stats.quantile.md),
[r.stats.zonal](r.stats.zonal.md), [r.statistics](r.statistics.md),
[r.univar](r.univar.md)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research
Laboratory  
Sort option by Martin Landa, Czech Technical University in Prague, 2013
