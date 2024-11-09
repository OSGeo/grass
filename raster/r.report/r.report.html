<h2>DESCRIPTION</h2>

<em>r.report</em> allows the user to set up a series of report
parameters to be applied to a raster map, and creates a report. The
report will print out to the standard output if <b>output</b>
parameter is not given.

<p>
The report itself consists of two parts, a header section and the main
body of the report.

<p>
The header section of the report identifies the raster map(s) (by map
name and title), project, mapset, report date, and the region of
interest. The area of interest is described in two parts: the user's
current geographic region is presented, and the mask is presented (if
any is used).

<p>
The main body of the report consists of from one to three tables which
present the statistics for each category and the totals for each unit
column. Note that the statistics is always computed in the current
geographical region.

<p>
When multiple (typically two) raster maps are specified, cross-tabulation
table for each combination of categories in the raster maps will be computed
and formatted in a human-readable way (see example).

<h2>NOTES</h2>

<!--
If the user runs <em>r.report</em> interactively and saves the report output
in a file, this file will be placed into the user's current working
directory.

If the user runs <em>r.report</em> non-interactively, report output can be
saved by redirecting it to a file or a printer using the UNIX redirection
mechanism.
-->

Note that, unlike <em><a href="r.stats.html">r.stats</a></em>,
<em>r.report</em> allows the user to select the specific units of
measure in which statistics will be reported. To output
computer-friendly data suitable for importing into a spreadsheet use
the <em><a href="r.stats.html">r.stats</a></em> module. In
fact <em>r.report</em> is running
<em><a href="r.stats.html">r.stats</a></em> in the background and
reformatting the results to be more human-friendly.

<h2>EXAMPLE</h2>

Report sorted areas in square miles and acres for each
category. No-data are not reported (see <b>-n</b> flag).

