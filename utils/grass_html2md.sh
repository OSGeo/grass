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

# path to LUA file (./utils/pandoc_codeblock.lua)
UTILSPATH="utils"

# run recursively: HTML to MD
for f in $(find . -name *.html); do
    echo "${f}"

    # HTML: Process the tmp file to selectively replace .html with .md only in relative URLs
    sed -E '
  # Step 1: Preserve https or http URLs with .html
  s|(<a href="https?://[^"]+\.html)">|\1_KEEPHTML">|g;
  # Step 2: Replace .html with .md for other links
  s|(<a href=")([^"]+)\.html">|\1\2.md">|g;
  # Step 3: Restore preserved https or http URLs
  s|_KEEPHTML">|">|g;
' "${f%%.html}.html" > "${f%%.html}_tmp.html"

    cat "${f%%.html}_tmp.html" | sed 's#<div class="code"><pre>#<pre><code>#g' | sed 's#</pre></div>#</code></pre>#g' | pandoc \
        --from=html --to=markdown -t gfm --lua-filter "${UTILSPATH}/pandoc_codeblock.lua" > "${f%%.html}.md"

    rm -f "${f%%.html}_tmp.html"

done
