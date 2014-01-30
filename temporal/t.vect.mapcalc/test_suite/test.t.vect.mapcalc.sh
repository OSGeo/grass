#!/usr/bin/sh

g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 -p

export GRASS_OVERWRITE=1

# Test for temporal algebra in LatLon location.
rm /tmp/vinput1_point_test.txt
rm /tmp/vinput2_point_test.txt
rm /tmp/vinput3_point_test.txt
rm /tmp/vinput4_point_test.txt
rm /tmp/vinput1_area_test.txt
rm /tmp/vinput2_area_test.txt
rm /tmp/vinput3_area_test.txt
rm /tmp/vinput4_area_test.txt

# Create random area test maps.
for i in {1..60}
  do
    if [[ "$i" -le 20 ]]; then
      echo vtestpoint1_$i >> /tmp/vinput1_point_test.txt
      echo vtestarea1_$i  >> /tmp/vinput1_area_test.txt
      v.random --o -z output=vtestpoint1_$i n=3 seed=$i+1
      v.voronoi --o input=vtestpoint1_$i output=vtestarea1_$i
    elif [ "$i" -gt 20 ] && [ "$i" -le 40 ]; then
      echo vtestpoint2_$i >> /tmp/vinput2_point_test.txt
      echo vtestarea2_$i  >> /tmp/vinput2_area_test.txt
      v.random --o -z output=vtestpoint2_$i n=3 seed=$i+1
      v.voronoi --o input=vtestpoint2_$i output=vtestarea2_$i
    else
      echo vtestpoint3_$i >> /tmp/vinput3_point_test.txt
      echo vtestpoint4_$i >> /tmp/vinput4_point_test.txt
      echo vtestarea3_$i  >> /tmp/vinput3_area_test.txt
      echo vtestarea4_$i  >> /tmp/vinput4_area_test.txt
      v.random --o -z output=vtestpoint3_$i n=3 seed=$i+1
      v.voronoi --o input=vtestpoint3_$i output=vtestarea3_$i
      v.random --o -z output=vtestpoint4_$i n=3 seed=$i+1
      v.voronoi --o input=vtestpoint4_$i output=vtestarea4_$i
    fi
  done

# Create STVDS and register test maps.
t.create output=A1 type=stvds title="Area test dataset" descr="Area test dataset"
t.create output=A2 type=stvds title="Area test dataset" descr="Area test dataset"
t.create output=A3 type=stvds title="Area test dataset" descr="Area test dataset"
t.create output=A4 type=stvds title="Area test dataset" descr="Area test dataset"
t.create output=P1 type=stvds title="Point test dataset" descr="Point test dataset"
t.create output=P2 type=stvds title="Point test dataset" descr="Point test dataset"
t.create output=P3 type=stvds title="Point test dataset" descr="Point test dataset"
t.create output=P4 type=stvds title="Point test dataset" descr="Point test dataset"

t.register -i type=vect input=A1 file=/tmp/vinput1_area_test.txt increment="1 days" start="2013-01-01"
t.register -i type=vect input=A2 file=/tmp/vinput2_area_test.txt increment="1 days" start="2013-01-10"
t.register -i type=vect input=A3 file=/tmp/vinput3_area_test.txt increment="3 days" start="2013-01-01"
t.register -i type=vect input=A4 file=/tmp/vinput4_area_test.txt increment="3 days" start="2013-01-10"
t.register -i type=vect input=P1 file=/tmp/vinput1_point_test.txt increment="1 days" start="2013-01-01"
t.register -i type=vect input=P2 file=/tmp/vinput2_point_test.txt increment="1 days" start="2013-01-10"
t.register -i type=vect input=P3 file=/tmp/vinput3_point_test.txt increment="3 days" start="2013-01-01"
t.register -i type=vect input=P4 file=/tmp/vinput4_point_test.txt increment="3 days" start="2013-01-10"

# Test different options.
t.vect.mapcalc expression='B1 = A1 & A2' basename="bmap1"
t.vect.list input=B1 column=name,start_time,end_time
t.vect.mapcalc expression='B2 = A1 {equal|during,+&} A3' basename="bmap2"
t.vect.list input=B2 column=name,start_time,end_time
t.vect.mapcalc expression='B3 = buff_p(P1,10)' basename="bmap3"
t.vect.list input=B3 column=name,start_time,end_time
t.vect.mapcalc expression='B4 = buff_p(P2,30) {equal|during,|^} A4' basename="bmap4"
t.vect.list input=B4 column=name,start_time,end_time
t.vect.mapcalc expression='B5 = if(td(A1) == 1 || start_date() >= "2010-01-10", A2)' basename="bmap5"
t.vect.list input=B5 column=name,start_time,end_time
t.vect.mapcalc expression='B6 = buff_p(P2,30) {equal|during|started,&^} buff_p(P3,30)' basename="bmap6"
t.vect.list input=B6 column=name,start_time,end_time
t.vect.mapcalc expression='B7 = buff_p(P2,30) {starts,&^} buff_p(P3,30)' basename="bmap7"
t.vect.list input=B8 column=name,start_time,end_time
t.vect.mapcalc expression='B8 = buff_p(P2,30) {starts,|^} buff_p(P3,30)' basename="bmap8"
t.vect.list input=B8 column=name,start_time,end_time
