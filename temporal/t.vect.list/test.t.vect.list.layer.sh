#!/bin/sh
# This is a test to list vecter maps of a space time vecter dataset

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

v.random --o -z seed=1 output=soil_orig n=20 zmin=0 zmax=100 column=sand
# Adding new layer with categories
v.category input=soil_orig out=soils option=transfer layer=1,1 --o
v.category input=soils out=soil_orig option=transfer layer=1,2 --o
v.category input=soil_orig out=soils option=transfer layer=1,3 --o
v.category input=soils out=soil_orig option=transfer layer=1,4 --o
v.category input=soil_orig out=soils option=transfer layer=1,5 --o
v.category input=soils out=soil_orig option=transfer layer=1,6 --o
g.copy --o vect=soil_orig,soils
# Creating new tables for each layer
db.copy from_table=soil_orig to_table=soils_2
db.copy from_table=soil_orig to_table=soils_3
db.copy from_table=soil_orig to_table=soils_4
db.copy from_table=soil_orig to_table=soils_5
db.copy from_table=soil_orig to_table=soils_6
v.db.addtable map=soils table=soils_2 layer=2 
v.db.addtable map=soils table=soils_3 layer=3 
v.db.addtable map=soils table=soils_4 layer=4 
v.db.addtable map=soils table=soils_5 layer=5 
v.db.addtable map=soils table=soils_6 layer=6 

n1=`g.tempfile pid=1 -d` 
n2=`g.tempfile pid=2 -d`
n3=`g.tempfile pid=3 -d`
n4=`g.tempfile pid=4 -d`
n5=`g.tempfile pid=5 -d`

cat > "${n1}" << EOF
soils:1
soils:2
soils:3
soils:4
soils:5
soils:6
EOF

cat > "${n2}" << EOF
soils:1|2001-01-01
soils:2|2001-02-01
soils:3|2001-03-01
soils:4|2001-04-01
soils:5|2001-05-01
soils:6|2001-06-01
EOF

cat > "${n3}" << EOF
soils:1|2001-01-01|2001-04-01
soils:2|2001-05-01|2001-07-01
soils:3|2001-08-01|2001-10-01
soils:4|2001-11-01|2002-01-01
soils:5|2002-02-01|2002-04-01
soils:6|2002-05-01|2002-07-01
EOF

cat > "${n4}" << EOF
soils:1|2001-01-01|2001-07-01
soils:2|2001-02-01|2001-04-01
soils:3|2001-03-01|2001-04-01
soils:4|2001-04-01|2001-06-01
soils:5|2001-05-01|2001-06-01
soils:6|2001-06-01|2001-07-01
EOF

cat > "${n5}" << EOF
soils:1|2001-01-01|2001-03-11
soils:2|2001-02-01|2001-04-01
soils:3|2001-03-01|2001-06-02
soils:4|2001-04-01|2001-04-01
soils:5|2001-05-01|2001-05-01
soils:6|2001-06-01|2001-07-01
EOF

t.create --o type=stvds temporaltype=absolute output=soils_abs1 title="A test with input files" descr="A test with input files"
t.create --o type=stvds temporaltype=absolute output=soils_abs2 title="A test with input files" descr="A test with input files"
t.create --o type=stvds temporaltype=absolute output=soils_abs3 title="A test with input files" descr="A test with input files"
t.create --o type=stvds temporaltype=absolute output=soils_abs4 title="A test with input files" descr="A test with input files"
t.create --o type=stvds temporaltype=absolute output=soils_abs5 title="A test with input files" descr="A test with input files"

# The first @test
t.register type=vect --o   input=soils_abs1 file="${n1}" start="2001-01-01" increment="1 months"
t.vect.list    separator=" | " method=comma     input=soils_abs1
t.vect.list -h input=soils_abs1
t.vect.list -h separator=" | " method=cols      input=soils_abs1
t.vect.list -h separator=" | " method=delta     input=soils_abs1
t.vect.list -h separator=" | " method=deltagaps input=soils_abs1

t.register type=vect --o -i input=soils_abs2 file="${n2}"
t.vect.list    separator=" | " method=comma     input=soils_abs2
t.vect.list -h input=soils_abs2
t.vect.list -h separator=" | " method=cols      input=soils_abs2
t.vect.list -h separator=" | " method=delta     input=soils_abs2
t.vect.list -h separator=" | " method=deltagaps input=soils_abs2

t.register type=vect --o -i input=soils_abs3 file="${n3}"
t.vect.list    separator=" | " method=comma     input=soils_abs3
t.vect.list -h separator=" | " method=delta     input=soils_abs3
t.vect.list -h separator=" | " method=deltagaps input=soils_abs3

t.register type=vect --o -i input=soils_abs4 file="${n4}"
t.vect.list    separator=" | " method=comma     input=soils_abs4
t.vect.list -h separator=" | " method=delta     input=soils_abs4
t.vect.list -h separator=" | " method=deltagaps input=soils_abs4

t.register type=vect --o -i input=soils_abs5 file="${n5}"
t.vect.list    separator=" | " method=comma     input=soils_abs5
t.vect.list -h input=soils_abs5
t.vect.list -h separator=" | " method=cols      input=soils_abs5
t.vect.list -h separator=" | " method=delta     input=soils_abs5
t.vect.list -h separator=" | " method=deltagaps input=soils_abs5

t.unregister type=vect maps=soils:1,soils:2,soils:3,soils:4,soils:5,soils:6
t.remove type=stvds input=soils_abs1,soils_abs2,soils_abs3,soils_abs4,soils_abs5

g.remove vect=soil_orig,soils
