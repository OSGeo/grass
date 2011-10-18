# We create several space time raster datasets

# A simple space ime raster datasets creation and unpdate @test with absolute time
t.create --v --o type=strds temporaltype=absolute output=precip_abs1 title="Test" descr="This is the 1 test strds" semantictype=event
t.info type=strds input=precip_abs1
t.support --v type=strds input=precip_abs1 title="Test support" descr="This is the support test strds" semantictype=const
t.info type=strds input=precip_abs1

# A simple space ime raster datasets creation and update @test with relative time
t.create --v --o type=strds temporaltype=relative output=precip_rel1 title="Test" descr="This is the 1 test strds" semantictype=event
t.info type=strds input=precip_rel1
t.support --v type=strds input=precip_rel1 title="Test support" descr="This is the support test strds" semantictype=continuous
t.info type=strds input=precip_rel1

t.remove --v type=strds input=precip_abs1,precip_rel1
