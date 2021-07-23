# Tests generating toolboxes XML

# run make in gui/wxpython before the test
# run test using sh -e

python3 $GISBASE/gui/wxpython/core/toolboxes.py doctest
python3 $GISBASE/gui/wxpython/core/toolboxes.py test
