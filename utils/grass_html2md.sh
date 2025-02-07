#!/bin/bash
set -eu

###############################################################################
# Convert recursively all .html files to .md (GitHub flavoured Markdown)
#
# Dependencies:
#    pandoc
#    wget
#
# Author(s):
#    Martin Landa, Markus Neteler, Corey White
#
# Usage:
#    If you have "pandoc" in PATH, execute for HTML file conversion in
#    current directory and subdirectories:
#      ./utils/grass_html2md.sh
#
# COPYRIGHT: (C) 2024-2025 by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
#
###############################################################################

# cleanup at user break
cleanup()
{
   rm -f "${f%%.html}_tmp.html"
}

# what to do in case of user break:
exitprocedure()
{
   echo "User break!"
   cleanup
   exit 1
}
# shell check for user break (signal list: trap -l)
trap "exitprocedure" 2 3 15

# path to LUA file (./utils/pandoc_codeblock.lua)
UTILSPATH="utils"

process_file() {
    local file="$1" # temporary file
    local f="$2" # original file

    cat "$file" | \
        sed 's#<div class="code"><pre>#<pre><code>#g' | \
        sed 's#</pre></div>#</code></pre>#g' | \
        pandoc -f html-native_divs \
            -t gfm+pipe_tables+gfm_auto_identifiers --wrap=auto \
            --lua-filter "${UTILSPATH}/pandoc_codeblock.lua" | \
        sed 's+ \\\$+ \$+g' | sed 's+%20+-+g' > "${f%%.html}.md"

    rm -f "$file"

}

# run recursively: HTML to MD
for f in $(find . -name *.html); do
    echo "${f}"

    # HTML: Process the tmp file to selectively replace .html with .md only in relative URLs
    sed -E '
  # Step 1: Preserve http/https links with .html (and optional anchors)
  s|(<a href="https?://[^"]+\.html)(#[^"]*)?">|\1_KEEPHTML\2">|g;
  # Step 2: Replace .html with .md for local links (with or without anchors)
  s|(<a href=")([^"]+)\.html(#[^"]*)?">|\1\2.md\3">|g;
  # Step 3: Restore preserved http/https links with .html
  s|_KEEPHTML||g;
' "${f%%.html}.html" > "${f%%.html}_tmp.html"

    process_file "${f%%.html}_tmp.html" ${f%%.html}.html

done
