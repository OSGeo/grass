# This is a simple timestamp check, creation and removal test

# We need to set a specific region in the
# @preprocess step of this test. We generate
# voxel data with r3.mapcalc.
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

# Lets gerenate a test map
r3.mapcalc --o expr="map3d = 1"

# The first @test uses several different absolute datum formats
r3.timestamp map=map3d date=none
r3.timestamp map=map3d 
r3.timestamp map=map3d date="2003"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="Jul 2003"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="14 Jul 2003"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="14 Jul 2003 10"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="14 Jul 2003 10:30 +0700"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="14 Jul 2003 10:30:25"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="14 Jul 2003 10:30:25 +0700 / 15 Jul 2003 11:35:12 +0700"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="14 Jul 2003 10:30:25 +0700 / 15 Jul 2003"
r3.timestamp map=map3d 
r3.timestamp map=map3d date=none
r3.timestamp map=map3d 

# The second @test uses several different relative datum formats
r3.timestamp map=map3d date=none
r3.timestamp map=map3d 
r3.timestamp map=map3d date="2 years"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="2 years 3 months"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="5 days"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="3 hours"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="5 minutes 30 seconds"
r3.timestamp map=map3d 
r3.timestamp map=map3d date="2 years 2 months / 5 years 8 months"
r3.timestamp map=map3d 
r3.timestamp map=map3d date=none

# The third @test to check @failure with wrong time stamps
g.message message="Now checking for expected failures due to wrong time stamps..."
r3.timestamp map=map3d date="2 years 3 months 8 days"
r3.timestamp map=map3d date="1 month 5 days"
r3.timestamp map=map3d date="July 2003"
r3.timestamp map=map3d date="14 Jul 2003 +0700"

