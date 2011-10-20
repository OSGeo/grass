# This is an example script how to register imported ECA&D data for Europe
# in the temporal database of grass setting valid time interval 

import grass.script as grass

# You need to download the european climate date from the ECA&D server http://eca.knmi.nl/ as netCDF time series
# We only use the data from 1995 - 2010
# First import the ECA&D data into GRASS GIS 
input = "tg_0.25deg_reg_1995-2010_v4.0.nc"
output = "temp_mean"
grass.run_command("r.in.gdal", flags="o", input=input, output=output, overwrite=True)
input = "tn_0.25deg_reg_1995-2010_v4.0.nc"
output = "temp_min"
grass.run_command("r.in.gdal", flags="o", input=input, output=output, overwrite=True)
input = "tx_0.25deg_reg_1995-2010_v4.0.nc"
output = "temp_max"
grass.run_command("r.in.gdal", flags="o", input=input, output=output, overwrite=True)
input = "rr_0.25deg_reg_1995-2010_v4.0.nc"
output = "precip"
grass.run_command("r.in.gdal", flags="o", input=input, output=output, overwrite=True)


# This should be the number of maps to register
num_maps = 5844

# Daily mean temperatue

dataset = "temp_mean_1995_2010_daily"

grass.run_command("t.create", type="strds", output=dataset,\
                  semantic="continuous", temporal="absolute", \
		  title="European mean temperature 1995-2010", \
		  description="The european daily mean temperature 1995 - 2010 from ECA&D ", \
		  overwrite=True)

name = "temp_mean."
filename = grass.tempfile()
file = open(filename, "w")
for i in range(num_maps):
    inc = i + 1
    map_name = name + str(inc)
    string = map_name + "\n"
    file.write(string)

file.close()

grass.run_command("tr.register", flags="i", input=dataset, file=filename, start="1995-01-01", increment="1 days", overwrite=True)

# Daily min temperatue

dataset = "temp_min_1995_2010_daily"

grass.run_command("t.create", type="strds", output=dataset,\
                  semantic="continuous", temporal="absolute", \
		  title="European min temperature 1995-2010", \
		  description="The european daily min temperature 1995 - 2010 from ECA&D ", \
		  overwrite=True)

name = "temp_min."
filename = grass.tempfile()
file = open(filename, "w")
for i in range(num_maps):
    inc = i + 1
    map_name = name + str(inc)
    string = map_name + "\n"
    file.write(string)

file.close()

grass.run_command("tr.register", flags="i", input=dataset, file=filename, start="1995-01-01", increment="1 days", overwrite=True)

# Daily max temperatue

dataset = "temp_max_1995_2010_daily"

grass.run_command("t.create", type="strds", output=dataset,\
                  semantic="continuous", temporal="absolute", \
		  title="European max temperature 1995-2010", \
		  description="The european daily max temperature 1995 - 2010 from ECA&D ", \
		  overwrite=True)

name = "temp_max."
filename = grass.tempfile()
file = open(filename, "w")
for i in range(num_maps):
    inc = i + 1
    map_name = name + str(inc)
    string = map_name + "\n"
    file.write(string)

file.close()

grass.run_command("tr.register", flags="i", input=dataset, file=filename, start="1995-01-01", increment="1 days", overwrite=True)

# Daily precipitation

dataset = "precipitation_1995_2010_daily"

grass.run_command("t.create", type="strds", output=dataset,\
                  semantic="event", temporal="absolute", \
		  title="European precipitation 1995-2010", \
		  description="The european daily precipitation 1995 - 2010 from ECA&D ", \
		  overwrite=True)

name = "precip."
filename = grass.tempfile()
file = open(filename, "w")
for i in range(num_maps):
    inc = i + 1
    map_name = name + str(inc)
    string = map_name + "\n"
    file.write(string)

file.close()

grass.run_command("tr.register", flags="i", input=dataset, file=filename, start="1995-01-01", increment="1 days", overwrite=True)

# Now aggregate the data

grass.run_command("g.region", rast="precip.1", flags="p")

input = "temp_mean_1995_2010_daily"
output = "temp_mean_1995_2010_monthly"
basename = "temp_mean_month"
grass.run_command("tr.aggregate", input=input, method="average", output=output, base=basename, granularity="1 months", overwrite=True)
output = "temp_mean_1995_2010_three_monthly"
basename = "temp_mean_three_month"
grass.run_command("tr.aggregate", input=input, method="average", output=output, base=basename, granularity="3 months", overwrite=True)

input = "temp_min_1995_2010_daily"
output = "temp_min_1995_2010_monthly"
basename = "temp_min_month"
grass.run_command("tr.aggregate", input=input, method="minimum", output=output, base=basename, granularity="1 months", overwrite=True)
output = "temp_min_1995_2010_three_monthly"
basename = "temp_min_three_month"
grass.run_command("tr.aggregate", input=input, method="minimum", output=output, base=basename, granularity="3 months", overwrite=True)

input = "temp_max_1995_2010_daily"
output = "temp_max_1995_2010_monthly"
basename = "temp_max_month"
grass.run_command("tr.aggregate", input=input, method="maximum", output=output, base=basename, granularity="1 months", overwrite=True)
output = "temp_max_1995_2010_three_monthly"
basename = "temp_max_three_month"
grass.run_command("tr.aggregate", input=input, method="maximum", output=output, base=basename, granularity="3 months", overwrite=True)

input = "precipitation_1995_2010_daily"
output = "precipitation_1995_2010_monthly"
basename = "precip_month"
grass.run_command("tr.aggregate", input=input, method="sum", output=output, base=basename, granularity="1 months", overwrite=True)
output = "precipitation_1995_2010_three_monthly"
basename = "precip_three_month"
grass.run_command("tr.aggregate", input=input, method="sum", output=output, base=basename, granularity="3 months", overwrite=True)

