# This is a test to register and unregister raster maps in
# space time raster dataset.
# The raster maps will be registered in different space time raster
# datasets

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create two space time raster datasets
# with relative and absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

# The first @test
# We create the space time raster datasets and register the raster maps with absolute time interval

t.create --v --o type=strds temporaltype=absolute dataset=precip_abs1 gran="1 senconds" title="A test" descr="A test"
t.create --v --o type=strds temporaltype=absolute dataset=precip_abs2 gran="1 minutes" title="A test" descr="A test"
t.create --v --o type=strds temporaltype=absolute dataset=precip_abs3 gran="1 hours" title="A test" descr="A test"
t.create --v --o type=strds temporaltype=absolute dataset=precip_abs4 gran="1 days" title="A test" descr="A test"
t.create --v --o type=strds temporaltype=absolute dataset=precip_abs5 gran="1 weeks" title="A test" descr="A test"
t.create --v --o type=strds temporaltype=absolute dataset=precip_abs6 gran="1 months" title="A test" descr="A test"
t.create --v --o type=strds temporaltype=absolute dataset=precip_abs7 gran="1 years" title="A test" descr="A test"

tr.register --v -i dataset=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="1 seconds"
t.info type=strds dataset=precip_abs1
t.info -t type=strds dataset=precip_abs1
tr.unregister --v dataset=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.info type=strds dataset=precip_abs1

tr.register --v -i dataset=precip_abs2 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="20 seconds, 5 minutes"
t.info type=strds dataset=precip_abs2
t.info -t type=strds dataset=precip_abs2

tr.register --v -i dataset=precip_abs3 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="8 hours"
t.info type=strds dataset=precip_abs3
t.info -t type=strds dataset=precip_abs3
tr.unregister --v maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.info type=strds dataset=precip_abs3

tr.register dataset=precip_abs4 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="3 days"
t.info type=strds dataset=precip_abs4

tr.register dataset=precip_abs5 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="4 weeks"
t.info type=strds dataset=precip_abs5

tr.register dataset=precip_abs6 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-08-01" increment="2 months"
t.info type=strds dataset=precip_abs6

tr.register dataset=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="20 years, 3 months, 1 days, 4 hours"
t.info type=strds dataset=precip_abs7
t.info -t type=strds dataset=precip_abs7
# Register with different valid time again
tr.register dataset=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
t.info type=strds dataset=precip_abs7
t.info -t type=strds dataset=precip_abs7
# Register with different valid time again creating an interval
tr.register -i dataset=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
t.info type=strds dataset=precip_abs7
t.info -t type=strds dataset=precip_abs7

t.remove --v type=raster dataset=prec_1,prec_2,prec_3
t.remove --v type=strds dataset=precip_abs1,precip_abs2,precip_abs3,precip_abs4,precip_abs5,precip_abs6,precip_abs7
t.remove --v type=raster dataset=prec_4,prec_5,prec_6
