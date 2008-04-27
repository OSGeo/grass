#!/bin/sh

echo "Did you export a fresh .tex file from LyX first?"
echo
rm -rf GEM-Manual
rm -f index.html
latex GEM-Manual.tex
latex GEM-Manual.tex
latex GEM-Manual.tex

latex2html -split 0 -show_section_numbers -image_type gif GEM-Manual.tex

rm -f GEM-Manual.aux
rm -f GEM-Manual.dvi
rm -f GEM-Manual.log
rm -f GEM-Manual.toc

rm -f *~
