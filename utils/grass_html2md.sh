#!/bin/sh
set -eu

###############################################################################
# Convert recursively all .html files to .md (GitHub flavoured Markdown)
#
# Dependencies:
#    pandoc
#    wget
#
# Author(s):
#    Martin Landa, Markus Neteler
#
# Usage:
#    If you have "pandoc" in PATH, execute for HTML file conversion in
#    current directory and subdirectories:
#      ./utils/grass_html2md.sh
#
# COPYRIGHT: (C) 2024 by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
#
###############################################################################

# define $TMP if not present
if test -z "${TMP}" ; then
  TMP="/tmp"
fi

# TODO: path to LUA file setting to be improved (./utils/pandoc_codeblock.lua)
#wget https://raw.githubusercontent.com/OSGeo/grass/refs/heads/main/utils/pandoc_codeblock.lua -O "${TMP}/pandoc_codeblock.lua"
TMP="utils"

# run recursively: HTML to MD
for f in `find . -name *.html`; do
    echo "${f}"
    cat "${f}" | sed 's#<div class="code"><pre>#<pre><code>#g' | sed 's#</pre></div>#</code></pre>#g' | pandoc \
        --from=html --to=markdown -t gfm --lua-filter "${TMP}/pandoc_codeblock.lua" | \
        sed 's+  $++g' | sed 's+\.html)+\.md)+g' > "${f%%.html}.md"
done
