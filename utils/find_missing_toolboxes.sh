#!/bin/sh

TOOLBOXES="gui/wxpython/xml/toolboxes.xml"

if [ ! -f "$TOOLBOXES" ]; then
    echo "ERROR: $TOOLBOXES not found. Run from the repository root." >&2
    exit 1
fi

WXGUI_ITEMS="gui/wxpython/xml/wxgui_items.xml"

# Tools accessible in the GUI: direct module-item entries in toolboxes.xml,
# tools wrapped by a wxgui-item dialog, and tools invoked directly via a <command> tag.
registered=$(
    {
        sed -n 's/.*<module-item name="\([^"]*\)".*/\1/p' "$TOOLBOXES"
        sed -n 's/.*<related-module>\([^<]*\)<\/related-module>.*/\1/p' "$WXGUI_ITEMS"
        sed -n 's/.*<command>\([a-z][a-z0-9]*\.[a-z][a-z0-9._]*\).*/\1/p' "$WXGUI_ITEMS"
    } | sort -u
)

# Find tool directories: one level deep anywhere in the tree, name matches the GRASS tool naming convention.
source_tools=$(find . -mindepth 2 -maxdepth 2 -type d -regex '.*/[a-z][a-z0-9]*\.[a-z][a-z0-9._]*' \
    | sed 's|.*/||' | sort -u)

# Tools intentionally absent from the GUI menu.  One name per line so that grep -Fxf can match exactly.
#   d.*  Display tools are called from the map display, not the module menu.
#   g.*  Internal tools that are not meant for the user to use.
excluded_list=$(cat <<'EOF'
d.background
d.barscale
d.colorlist
d.colortable
d.correlate
d.erase
d.extract
d.font
d.fontlist
d.frame
d.geodesic
d.graph
d.grid
d.his
d.histogram
d.info
d.labels
d.legend
d.legend.vect
d.linegraph
d.mon
d.northarrow
d.out.file
d.path
d.polar
d.profile
d.rast
d.rast.arrow
d.rast.leg
d.rast.num
d.redraw
d.rgb
d.rhumbline
d.shade
d.text
d.title
d.to.rast
d.vect
d.vect.chart
d.vect.thematic
d.what.rast
d.what.vect
d.where
g.cairocomp
g.dirseps
g.filename
g.findetc
g.findfile
g.gui
g.html2man
g.message
g.mkfontcap
g.parser
g.pnmcomp
g.ppmtopng
g.search.modules
g.setproj
g.tempfile
g.version
EOF
)

# Remove excluded tools, then remove already-registered tools.
missing=$(
    echo "$source_tools" \
        | grep -Fxvf <(echo "$excluded_list") \
        | grep -Fxvf <(echo "$registered")
)

if [ -z "$missing" ]; then
    echo "All source tools are registered in toolboxes.xml."
    exit 0
fi

count=$(echo "$missing" | wc -l | tr -d ' ')
echo "Tools in source tree but missing from toolboxes.xml ($count):"
echo ""
echo "$missing" | sed 's/^/  /'