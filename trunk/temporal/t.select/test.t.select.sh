#!/usr/bin/sh

# Test for temporal algebra in LatLon location.
n1=`g.tempfile pid=1 -d`
n2=`g.tempfile pid=2 -d`
n3=`g.tempfile pid=3 -d`
n4=`g.tempfile pid=4 -d`

export GRASS_OVERWRITE=1

# Create random points test maps.
for i in {1..60}
  do
    if [ "$i" -le 20 ]; then
      echo testpoint1_$i >> ${n1}
      v.random  -z output=testpoint1_$i npoints=3 seed=$i
    elif [ "$i" -gt 20 ] && [ "$i" -le 40 ]; then
      echo testpoint2_$i >>  ${n2}
      v.random  -z output=testpoint2_$i npoints=3 seed=$i
    else
      echo testpoint3_$i >>  ${n3}
      echo testpoint4_$i >>  ${n4}
      v.random  -z output=testpoint3_$i npoints=3 seed=$i
      v.random  -z output=testpoint4_$i npoints=3 seed=$i
    fi
  done

# Create STVDS and register test maps.
t.create  output=test1 type=stvds title="test dataset" descr="test dataset"
t.create  output=test2 type=stvds title="test dataset" descr="test dataset"
t.create  output=test3 type=stvds title="test dataset" descr="test dataset"
t.create  output=test4 type=stvds title="test dataset" descr="test dataset"


t.register -i  type=vector input=test1 file=${n1} increment="1 days" start="2013-01-01"
t.register -i  type=vector input=test2 file=${n2} increment="1 days" start="2013-01-10"
t.register -i  type=vector input=test3 file=${n3} increment="3 days" start="2013-01-01"
t.register -i  type=vector input=test4 file=${n4} increment="3 days" start="2013-01-10"

t.info type=stvds input=test1
t.info type=stvds input=test2
t.info type=stvds input=test3
t.info type=stvds input=test4

# Test different options.
t.select  expression="A = test1 : test2" type="stvds"
t.info type=stvds input=A
t.select  expression="A = test1 !: test2" type="stvds"
t.info type=stvds input=A
t.select  expression="A = test2 {equal,:} test1" type="stvds"
t.info type=stvds input=A
t.select  expression='A = test1 : buff_t(test2, "1 days")' type="stvds"
t.info type=stvds input=A
t.select  expression='A = test1 {during,:} buff_t(test2, "1 days")' type="stvds"
t.info type=stvds input=A
t.select  expression='A = tsnap(test1 {during,:} buff_t(test2, "1 days"))' type="stvds"
t.info type=stvds input=A
t.select  expression='A = if(td(test1) == 1, test2)' type="stvds"
t.info type=stvds input=A
t.select  expression='A = if(td(test1) == 2, test2)' type="stvds"
t.info type=stvds input=A
t.select  expression='A = if(start_year() == 2013, test2)' type="stvds"
t.info type=stvds input=A
t.select  expression='A = if(start_year() == 2013 && start_day() < 20, test2)' type="stvds"
t.info type=stvds input=A
t.select  expression='A = if(start_year() == 2013 && start_day() < 20 && start_day() > 15, test2)' type="stvds"
t.info type=stvds input=A
t.select  expression='A = if(start_year() == 2013 || start_day() < 20 && start_day() > 15, test2)' type="stvds"
t.info type=stvds input=A
t.select  expression='A = if(td(test1) == 1 || start_day() < 20 && start_day() > 15, test2)' type="stvds"
t.info type=stvds input=A
t.select  expression='A = if(td(test1) == 1 || start_day() < 20, test2)' type="stvds"
t.info type=stvds input=A
t.select  expression='A = if(td(test1) == 1, if(start_day() > 15, test2))' type="stvds"
t.vect.list input=A column=name,start_time
t.select  expression='A = if(start_day() > 15, test2, test1)' type="stvds"
t.vect.list input=A column=name,start_time
t.select  expression='A = if({during}, start_day() > 15, test2, test1)' type="stvds"
t.vect.list input=A column=name,start_time
t.select  expression='A = if({during}, td(test1) == 1, test2, test1)' type="stvds"
t.vect.list input=A column=name,start_time
t.select  expression='A = if(test1 {#} test2 == 1, test1)' type="stvds"
t.vect.list input=A column=name,start_time
t.select  expression='A = if(test3 {contains,#} test2 > 1, test3)' type="stvds"
t.vect.list input=A column=name,start_time
t.select  expression='A = if({equal|during}, test3 {contains,#} test2 > 1, test3, test1)' type="stvds"
t.vect.list input=A column=name,start_time
t.select  expression="A = test1 {during,:} test3" type="stvds"
t.vect.list input=A column=name,start_time,end_time
t.select  expression="A = test1 {during,:} test4" type="stvds"
t.vect.list input=A column=name,start_time,end_time
t.select  expression="A = test1 {starts,:} test4" type="stvds"
t.vect.list input=A column=name,start_time,end_time
t.select  expression="A = test1 {finishes,:} test4" type="stvds"
t.vect.list input=A column=name,start_time,end_time
t.select  expression="A = test4 {finished,:} test1" type="stvds"
t.vect.list input=A column=name,start_time,end_time
t.select  expression="A = test4 {started,:} test1" type="stvds"
t.vect.list input=A column=name,start_time,end_time

t.remove -rf type=stvds input=test1,test2,test3,test4,A
