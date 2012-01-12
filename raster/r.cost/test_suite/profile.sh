# NC dataset
g.region rast=elevation -p
r.mapcalc 'one = 1'
CMD="r.cost -k in=one start_points=school_copy output=cost.test --o"

valgrind --tool=callgrind --trace-children=yes $CMD
kcachegrind callgrind.out.1196

