#!/bin/sh
# This is a test to list vecter maps of a space time vecter dataset

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

v.random --o -z output=lidar_1 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=lidar_2 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=lidar_3 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=lidar_4 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=lidar_5 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=lidar_6 n=20 zmin=0 zmax=100 column=height

n1=`g.tempfile pid=1 -d` 
n2=`g.tempfile pid=2 -d`
n3=`g.tempfile pid=3 -d`
n4=`g.tempfile pid=4 -d`
n5=`g.tempfile pid=5 -d`

cat > "${n1}" << EOF
lidar_1
lidar_2
lidar_3
lidar_4
lidar_5
lidar_6
EOF

cat > "${n2}" << EOF
lidar_1|2001-01-01
lidar_2|2001-02-01
lidar_3|2001-03-01
lidar_4|2001-04-01
lidar_5|2001-05-01
lidar_6|2001-06-01
EOF

cat > "${n3}" << EOF
lidar_1|2001-01-01|2001-04-01
lidar_2|2001-05-01|2001-07-01
lidar_3|2001-08-01|2001-10-01
lidar_4|2001-11-01|2002-01-01
lidar_5|2002-02-01|2002-04-01
lidar_6|2002-05-01|2002-07-01
EOF

cat > "${n4}" << EOF
lidar_1|2001-01-01|2001-07-01
lidar_2|2001-02-01|2001-04-01
lidar_3|2001-03-01|2001-04-01
lidar_4|2001-04-01|2001-06-01
lidar_5|2001-05-01|2001-06-01
lidar_6|2001-06-01|2001-07-01
EOF

cat > "${n5}" << EOF
lidar_1|2001-01-01|2001-03-11
lidar_2|2001-02-01|2001-04-01
lidar_3|2001-03-01|2001-06-02
lidar_4|2001-04-01|2001-04-01
lidar_5|2001-05-01|2001-05-01
lidar_6|2001-06-01|2001-07-01
EOF

t.create --o type=stvds temporaltype=absolute output=lidar_abs1 title="A test with input files" descr="A test with input files"
t.create --o type=stvds temporaltype=absolute output=lidar_abs2 title="A test with input files" descr="A test with input files"
t.create --o type=stvds temporaltype=absolute output=lidar_abs3 title="A test with input files" descr="A test with input files"
t.create --o type=stvds temporaltype=absolute output=lidar_abs4 title="A test with input files" descr="A test with input files"
t.create --o type=stvds temporaltype=absolute output=lidar_abs5 title="A test with input files" descr="A test with input files"

# The first @test
t.register --o type=vect  input=lidar_abs1 file="${n1}" start="2001-01-01" increment="1 months"
t.vect.list    separator=" | " method=comma     input=lidar_abs1
t.vect.list -h input=lidar_abs1
t.vect.list -h separator=" | " method=cols      input=lidar_abs1
t.vect.list -h separator=" | " method=delta     input=lidar_abs1
t.vect.list -h separator=" | " method=deltagaps input=lidar_abs1

t.register --o type=vect -i input=lidar_abs2 file="${n2}"
t.vect.list    separator=" | " method=comma     input=lidar_abs2
t.vect.list -h input=lidar_abs2
t.vect.list -h separator=" | " method=cols      input=lidar_abs2
t.vect.list -h separator=" | " method=delta     input=lidar_abs2
t.vect.list -h separator=" | " method=deltagaps input=lidar_abs2

t.register --o type=vect -i input=lidar_abs3 file="${n3}"
t.vect.list    separator=" | " method=comma     input=lidar_abs3
t.vect.list -h separator=" | " method=delta     input=lidar_abs3
t.vect.list -h separator=" | " method=deltagaps input=lidar_abs3

t.register --o type=vect -i input=lidar_abs4 file="${n4}"
t.vect.list    separator=" | " method=comma     input=lidar_abs4
t.vect.list -h separator=" | " method=delta     input=lidar_abs4
t.vect.list -h separator=" | " method=deltagaps input=lidar_abs4

t.register --o type=vect -i input=lidar_abs5 file="${n5}"
t.vect.list    separator=" | " method=comma     input=lidar_abs5
t.vect.list -h input=lidar_abs5
t.vect.list -h separator=" | " method=cols      input=lidar_abs5
t.vect.list -h separator=" | " method=delta     input=lidar_abs5
t.vect.list -h separator=" | " method=deltagaps input=lidar_abs5

#t.unregister type=vect maps=lidar_1,lidar_2,lidar_3,lidar_4,lidar_5,lidar_6
#t.remove type=stvds input=lidar_abs1,lidar_abs2,lidar_abs3,lidar_abs4,lidar_abs5
#g.remove vect=lidar_1,lidar_2,lidar_3,lidar_4,lidar_5,lidar_6
