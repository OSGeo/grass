# This is a simple timestamp check, creation and removal test

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster data with r.mapcalc.
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

# Lets generate a test map
v.random --o seed=1 npoints=20 output=map

# The first @test uses several different absolute datum formats
v.timestamp map=map date=none
v.timestamp map=map
v.timestamp map=map layer=1 date="2003"
v.timestamp map=map layer=1
v.timestamp map=map layer=2 date="Jul 2003"
v.timestamp map=map layer=2
v.timestamp map=map date="14 Jul 2003"
v.timestamp map=map
v.timestamp map=map date="14 Jul 2003 10"
v.timestamp map=map
v.timestamp map=map layer=3 date="14 Jul 2003 10:30 +0700"
v.timestamp map=map layer=3
v.timestamp map=map layer=4 date="14 Jul 2003 10:30:25"
v.timestamp map=map layer=4
v.timestamp map=map layer=2 date="14 Jul 2003 10:30:25 +0700 / 15 Jul 2003 11:35:12 +0700"
v.timestamp map=map layer=2
v.timestamp map=map layer=3 date="14 Jul 2003 10:30:25 +0700 / 15 Jul 2003"
v.timestamp map=map layer=3
v.info map=map
v.timestamp map=map date=none
v.timestamp map=map layer=2 date=none
v.timestamp map=map layer=3 date=none
v.timestamp map=map layer=4 date=none
v.timestamp map=map
v.timestamp map=map layer=2
v.timestamp map=map layer=3
v.timestamp map=map layer=4

# The second @test uses several different relative datum formats
v.timestamp map=map date=none
v.timestamp map=map
v.timestamp map=map date="2 years"
v.timestamp map=map
v.timestamp map=map date="2 years 3 months"
v.timestamp map=map
v.timestamp map=map layer=1 date="5 days"
v.timestamp map=map layer=1
v.timestamp map=map layer=2 date="3 hours"
v.timestamp map=map layer=2
v.timestamp map=map layer=1 date="5 minutes 30 seconds"
v.timestamp map=map layer=1
v.timestamp map=map layer=2 date="2 years 2 months / 5 years 8 months"
v.timestamp map=map layer=2
v.info map=map
v.timestamp map=map date=none
v.timestamp map=map layer=2 date=none
v.timestamp map=map layer=3 date=none
v.timestamp map=map
v.timestamp map=map layer=2
v.timestamp map=map layer=3

# The third @test to check @failure with wrong time stamps
g.message message="Now checking for expected failures due to wrong time stamps..."
v.timestamp map=map date="2 years 3 months 8 days"
v.timestamp map=map date="1 month 5 days"
v.timestamp map=map date="July 2003"
v.timestamp map=map date="14 Jul 2003 +0700"
