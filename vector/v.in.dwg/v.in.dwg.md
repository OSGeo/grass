## DESCRIPTION

*v.in.dwg* imports DWG/DXF file into GRASS.

## EXAMPLE

```
v.in.dwg input=map.dwg output=map
```

## NOTES

v.in.dwg requires OpenDWG toolkit. To get this toolkit you must become
at least \"Associate Member\" of OpenDWG Alliance
(http://www.opendesign.com/).

The toolkit, for example `ad27linx.tar`, unpack in a directory (e.g.
/home/usr1/opendwg27) and use the related `configure` options to tell
GRASS about it:

```
   ./configure \
   ... \
   --with-opendwg \
   --with-opendwg-includes=/home/usr1/opendwg27 \
   --with-opendwg-libs=/home/usr1/opendwg27
```

Then you can compile this module.

Not all entity types are supported (warning printed).

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy
