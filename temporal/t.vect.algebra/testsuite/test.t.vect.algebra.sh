#!/usr/bin/sh

g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 -p

export GRASS_OVERWRITE=1

# Test for temporal algebra in LatLon location.
if test -f vinput1_point_test ; then
    rm vinput1_point_test.txt
fi
if test -f vinput1_point_test ; then
    rm vinput2_point_test.txt
fi
if test -f vinput1_point_test ; then
    rm vinput3_point_test.txt
fi
if test -f vinput1_point_test ; then
    rm vinput4_point_test.txt
fi

if test -f vinput1_point_test ; then
    rm vinput1_area_test.txt
fi
if test -f vinput1_point_test ; then
    rm vinput2_area_test.txt
fi
if test -f vinput1_point_test ; then
    rm vinput3_area_test.txt
fi
if test -f vinput1_point_test ; then
    rm vinput4_area_test.txt
fi

LIST="1 2 3 4 5 6 7 8 9 0 10 11 12 13 14 15 16 17 18 19 20"

# Create random area test maps.
for i in ${LIST}
  do
    if test "$i" -le 10 ; then
      echo vtestpoint1_$i >> vinput1_point_test.txt
      echo vtestarea1_$i  >> vinput1_area_test.txt
      v.random --o -z output=vtestpoint1_$i n=3 seed=$i+1
      v.voronoi --o input=vtestpoint1_$i output=vtestarea1_$i
    elif test "$i" -gt 10  && test "$i" -le 15 ; then
      echo vtestpoint2_$i >> vinput2_point_test.txt
      echo vtestarea2_$i  >> vinput2_area_test.txt
      v.random --o -z output=vtestpoint2_$i n=3 seed=$i+1
      v.voronoi --o input=vtestpoint2_$i output=vtestarea2_$i
    else
      echo vtestpoint3_$i >> vinput3_point_test.txt
      echo vtestpoint4_$i >> vinput4_point_test.txt
      echo vtestarea3_$i  >> vinput3_area_test.txt
      echo vtestarea4_$i  >> vinput4_area_test.txt
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

t.register -i type=vector input=A1 file=vinput1_area_test.txt increment="1 days" start="2013-01-01"
t.register -i type=vector input=A2 file=vinput2_area_test.txt increment="1 days" start="2013-01-10"
t.register -i type=vector input=A3 file=vinput3_area_test.txt increment="3 days" start="2013-01-01"
t.register -i type=vector input=A4 file=vinput4_area_test.txt increment="3 days" start="2013-01-10"
t.register -i type=vector input=P1 file=vinput1_point_test.txt increment="1 days" start="2013-01-01"
t.register -i type=vector input=P2 file=vinput2_point_test.txt increment="1 days" start="2013-01-10"
t.register -i type=vector input=P3 file=vinput3_point_test.txt increment="3 days" start="2013-01-01"
t.register -i type=vector input=P4 file=vinput4_point_test.txt increment="3 days" start="2013-01-10"

# Test different options.
t.vect.algebra expression='B1 = A1 & A2' basename="bmap1"
t.vect.list input=B1 column=name,start_time,end_time
t.vect.algebra expression='B2 = A1 {+,equal|during,i} A3' basename="bmap2"
t.vect.list input=B2 column=name,start_time,end_time
t.vect.algebra expression='B3 = buff_p(P1,10)' basename="bmap3"
t.vect.list input=B3 column=name,start_time,end_time
t.vect.algebra expression='B4 = buff_p(P2,30) {|,equal|during} A4' basename="bmap4"
t.vect.list input=B4 column=name,start_time,end_time
t.vect.algebra expression='B5 = if(td(A1) == 1 || start_date(A1) >= "2010-01-10", A2)' basename="bmap5"
t.vect.list input=B5 column=name,start_time,end_time
t.vect.algebra expression='B6 = buff_p(P2,30) {&,equal|during|started,d} buff_p(P3,30)' basename="bmap6"
t.vect.list input=B6 column=name,start_time,end_time
t.vect.algebra expression='B7 = buff_p(P2,30) {&,starts,d} buff_p(P3,30)' basename="bmap7"
t.vect.list input=B8 column=name,start_time,end_time
t.vect.algebra expression='B8 = buff_p(P2,30) {|,starts,d} buff_p(P3,30)' basename="bmap8"
t.vect.list input=B8 column=name,start_time,end_time
