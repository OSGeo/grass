---
name: LRS
description: Toolset for LRS (Linear Referencing System)
---

# Toolset for LRS (Linear Referencing System)

## DESCRIPTION

A Linear Referencing System (LRS) is a system where features (points or
segments) are localized by a measure along a linear element. The LRS can
be used to reference events for any network of linear features, for
example roads, railways, rivers, pipelines, electric and telephone
lines, water and sewer networks. An event is defined in LRS by a route
ID and a measure. A route is a path on the network, usually composed
from more features in the input map. Events can be either points or
lines (segments).

LRS is created from input lines and points in vector map. Points - MP
(mileposts) must have attached attributes specifying line and distance.
The distances from the beginning of the linear feature in real world are
specified by MP+offset. Typically, MP is in kilometers and offset in
meters.

The implementation of LRS in GRASS has some particularities.

### Double referenced system

This feature gives a possibility to continue to use most of old
mileposts if only small part of linear object in real world has changed.
Example:

```sh
--- road (linear feature)
 +   MP (milepost, point, distance from the beginning in km)
```

Old situation:

```sh
+----+----+----+----+----+
0    2    3    4    5    6
```

New situation (for example a new bypass around the village)

```sh
          ?    ?
          +----+
          |    |
          |    |
+----+----+    +----+----+
0    2    3    4    5    6
```

The segment between km 3 and 4 is now longer, it is now 3 km not 1 km as
in old version. It would be expensive to change also all MP \>= 4, but
we cannot use km 4 twice. It is possible to use another notation for the
new segment, we reference the segment from the kilometer 3, using only
offset.

```sh
      3+1000  3+2000
          +----+
          |    |
          |    |
+----+----+    +----+----+
0    2    3  3+3000 5    6
               4
```

This way, there is no ambiguity and minimal changes are needed. But the
MP 4 is no more the end of segment 3 - 4 but the end of segment 3+2000 -
3+3000. This information must be entered to the system and it is done by
optional MP attributes:

- end_mp - end MP
- end_off - end offset

In this case original MP on km 4 will have these attributes:

```sh
start_mp:  4
start_off: 0
end_mp:    3
end_off:   3000
```

Because each MP can keep 2 values (start, end) it is called 'double'
referenced LRS.

To avoid potential confusion, MP values are limited to integers only. It
would be ambiguous to have for example segments 3.500 - 3.500+200 and
3.600 - 3.600+200. The position 3+650 would fall into 2 segments,
correct would be 3.600+50. That means, that MP must be the beginning of
antonomous segment and all parts which becomes longer then before must
be referenced from the last not changed MP.

The MP *start_mp* and *end_mp* columns must be decimal, but
*v.lrs.create* takes only the decimal part, and adds its value to offset
and prints a warning.

It is highly recommended to work with polylines instead of segmented
vector lines. The command *v.build.polylines* creates this map
structure.

### LRS table structure

|               |                  |                                                                                                                                                 |
|---------------|------------------|-------------------------------------------------------------------------------------------------------------------------------------------------|
| **Attribute** | **Type**         | **Description**                                                                                                                                 |
| rsid          | integer          | reference segment ID, unique in the table                                                                                                       |
| lcat          | integer          | category of the line in the LRS map                                                                                                             |
| lid           | integer          | route ID (LID)                                                                                                                                  |
| start_map     | double precision | distance measured along the line in LRS map from the beginning of the line to the beginning of the segment (absolute milepost distance)         |
| end_map       | double precision | distance measured along the line in LRS map from the beginning of the line to the end of the segment (absolute distance of subsequent milepost) |
| start_mp      | double precision | milepost number assigned to the start of the segment                                                                                            |
| start_off     | double precision | distance from start_mp to the start of the segment measured along the physical object                                                           |
| end_mp        | double precision | milepost number assigned to the end of the segment                                                                                              |
| end_off       | double precision | distance from end_mp to end of the segment measured along the physical object                                                                   |
| end_type      | integer          | 1: the same as specified for from\_ ; 2: calculated from map along the line from previous MP; 3: defined by user                                |

### Available commands

- [v.lrs.create](v.lrs.create.md) to create a linear referencing system,
- [v.lrs.label](v.lrs.label.md) to create stationing on the LRS,
- [v.lrs.segment](v.lrs.segment.md) to create points/segments on LRS,
  and
- [v.lrs.where](v.lrs.where.md) to find line id and real km+offset for
  given points in vector map using linear referencing system.

### Input lines for v.lrs.segment and v.lrs.label

*v.lrs.create* joins all connected lines of the same line ID into one
line, the LRS library and other modules using LRS expect this!
LR_get_nearest_offset in the LRS library checks duplicate segments only
by line_cat and map_offset, not by coordinates in map.

### Duplicate positions

It can happen that one offset appears on 2 different lines:

```sh
------1-------     --------2------
+0.0            +1.0              +2.0
```

In this case, the module gives error because the position results in 2
points.

It can be also intended, for example a part of the road is shared with
another one, but MP are used only for one:

```sh
 + road1/km15         + road1/km22
  \                  /
   \ road1/km17     / road1/km20
    +--------------+
   / road2/km52     \ road2/km52
  /                  \
 + road2/km50         + road2/km54
```

## NOTES

Explanations of selected options:

- llayer: vector layer in line map (usually 1; see
  [vectorintro](vectorintro.md) for "layer" concept)
- player: vector layer in point map (usually 1; see
  [vectorintro](vectorintro.md) for "layer" concept)
- rsdriver: Driver name for LRS table - DBMI SQL driver (dbf, pg, mysql,
  sqlite, etc)
- rsdatabase: Database name for LRS table - DBMI SQL database name
  (e.g., "lrsdb")
- rstable: Name of the LRS table - DBMI SQL table name (e.g.,
  "streamslrs")

## SEE ALSO

*R. Blazek, 2004, [Introducing the Linear Reference System in
GRASS](https://foss4g.asia/2004/Full-Paper_PDF/Introducing-the-Linear-Reference-System-in-GRASS.pdf),
Bangkok, GRASS User Conf. Proc.*  
*R. Blazek, 2005, [Introducing the Linear Reference System in
GRASS](https://web.archive.org/web/20240814152234/http://creativecity.gscc.osaka-cu.ac.jp/IJG/article/download/320/321),
International Journal of Geoinformatics, Vol. 1(3), pp. 95-100*  

*[v.build.polylines](v.build.polylines.md),
[v.lrs.create](v.lrs.create.md), [v.lrs.segment](v.lrs.segment.md),
[v.lrs.where](v.lrs.where.md), [v.lrs.label](v.lrs.label.md)*

## AUTHORS

Radim Blazek, ITC-irst/MPA Solutions Trento  
Documentation update (based on above journal article and available
fragments): Markus Neteler
