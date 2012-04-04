g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"
# We create several space time raster datasets

# A simple space ime raster datasets creation and unpdate @test with absolute time
t.create --v --o type=strds temporaltype=absolute output=precip_abs1 title="Test" descr="This is the 1 test strds" semantictype=sum
t.register -i input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="1 seconds"

t.info type=strds input=precip_abs1
t.support --v type=strds input=precip_abs1 title="Test support" descr="This is the support test strds" semantictype=mean
t.info type=strds input=precip_abs1

# @test the update functions
r.mapcalc --o expr="prec_1 = rand(0, 55)"
r.mapcalc --o expr="prec_2 = rand(0, 45)"
r.mapcalc --o expr="prec_3 = rand(0, 32)"
r.mapcalc --o expr="prec_4 = rand(0, 51)"
r.mapcalc --o expr="prec_5 = rand(0, 30)"
r.mapcalc --o expr="prec_6 = rand(0, 65)"
t.support --v -m type=strds input=precip_abs1
t.info type=strds input=precip_abs1

# A simple space ime raster datasets creation and update @test with relative time
t.create --v --o type=strds temporaltype=relative output=precip_rel1 title="Test" descr="This is the 1 test strds" semantictype=min
t.info type=strds input=precip_rel1
t.support --v type=strds input=precip_rel1 title="Test support" descr="This is the support test strds" semantictype=max
t.info type=strds input=precip_rel1

t.remove --v type=strds input=precip_abs1,precip_rel1
t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
g.remove rast=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6