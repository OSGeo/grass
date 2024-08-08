## DESCRIPTION

*g.extension.rebuild.all* reinstalls and updates all locally installed
GRASS Addons extensions in local GRASS installation. The extensions can
be installed by *[g.extension](g.extension.html)*. List of locally
installed extensions can be printed by `g.extension -a`.

## EXAMPLES

Rebuild locally installed extensions which were built against different
GIS Library (see `g.version -r`)

```
g.extension.rebuild.all
```

Force to rebuild all locally installed extensions

```
g.extension.rebuild.all -f
```

## SEE ALSO

*[g.extension](g.extension.html)*

See also [GRASS Addons](https://grasswiki.osgeo.org/wiki/GRASS_AddOns)
wiki page.

## AUTHOR

Martin Landa, Czech Technical University in Prague, Czech Republic
