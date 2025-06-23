## DESCRIPTION

*v.in.dxf* converts DXF format CAD files to GRASS vector format. The
following graphical objects (DXF entities) are supported:

- point
  - **DXF POINT**
- line
  - **DXF LINE**
  - **DXF POLYLINE**
  - **DXF LWPOLYLINE**
  - **DXF ARC**
  - **DXF CIRCLE**
  - **DXF TEXT**
- face
  - **DXF POLYFACE MESHES**
  - **DXF 3DFACE**

Table and column names are changed to lowercase characters for easier
SQL usage (lowercase table/column names avoid the need to quote them if
the attribute table is stored in a SQL DBMS such as PostgreSQL).

The "layer" column will contain the name(s) of the DXF input layer(s).
The DXF entity type string will be stored in the "entity" column as
uppercase.

The "handle" column can be used to store small bits of data associated
with any entity in the DXF file (i.e., entity handle or unique object
identifiers in the layer). The entity handle is a "text string of up to
16 hexadecimal digits", which is a 64-bit integer (currently not
supported by GRASS database drivers). For text type entities, the text
value will be stored in the "label" column of the GRASS vector output
map. Neither the "handle" nor "label" column is mandatory.

## REFERENCES

[AutoCad DXF](https://en.wikipedia.org/wiki/AutoCAD_DXF) (from
Wikipedia, the free encyclopedia)  
[DXF
References](http://usa.autodesk.com/adsk/servlet/item?siteID=123112&id=12272454&linkID=10809853)
(Autodesk-supplied documentation)

## SEE ALSO

*[v.out.dxf](v.out.dxf.md), [v.in.ogr](v.in.ogr.md),
[v.out.ogr](v.out.ogr.md)*

*[How-to import DXF files in
wxGUI](https://grasswiki.osgeo.org/wiki/Import_DXF) (from GRASS User
Wiki)*

## AUTHORS

Original written by Chuck Ehlschlaeger, 6/1989  
Revised by Dave Gerdes, 12/1989  
US Army Construction Engineering Research Lab

Updated for GRASS 6 and 3D support. Huidae Cho, 3/2006  
With minor additions by Benjamin Ducke (Oxford Archaeology), 4/2009
