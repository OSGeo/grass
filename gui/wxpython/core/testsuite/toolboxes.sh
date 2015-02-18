# Tests generating toolboxes XML

# run make in gui/wxpython before the test
# run test using sh -e

python $GISBASE/gui/wxpython/core/toolboxes.py doctest
python $GISBASE/gui/wxpython/core/toolboxes.py test