<div class="code"><pre>
r.report -n map=geology_30m units=mi,a sort=desc
</pre></div>
<div class="code"><pre>
+-----------------------------------------------------------------------------+
|                         RASTER MAP CATEGORY REPORT                          |
|PROJECT: nc_spm_08_grass7                            Fri Dec  6 17:00:21 2013|
|-----------------------------------------------------------------------------|
|          north: 279073.97546639    east: 798143.31179672                    |
|REGION    south: 113673.97546639    west: 595143.31179672                    |
|          res:               200    res:              200                    |
|-----------------------------------------------------------------------------|
|MASK: none                                                                   |
|-----------------------------------------------------------------------------|
|MAP: South-West Wake county: geology derived from vector map (geology_30m in |
|-----------------------------------------------------------------------------|
|                 Category Information                  |    square|          |
|  #|description                                        |     miles|     acres|
|-----------------------------------------------------------------------------|
|217|CZfg . . . . . . . . . . . . . . . . . . . . . . . | 27.783911|17,781.703|
|270|CZig . . . . . . . . . . . . . . . . . . . . . . . | 26.162282|16,743.861|
|405|CZbg . . . . . . . . . . . . . . . . . . . . . . . |  9.698886|  6207.287|
|262|CZlg . . . . . . . . . . . . . . . . . . . . . . . |  7.629379|  4882.802|
|862|CZam . . . . . . . . . . . . . . . . . . . . . . . |  2.532830|  1621.011|
|910|CZbg . . . . . . . . . . . . . . . . . . . . . . . |  1.683405|  1077.379|
|583|CZve . . . . . . . . . . . . . . . . . . . . . . . |  0.972977|   622.706|
|921|Km . . . . . . . . . . . . . . . . . . . . . . . . |  0.463323|   296.526|
|766|CZg. . . . . . . . . . . . . . . . . . . . . . . . |  0.324326|   207.569|
|720|CZam . . . . . . . . . . . . . . . . . . . . . . . |  0.185329|   118.611|
|946|CZam . . . . . . . . . . . . . . . . . . . . . . . |  0.138997|    88.958|
|948|CZam . . . . . . . . . . . . . . . . . . . . . . . |  0.030888|    19.768|
|-----------------------------------------------------------------------------|
|TOTAL                                                  | 77.606534|49,668.182|
+-----------------------------------------------------------------------------+
</pre></div>
Report areas for each category of land use for each zipcode (included only part of the table):

<div class="code"><pre>
r.report map=zipcodes@PERMANENT,landclass96@PERMANENT units=h,p
</pre></div>
<div class="code"><pre>
+-----------------------------------------------------------------------------+
|                         RASTER MAP CATEGORY REPORT                          |
|PROJECT: nc_spm_08_latest                            Tue Feb 11 10:10:46 2014|
|-----------------------------------------------------------------------------|
|          north: 228527.25    east: 644971                                   |
|REGION    south: 215018.25    west: 629980                                   |
|          res:        28.5    res:    28.5                                   |
|-----------------------------------------------------------------------------|
|MASK: none                                                                   |
|-----------------------------------------------------------------------------|
|MAPS: South West Wake: Zipcode areas derived from vector map (zipcodes@PERMAN|
|        South-West Wake county: Simplified landuse classes (landclass96@PERMA|
|-----------------------------------------------------------------------------|
|                   Category Information                    |          |   %  |
|    #|description                                          |  hectares| cover|
|-----------------------------------------------------------------------------|
|27511|CARY                                                 |  1053.813|  5.20|
|     |-----------------------------------------------------|----------|------|
|     |1|developed. . . . . . . . . . . . . . . . . . . . . |   197.214| 18.71|
|     |3|herbaceous . . . . . . . . . . . . . . . . . . . . |    25.017|  2.37|
|     |4|shrubland. . . . . . . . . . . . . . . . . . . . . |    58.563|  5.56|
|     |5|forest . . . . . . . . . . . . . . . . . . . . . . |   771.313| 73.19|
|     |6|water. . . . . . . . . . . . . . . . . . . . . . . |     1.625|  0.15|
|     |*|no data. . . . . . . . . . . . . . . . . . . . . . |     0.081|  0.01|
|-----------------------------------------------------------|----------|------|
|27513|CARY                                                 |   204.525|  1.01|
|     |-----------------------------------------------------|----------|------|
|     |1|developed. . . . . . . . . . . . . . . . . . . . . |    32.571| 15.93|
|     |3|herbaceous . . . . . . . . . . . . . . . . . . . . |     6.011|  2.94|
|     |4|shrubland. . . . . . . . . . . . . . . . . . . . . |    51.659| 25.26|
|     |5|forest . . . . . . . . . . . . . . . . . . . . . . |   114.284| 55.88|
|-----------------------------------------------------------|----------|------|
...
|-----------------------------------------------------------------------------|
|TOTAL                                                      |22,968.900|100.00|
+-----------------------------------------------------------------------------+
</pre></div>

The output from <em>r.report</em> can be output in JSON by passing the <b>format=json</b> option.

<div class="code"><pre>
r.report -n -a map=towns,elevation units=miles,meters,kilometers,acres,hectares,cells,percent nsteps=2 format=json
</pre></div>

<div class="code"><pre>
{
    "location": "nc_spm_08_grass7",
    "created": "2024-07-24T14:59:09+0530",
    "region": {
        "north": 320000,
        "south": 10000,
        "east": 935000,
        "west": 120000,
        "ew_res": 500,
        "ns_res": 500
    },
    "mask": null,
    "maps": [
        {
            "name": "towns",
            "label": "South West Wake: Cities and towns derived from zipcodes",
            "type": "raster",
        },
        {
            "name": "zipcodes",
            "label": "South West Wake: Zipcode areas derived from vector map",
            "type": "raster",
        }
    ],
    "categories": [
        {
            "category": 1,
            "label": "CARY",
            "units": [
                {
                    "unit": "square miles",
                    "value": 10.231707201374819
                },
                {
                    "unit": "square meters",
                    "value": 26500000
                },
                {
                    "unit": "square kilometers",
                    "value": 26.5
                },
                {
                    "unit": "acres",
                    "value": 6548.2926088798722
                },
                {
                    "unit": "hectares",
                    "value": 2650
                },
                {
                    "unit": "cell counts",
                    "value": 106
                },
                {
                    "unit": "% cover",
                    "value": 13.086419753086419
                }
            ],
            "categories": [
                {
                    "category": 1,
                    "label": "from to",
                    "range": {
                        "from": 55.578792572021484,
                        "to": 105.9543285369873
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 0.8687298567205034
                        },
                        {
                            "unit": "square meters",
                            "value": 2250000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 2.25
                        },
                        {
                            "unit": "acres",
                            "value": 555.98710830112122
                        },
                        {
                            "unit": "hectares",
                            "value": 225
                        },
                        {
                            "unit": "cell counts",
                            "value": 9
                        },
                        {
                            "unit": "% cover",
                            "value": 8.4905660377358494
                        }
                    ]
                },
                {
                    "category": 2,
                    "label": "from to",
                    "range": {
                        "from": 105.9543285369873,
                        "to": 156.32986450195312
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 9.3629773446543147
                        },
                        {
                            "unit": "square meters",
                            "value": 24250000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 24.25
                        },
                        {
                            "unit": "acres",
                            "value": 5992.305500578751
                        },
                        {
                            "unit": "hectares",
                            "value": 2425
                        },
                        {
                            "unit": "cell counts",
                            "value": 97
                        },
                        {
                            "unit": "% cover",
                            "value": 91.509433962264154
                        }
                    ]
                }
            ]
        },
        {
            "category": 2,
            "label": "GARNER",
            "units": [
                {
                    "unit": "square miles",
                    "value": 5.5019557592298556
                },
                {
                    "unit": "square meters",
                    "value": 14250000
                },
                {
                    "unit": "square kilometers",
                    "value": 14.25
                },
                {
                    "unit": "acres",
                    "value": 3521.2516859071011
                },
                {
                    "unit": "hectares",
                    "value": 1425
                },
                {
                    "unit": "cell counts",
                    "value": 57
                },
                {
                    "unit": "% cover",
                    "value": 7.0370370370370372
                }
            ],
            "categories": [
                {
                    "category": 1,
                    "label": "from to",
                    "range": {
                        "from": 55.578792572021484,
                        "to": 105.9543285369873
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 4.3436492836025176
                        },
                        {
                            "unit": "square meters",
                            "value": 11250000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 11.25
                        },
                        {
                            "unit": "acres",
                            "value": 2779.9355415056061
                        },
                        {
                            "unit": "hectares",
                            "value": 1125
                        },
                        {
                            "unit": "cell counts",
                            "value": 45
                        },
                        {
                            "unit": "% cover",
                            "value": 78.94736842105263
                        }
                    ]
                },
                {
                    "category": 2,
                    "label": "from to",
                    "range": {
                        "from": 105.9543285369873,
                        "to": 156.32986450195312
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 1.158306475627338
                        },
                        {
                            "unit": "square meters",
                            "value": 3000000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 3
                        },
                        {
                            "unit": "acres",
                            "value": 741.31614440149497
                        },
                        {
                            "unit": "hectares",
                            "value": 300
                        },
                        {
                            "unit": "cell counts",
                            "value": 12
                        },
                        {
                            "unit": "% cover",
                            "value": 21.05263157894737
                        }
                    ]
                }
            ]
        },
        {
            "category": 3,
            "label": "APEX",
            "units": [
                {
                    "unit": "square miles",
                    "value": 0.9652553963561149
                },
                {
                    "unit": "square meters",
                    "value": 2500000
                },
                {
                    "unit": "square kilometers",
                    "value": 2.5
                },
                {
                    "unit": "acres",
                    "value": 617.76345366791247
                },
                {
                    "unit": "hectares",
                    "value": 250
                },
                {
                    "unit": "cell counts",
                    "value": 10
                },
                {
                    "unit": "% cover",
                    "value": 1.2345679012345678
                }
            ],
            "categories": [
                {
                    "category": 1,
                    "label": "from to",
                    "range": {
                        "from": 55.578792572021484,
                        "to": 105.9543285369873
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 0.096525539635611488
                        },
                        {
                            "unit": "square meters",
                            "value": 250000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 0.25
                        },
                        {
                            "unit": "acres",
                            "value": 61.776345366791247
                        },
                        {
                            "unit": "hectares",
                            "value": 25
                        },
                        {
                            "unit": "cell counts",
                            "value": 1
                        },
                        {
                            "unit": "% cover",
                            "value": 10
                        }
                    ]
                },
                {
                    "category": 2,
                    "label": "from to",
                    "range": {
                        "from": 105.9543285369873,
                        "to": 156.32986450195312
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 0.8687298567205034
                        },
                        {
                            "unit": "square meters",
                            "value": 2250000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 2.25
                        },
                        {
                            "unit": "acres",
                            "value": 555.98710830112122
                        },
                        {
                            "unit": "hectares",
                            "value": 225
                        },
                        {
                            "unit": "cell counts",
                            "value": 9
                        },
                        {
                            "unit": "% cover",
                            "value": 90
                        }
                    ]
                }
            ]
        },
        {
            "category": 4,
            "label": "RALEIGH-CITY",
            "units": [
                {
                    "unit": "square miles",
                    "value": 6.0811089970435237
                },
                {
                    "unit": "square meters",
                    "value": 15750000
                },
                {
                    "unit": "square kilometers",
                    "value": 15.75
                },
                {
                    "unit": "acres",
                    "value": 3891.9097581078486
                },
                {
                    "unit": "hectares",
                    "value": 1575
                },
                {
                    "unit": "cell counts",
                    "value": 63
                },
                {
                    "unit": "% cover",
                    "value": 7.7777777777777777
                }
            ],
            "categories": [
                {
                    "category": 1,
                    "label": "from to",
                    "range": {
                        "from": 55.578792572021484,
                        "to": 105.9543285369873
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 5.3089046799586326
                        },
                        {
                            "unit": "square meters",
                            "value": 13750000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 13.75
                        },
                        {
                            "unit": "acres",
                            "value": 3397.6989951735186
                        },
                        {
                            "unit": "hectares",
                            "value": 1375
                        },
                        {
                            "unit": "cell counts",
                            "value": 55
                        },
                        {
                            "unit": "% cover",
                            "value": 87.301587301587304
                        }
                    ]
                },
                {
                    "category": 2,
                    "label": "from to",
                    "range": {
                        "from": 105.9543285369873,
                        "to": 156.32986450195312
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 0.7722043170848919
                        },
                        {
                            "unit": "square meters",
                            "value": 2000000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 2
                        },
                        {
                            "unit": "acres",
                            "value": 494.21076293432998
                        },
                        {
                            "unit": "hectares",
                            "value": 200
                        },
                        {
                            "unit": "cell counts",
                            "value": 8
                        },
                        {
                            "unit": "% cover",
                            "value": 12.698412698412698
                        }
                    ]
                }
            ]
        },
        {
            "category": 5,
            "label": "RALEIGH-SOUTH",
            "units": [
                {
                    "unit": "square miles",
                    "value": 47.394039961085241
                },
                {
                    "unit": "square meters",
                    "value": 122750000
                },
                {
                    "unit": "square kilometers",
                    "value": 122.75
                },
                {
                    "unit": "acres",
                    "value": 30332.185575094503
                },
                {
                    "unit": "hectares",
                    "value": 12275
                },
                {
                    "unit": "cell counts",
                    "value": 491
                },
                {
                    "unit": "% cover",
                    "value": 60.617283950617285
                }
            ],
            "categories": [
                {
                    "category": 1,
                    "label": "from to",
                    "range": {
                        "from": 55.578792572021484,
                        "to": 105.9543285369873
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 25.579268003437047
                        },
                        {
                            "unit": "square meters",
                            "value": 66250000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 66.25
                        },
                        {
                            "unit": "acres",
                            "value": 16370.731522199681
                        },
                        {
                            "unit": "hectares",
                            "value": 6625
                        },
                        {
                            "unit": "cell counts",
                            "value": 265
                        },
                        {
                            "unit": "% cover",
                            "value": 53.971486761710793
                        }
                    ]
                },
                {
                    "category": 2,
                    "label": "from to",
                    "range": {
                        "from": 105.9543285369873,
                        "to": 156.32986450195312
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 21.814771957648198
                        },
                        {
                            "unit": "square meters",
                            "value": 56500000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 56.5
                        },
                        {
                            "unit": "acres",
                            "value": 13961.454052894822
                        },
                        {
                            "unit": "hectares",
                            "value": 5650
                        },
                        {
                            "unit": "cell counts",
                            "value": 226
                        },
                        {
                            "unit": "% cover",
                            "value": 46.028513238289207
                        }
                    ]
                }
            ]
        },
        {
            "category": 6,
            "label": "RALEIGH-WEST",
            "units": [
                {
                    "unit": "square miles",
                    "value": 8.0116197897557537
                },
                {
                    "unit": "square meters",
                    "value": 20750000
                },
                {
                    "unit": "square kilometers",
                    "value": 20.75
                },
                {
                    "unit": "acres",
                    "value": 5127.4366654436735
                },
                {
                    "unit": "hectares",
                    "value": 2075
                },
                {
                    "unit": "cell counts",
                    "value": 83
                },
                {
                    "unit": "% cover",
                    "value": 10.246913580246913
                }
            ],
            "categories": [
                {
                    "category": 1,
                    "label": "from to",
                    "range": {
                        "from": 55.578792572021484,
                        "to": 105.9543285369873
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 0.096525539635611488
                        },
                        {
                            "unit": "square meters",
                            "value": 250000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 0.25
                        },
                        {
                            "unit": "acres",
                            "value": 61.776345366791247
                        },
                        {
                            "unit": "hectares",
                            "value": 25
                        },
                        {
                            "unit": "cell counts",
                            "value": 1
                        },
                        {
                            "unit": "% cover",
                            "value": 1.2048192771084338
                        }
                    ]
                },
                {
                    "category": 2,
                    "label": "from to",
                    "range": {
                        "from": 105.9543285369873,
                        "to": 156.32986450195312
                    },
                    "units": [
                        {
                            "unit": "square miles",
                            "value": 7.9150942501201422
                        },
                        {
                            "unit": "square meters",
                            "value": 20500000
                        },
                        {
                            "unit": "square kilometers",
                            "value": 20.5
                        },
                        {
                            "unit": "acres",
                            "value": 5065.6603200768823
                        },
                        {
                            "unit": "hectares",
                            "value": 2050
                        },
                        {
                            "unit": "cell counts",
                            "value": 82
                        },
                        {
                            "unit": "% cover",
                            "value": 98.795180722891573
                        }
                    ]
                }
            ]
        }
    ],
    "totals": [
        {
            "unit": "square miles",
            "value": 78.185687104845314
        },
        {
            "unit": "square meters",
            "value": 202500000
        },
        {
            "unit": "square kilometers",
            "value": 202.5
        },
        {
            "unit": "acres",
            "value": 50038.839747100916
        },
        {
            "unit": "hectares",
            "value": 20250
        },
        {
            "unit": "cell counts",
            "value": 810
        },
        {
            "unit": "% cover",
            "value": 100
        }
    ]
}
</pre></div>

<h2>SEE ALSO</h2>

<em>
<a href="r.stats.html">r.stats</a>,
<a href="g.region.html">g.region</a>,
<a href="r.coin.html">r.coin</a>,
<a href="r.describe.html">r.describe</a>,
<a href="r.info.html">r.info</a>,
<a href="r.univar.html">r.univar</a>
</em>

<h2>AUTHORS</h2>

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory<br>
Sort option by Martin Landa, Czech Technical University in Prague, 2013
