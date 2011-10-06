# This is an example script how to register imported ECA&D temperature data for Europe
# in the temporal database of grass assigning valid time 

import grass.script as grass

# Create the space time raster dataset with t.create
dataset = "temp_mean_1995_2010_daily"

grass.run_command("t.create", type="strds", dataset=dataset, granularity="1 days", \
                  semantic="continuous", temporal="absolute", \
		  title="European mean temperature 1995-2010", \
		  description="The european daily mean temperature 1995 - 2010 from ECA&D ", \
		  overwrite=True)

name = "temp_mean."
maps=""
for i in range(5844):
    inc = i + 1
    map_name = name + str(inc)
    if i == 0:
        maps += map_name
    else:
        maps += "," + map_name
    
# Register all maps at once
grass.run_command("tr.register", flags="i", dataset=dataset, maps=maps, start="1995-01-01", increment="1 days", overwrite=True)
