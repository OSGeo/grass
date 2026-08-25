## DESCRIPTION

*r.report* allows the user to set up a series of report parameters to be
applied to a raster map, and creates a report. The report will print out
to the standard output if **output** parameter is not given.
The report can be either formatted in a human-readable way (default) or in a
JSON format using the **format=json** parameter.

The report itself consists of two parts, a header section and the main
body of the report.

The header section of the report identifies the raster map(s) (by map
name and title), project, mapset, report date, and the region of
interest. The area of interest is described in two parts: the user's
current geographic region is presented, and the mask is presented (if
any is used).

The main body of the report consists of from one to three tables which
present the statistics for each category and the totals for each unit
column. Note that the statistics is always computed in the current
computational region.

When multiple (typically two) raster maps are specified,
cross-tabulation table for each combination of categories in the raster
maps will be computed and formatted in a human-readable way (see
example).

## NOTES

Note that, unlike *[r.stats](r.stats.md)*, *r.report* allows the user to
select the specific units of measure in which statistics will be
reported. To output computer-friendly data suitable for importing into a
spreadsheet use the *[r.stats](r.stats.md)* module. In fact *r.report*
is running *[r.stats](r.stats.md)* in the background and reformatting
the results to be more human-friendly.

## EXAMPLE

Report sorted areas in square miles and acres for each category. No-data
are not reported (see **-n** flag).

=== "Command line"

    ```sh
    g.region raster=geology_30m
    r.report -n map=geology_30m units=mi,a sort=desc
    ```

    ```text
    +-----------------------------------------------------------------------------+
    |                         RASTER MAP CATEGORY REPORT                          |
    |PROJECT: nc_spm_08_grass7                            Tue Jun 24 13:44:59 2025|
    |-----------------------------------------------------------------------------|
    |          north: 228500    east: 645000                                      |
    |REGION    south: 215000    west: 630000                                      |
    |          res:       30    res:      30                                      |
    |-----------------------------------------------------------------------------|
    |MASK: none                                                                   |
    |-----------------------------------------------------------------------------|
    |MAP: South-West Wake county: geology derived from vector map (geology_30m in |
    |-----------------------------------------------------------------------------|
    |                 Category Information                  |    square|          |
    |  #|description                                        |     miles|     acres|
    |-----------------------------------------------------------------------------|
    |217|CZfg . . . . . . . . . . . . . . . . . . . . . . . | 28.014105|17,929.027|
    |270|CZig . . . . . . . . . . . . . . . . . . . . . . . | 26.616840|17,034.778|
    |405|CZbg . . . . . . . . . . . . . . . . . . . . . . . |  9.795798|  6269.311|
    |262|CZlg . . . . . . . . . . . . . . . . . . . . . . . |  7.671232|  4909.589|
    |862|CZam . . . . . . . . . . . . . . . . . . . . . . . |  2.383100|  1525.184|
    |910|CZbg . . . . . . . . . . . . . . . . . . . . . . . |  1.736070|  1111.085|
    |583|CZve . . . . . . . . . . . . . . . . . . . . . . . |  0.834328|   533.970|
    |921|Km . . . . . . . . . . . . . . . . . . . . . . . . |  0.483709|   309.574|
    |766|CZg. . . . . . . . . . . . . . . . . . . . . . . . |  0.273129|   174.802|
    |720|CZam . . . . . . . . . . . . . . . . . . . . . . . |  0.186256|   119.204|
    |946|CZam . . . . . . . . . . . . . . . . . . . . . . . |  0.157066|   100.522|
    |948|CZam . . . . . . . . . . . . . . . . . . . . . . . |  0.033707|    21.572|
    |945|CZbg . . . . . . . . . . . . . . . . . . . . . . . |  0.000347|     0.222|
    |-----------------------------------------------------------------------------|
    |TOTAL                                                  | 78.185687|50,038.840|
    +-----------------------------------------------------------------------------+
    ```

=== "Python (grass.script)"

    ```python
    import grass.script as gs

    gs.run_command("g.region", raster="geology_30m")
    report = gs.parse_command(
        "r.report", flags="n", map="geology_30m", units="mi,a", sort="desc", format="json"
    )
    print(report["categories"][0])
    ```

    ```text
    {'category': 217, 'label': 'CZfg', 'units': [{'unit': 'square miles', 'value': 28.01410543563742}, {'unit': 'acres', 'value': 17929.02747880792}]}
    ```

    The corresponding (shortened) JSON output is:

    ```json
    {
        "project": "nc_spm_08_grass7",
        "created": "2025-06-24T13:33:05-0400",
        "region": {
            "north": 228500,
            "south": 215000,
            "east": 645000,
            "west": 630000,
            "ewres": 30,
            "nsres": 30
        },
        "mask": null,
        "maps": [
            {
                "name": "geology_30m",
                "title": "South-West Wake county: geology derived from vector map",
                "type": "raster"
            }
        ],
        "categories": [
            {
                "category": 217,
                "label": "CZfg",
                "units": [
                    {
                        "unit": "square miles",
                        "value": 28.014105435637418
                    },
                    {
                        "unit": "acres",
                        "value": 17929.027478807919
                    }
                ]
            },
            {
                "category": 270,
                "label": "CZig",
                "units": [
                    {
                        "unit": "square miles",
                        "value": 26.61684033408816
                    },
                    {
                        "unit": "acres",
                        "value": 17034.777813816392
                    }
                ]
            },
        ...
        ],
        "totals": [
            {
                "unit": "square miles",
                "value": 78.185687104845314
            },
            {
                "unit": "acres",
                "value": 50038.839747100916
            }
        ]
    }
    ```

