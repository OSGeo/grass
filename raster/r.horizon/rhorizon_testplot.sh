# Horizon angles CCW from East

# Horizon angle output in degree

# NC dataset
DEM=elevation
# test point near high way intersection
coords=636483.54,222176.25

g.region n=223540 s=220820 w=634650 e=638780 res=10 -p

# direction=0 is East, direction=90 is North
r.horizon elev_in=$DEM direction=0 horizon_step=5 bufferzone=200 \
    coordinate=$coords maxdistance=5000 -d output=horizon.csv --o

echo 'set datafile separator ","

set ylabel "Horizon angle"
set xlabel "Angle (CCW from East)"

plot "horizon.csv" using 1:2 with lines' > gnuplot.txt
gnuplot -persist gnuplot.txt
