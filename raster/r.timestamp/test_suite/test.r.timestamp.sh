# This is a simple timestamp check, creation and removal test

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster data with r.mapcalc.
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

# Lets generate a test map
r.mapcalc --o expr="map = 1"

# The first @test uses several different absolute datum formats
r.timestamp map=map date=none
r.timestamp map=map
r.timestamp map=map date="2003"
r.timestamp map=map
r.timestamp map=map date="Jul 2003"
r.timestamp map=map
r.timestamp map=map date="14 Jul 2003"
r.timestamp map=map
r.timestamp map=map date="14 Jul 2003 10"
r.timestamp map=map
r.timestamp map=map date="14 Jul 2003 10:30 +0700"
r.timestamp map=map
r.timestamp map=map date="14 Jul 2003 10:30:25"
r.timestamp map=map
r.timestamp map=map date="14 Jul 2003 10:30:25 +0700 / 15 Jul 2003 11:35:12 +0700"
r.timestamp map=map
r.timestamp map=map date="14 Jul 2003 10:30:25 +0700 / 15 Jul 2003"
r.timestamp map=map
r.timestamp map=map date=none
r.timestamp map=map

# The second @test uses several different relative datum formats
r.timestamp map=map date=none
r.timestamp map=map
r.timestamp map=map date="2 years"
r.timestamp map=map
r.timestamp map=map date="2 years 3 months"
r.timestamp map=map
r.timestamp map=map date="5 days"
r.timestamp map=map
r.timestamp map=map date="3 hours"
r.timestamp map=map
r.timestamp map=map date="5 minutes 30 seconds"
r.timestamp map=map
r.timestamp map=map date="2 years 2 months / 5 years 8 months"
r.timestamp map=map
r.timestamp map=map date=none

# The third @test to check @failure with wrong time stamps
g.message message="Now checking for expected failures due to wrong time stamps..."
r.timestamp map=map date="2 years 3 months 8 days"
r.timestamp map=map date="1 month 5 days"
r.timestamp map=map date="July 2003"
r.timestamp map=map date="14 Jul 2003 +0700"