Report areas for each category of land use for each zipcode (included
only part of the table):

=== "Command line"

    ```sh
    g.region raster=zipcodes@PERMANENT
    r.report map=zipcodes@PERMANENT,landclass96@PERMANENT units=h,p
    ```

    ```sh
    +-----------------------------------------------------------------------------+
    |                         RASTER MAP CATEGORY REPORT                          |
    |PROJECT: nc_spm_08_grass7                            Tue Jun 24 13:47:15 2025|
    |-----------------------------------------------------------------------------|
    |          north: 228500    east: 645000                                      |
    |REGION    south: 215000    west: 630000                                      |
    |          res:       10    res:      10                                      |
    |-----------------------------------------------------------------------------|
    |MASK: none                                                                   |
    |-----------------------------------------------------------------------------|
    |MAPS: South West Wake: Zipcode areas derived from vector map (zipcodes@PERMAN|
    |        South-West Wake county: Simplified landuse classes (landclass96@PERMA|
    |-----------------------------------------------------------------------------|
    |                   Category Information                    |          |   %  |
    |    #|description                                          |  hectares| cover|
    |-----------------------------------------------------------------------------|
    |27511|CARY                                                 |  1058.000|  5.22|
    |     |-----------------------------------------------------|----------|------|
    |     |1|developed. . . . . . . . . . . . . . . . . . . . . |   197.940| 18.71|
    |     |3|herbaceous . . . . . . . . . . . . . . . . . . . . |    24.440|  2.31|
    |     |4|shrubland. . . . . . . . . . . . . . . . . . . . . |    58.080|  5.49|
    |     |5|forest . . . . . . . . . . . . . . . . . . . . . . |   775.910| 73.34|
    |     |6|water. . . . . . . . . . . . . . . . . . . . . . . |     1.540|  0.15|
    |     |*|no data. . . . . . . . . . . . . . . . . . . . . . |     0.090|  0.01|
    |-----------------------------------------------------------|----------|------|
    |27513|CARY                                                 |   205.300|  1.01|
    |     |-----------------------------------------------------|----------|------|
    |     |1|developed. . . . . . . . . . . . . . . . . . . . . |    32.580| 15.87|
    |     |3|herbaceous . . . . . . . . . . . . . . . . . . . . |     5.930|  2.89|
    |     |4|shrubland. . . . . . . . . . . . . . . . . . . . . |    52.030| 25.34|
    |     |5|forest . . . . . . . . . . . . . . . . . . . . . . |   114.760| 55.90|
    |-----------------------------------------------------------|----------|------|
    ...
    |-----------------------------------------------------------------------------|
    |TOTAL                                                      |20,250.000|100.00|
    +-----------------------------------------------------------------------------+
    ```

=== "Python (grass.script)"

    ```python
    import grass.script as gs

    gs.run_command("g.region", raster="zipcodes@PERMANENT")
    report = gs.parse_command(
        "r.report",
        map=["zipcodes@PERMANENT", "landclass96@PERMANENT"],
        units="h,p",
        format="json",
    )
    print(report["categories"][0]["categories"][0])
    ```

    ```text
    {'category': 1, 'label': 'developed', 'units': [{'unit': 'hectares', 'value': 197.94}, {'unit': 'percent', 'value': 18.708884688090738}]}
    ```

    The corresponding (shortened) JSON output is:

    ```json
    {
        "project": "nc_spm_08_grass7",
        "created": "2025-06-24T13:58:07-0400",
        "region": {
            "north": 228500,
            "south": 215000,
            "east": 645000,
            "west": 630000,
            "ewres": 10,
            "nsres": 10
        },
        "mask": null,
        "maps": [
            {
                "name": "zipcodes@PERMANENT",
                "title": "South West Wake: Zipcode areas derived from vector map",
                "type": "raster"
            },
            {
                "name": "landclass96@PERMANENT",
                "title": "South-West Wake county: Simplified landuse classes",
                "type": "raster"
            }
        ],
        "categories": [
            {
                "category": 27511,
                "label": "CARY",
                "units": [
                    {
                        "unit": "hectares",
                        "value": 1058
                    },
                    {
                        "unit": "percent",
                        "value": 5.2246913580246916
                    }
                ],
                "categories": [
                    {
                        "category": 1,
                        "label": "developed",
                        "units": [
                            {
                                "unit": "hectares",
                                "value": 197.94
                            },
                            {
                                "unit": "percent",
                                "value": 18.708884688090738
                            }
                        ]
                    },
                ...
        "totals": [
            {
                "unit": "hectares",
                "value": 20250
            },
            {
                "unit": "percent",
                "value": 100
            }
        ]
    }
    ```

## SEE ALSO

*[r.stats](r.stats.md), [g.region](g.region.md), [r.coin](r.coin.md),
[r.describe](r.describe.md), [r.info](r.info.md),
[r.univar](r.univar.md)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research
Laboratory  
Sort option by Martin Landa, Czech Technical University in Prague, 2013
