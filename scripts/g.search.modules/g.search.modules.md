## DESCRIPTION

*g.search.module* searches for given keyword in GRASS GIS modules name,
description, keywords and optionally manpages, too. Also installed
addons are considered in the search.

## NOTES

Multiple keywords may be specified, *g.search.modules* will search for
all of them.

## EXAMPLES

Search all modules, where keywords *buffer* OR *clip* can be found:

```sh
g.search.modules keyword=buffer,clip

r.circle
    keywords: raster,buffer,geometry,circle
    description: Creates a raster map containing concentric rings around a
                 given point.

r.buffer.lowmem
    keywords: raster,buffer
    description: Creates a raster map showing buffer zones surrounding cells
                 that contain non-NULL category values. This is the low-
                 memory alternative to the classic r.buffer module.

r.buffer
    keywords: raster,buffer
    description: Creates a raster map showing buffer zones surrounding cells
                 that contain non-NULL category values.
```

Search all modules, where keywords *overlay* AND *clip* can be found
with some fancy terminal output (not shown here):

```sh
g.search.modules keyword=clip,overlay -a -c

v.clip
    keywords: vector,clip,area
    description: Extracts features of input map which overlay features
                 of clip map.

v.overlay
    keywords: vector,geometry,spatial
              query,clip,difference,intersection,union
    description: Overlays two vector maps offering clip, intersection,
                 difference, symmetrical difference, union operators.
```

Search in manual pages as well:

```sh
g.search.modules -m keyword=kapri

db.execute
    keywords: database,attribute table,SQL
    description: Executes any SQL statement. For SELECT statements use
                 'db.select'.

db.select
    keywords: database,attribute table,SQL
    description: Selects data from attribute table. Performs SQL query
                 statement(s).
```

## SEE ALSO

*[g.manual](g.manual.md)*

## AUTHORS

Jachym Cepicky, OpenGeoLabs s.r.o., Czech Republic: original author  
Anika Bettge, mundialis, Germany: addon search added
