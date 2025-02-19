## DESCRIPTION

*v.in.geonames* imports Geonames.org country files (Gazetteer data) into
a GRASS vector points map. The country files can be downloaded from the
[GeoNames Data Dump Server](https://download.geonames.org/export/dump/).
Only original files can be processed (unzip compressed file first).
These Geonames files are encoded in UTF-8 which is maintained in the
GRASS database.

## NOTES

*v.in.geonames* calls *[v.in.ascii](v.in.ascii.md)* to import data into
GRASS.

The current DB connection is used to write the database table (see
*[db.connect](db.connect.md)*). If importing into a [DBF
database](grass-dbf.md), the original column names longer that 10
characters are shortened to 10 characters to meet the DBF column name
restrictions. If this is a problem consider choosing another database
driver with *[db.connect](db.connect.md)* (eg. to [SQLite
driver](grass-sqlite.md)).

The main 'geoname' table has the following fields

```sh
geonameid         : integer id of record in geonames database
name              : name of geographical point (utf8) varchar(200)
asciiname         : name of geographical point in plain ascii characters, varchar(200)
alternatenames    : alternatenames, comma separated varchar(4000)
latitude          : latitude in decimal degrees (wgs84)
longitude         : longitude in decimal degrees (wgs84)
feature class     : see https://www.geonames.org/export/codes.html, char(1)
feature code      : see https://www.geonames.org/export/codes.html, varchar(10)
country code      : ISO-3166 2-letter country code, 2 characters
cc2               : alternate country codes, comma separated, ISO-3166 2-letter country code, 60 characters
admin1 code       : fipscode (subject to change to iso code), isocode for the us and ch, see file admin1Codes.txt for display names of this code; varchar(20)
admin2 code       : code for the second administrative division, a county in the US, see file admin2Codes.txt; varchar(80)
admin3 code       : code for third level administrative division, varchar(20)
admin4 code       : code for fourth level administrative division, varchar(20)
population        : integer
elevation         : in meters, integer
gtopo30           : average elevation of 30'x30' (ca 900mx900m) area in meters, integer
timezone          : the timezone id (see file https://download.geonames.org/export/dump/timeZones.txt)
modification date : date of last modification in yyyy-MM-dd format
```

## EXAMPLE

Download and import geonames for Czech Republic.

```sh
wget https://download.geonames.org/export/dump/CZ.zip
unzip CZ.zip

v.in.geonames input=CZ.txt output=geonames_cz
```

## REFERENCES

- [GeoNames Web site](https://www.geonames.org)
- [GeoNames Data Dump
  Server](https://download.geonames.org/export/dump/)

## SEE ALSO

*[db.connect](db.connect.md), [v.in.ascii](v.in.ascii.md),
[v.select](v.select.md)*

## AUTHOR

[Markus Neteler](https://grassbook.org/)
