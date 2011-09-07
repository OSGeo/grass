import os
from grass.script.temporal import *
import grass.script as grass

###############################################################################

def test_dict_sql_serializer():
    t = dict_sql_serializer()
    t.test()

def test_dataset_identifer():
	for i in range(2):
	    base = raster_base(ident="soil" + str(i) + "@PERMANENT", name="soil" + str(i), mapset="PERMANENT", creator="soeren", temporal_type="absolute", revision=1)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_creator("rene")
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = raster3d_base(ident="soil" + str(i) + "@PERMANENT", name="soil" + str(i), mapset="PERMANENT", temporal_type="absolute", creator="soeren")
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_creator("rene")
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = vector_base(ident="soil" + str(i) + "@PERMANENT", name="soil" + str(i), mapset="PERMANENT", temporal_type="absolute", creator="soeren")
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_creator("rene")
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = strds_base(ident="soil" + str(i) + "@PERMANENT", name="soil" + str(i), mapset="PERMANENT", creator="soeren", semantic_type="event", temporal_type="absolute", revision=1)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_creator("rene")
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = str3ds_base(ident="soil" + str(i) + "@PERMANENT", name="soil" + str(i), mapset="PERMANENT", temporal_type="absolute", semantic_type="event", creator="soeren")
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_creator("rene")
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = stvds_base(ident="soil" + str(i) + "@PERMANENT", name="soil" + str(i), mapset="PERMANENT", temporal_type="absolute", semantic_type="event", creator="soeren")
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_creator("rene")
	    base.update()
	    base.select()
	    base.print_self()
            
def test_absolute_timestamp():
	for i in range(2):
	    base = raster_absolute_time(ident="soil" + str(i) + "@PERMANENT", start_time=datetime(2011,01,01), end_time=datetime(2011,07,01), timezone=1)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_start_time(datetime(2010,01,01))
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = raster3d_absolute_time(ident="soil" + str(i) + "@PERMANENT", start_time=datetime(2011,01,01), end_time=datetime(2011,07,01), timezone=1)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_start_time(datetime(2010,01,01))
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = vector_absolute_time(ident="soil" + str(i) + "@PERMANENT", start_time=datetime(2011,01,01), end_time=datetime(2011,07,01), timezone=1)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_start_time(datetime(2010,01,01))
	    base.update()
	    base.select()
	    base.print_self()
            
	for i in range(2):
	    base = strds_absolute_time(ident="soil" + str(i) + "@PERMANENT", start_time=datetime(2011,01,01), end_time=datetime(2011,07,01), granularity="1 day", timezone=1)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_start_time(datetime(2010,01,01))
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = str3ds_absolute_time(ident="soil" + str(i) + "@PERMANENT", start_time=datetime(2011,01,01), end_time=datetime(2011,07,01), granularity="1 day", timezone=1)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_start_time(datetime(2010,01,01))
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = stvds_absolute_time(ident="soil" + str(i) + "@PERMANENT", start_time=datetime(2011,01,01), end_time=datetime(2011,07,01), granularity="1 day", timezone=1)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_start_time(datetime(2010,01,01))
	    base.update()
	    base.select()
	    base.print_self()
            
def test_spatial_extent():
	for i in range(2):
	    base = raster_spatial_extent(ident="soil" + str(i) + "@PERMANENT", north=100 + i, south=10+i, east=50+i, west=20+i, top=i, bottom=0)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_north(120+i)
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = raster3d_spatial_extent(ident="soil" + str(i) + "@PERMANENT", north=100 + i, south=10+i, east=50+i, west=20+i, top=i, bottom=0)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_north(120+i)
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = vector_spatial_extent(ident="soil" + str(i) + "@PERMANENT", north=100 + i, south=10+i, east=50+i, west=20+i, top=i, bottom=0)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_north(120+i)
	    base.update()
	    base.select()
	    base.print_self()
            
	for i in range(2):
	    base = strds_spatial_extent(ident="soil" + str(i) + "@PERMANENT", north=100 + i, south=10+i, east=50+i, west=20+i, top=i, bottom=0)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_north(120+i)
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = str3ds_spatial_extent(ident="soil" + str(i) + "@PERMANENT", north=100 + i, south=10+i, east=50+i, west=20+i, top=i, bottom=0)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_north(120+i)
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = stvds_spatial_extent(ident="soil" + str(i) + "@PERMANENT", north=100 + i, south=10+i, east=50+i, west=20+i, top=i, bottom=0)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_north(120+i)
	    base.update()
	    base.select()
	    base.print_self()

def test_relative_timestamp():
	for i in range(2):
	    base = raster_relative_time(ident="soil" + str(i) + "@PERMANENT", interval=i)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_interval(i+1)
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = raster3d_relative_time(ident="soil" + str(i) + "@PERMANENT", interval=i)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_interval(i+1)
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = vector_relative_time(ident="soil" + str(i) + "@PERMANENT", interval=i)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_interval(i+1)
	    base.update()
	    base.select()
	    base.print_self()
            
	for i in range(2):
	    base = strds_relative_time(ident="soil" + str(i) + "@PERMANENT", interval=i, granularity=5.5)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_interval(i+1)
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = str3ds_relative_time(ident="soil" + str(i) + "@PERMANENT", interval=i, granularity=5.5)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_interval(i+1)
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = stvds_relative_time(ident="soil" + str(i) + "@PERMANENT", interval=i, granularity=5.5)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_interval(i+1)
	    base.update()
	    base.select()
	    base.print_self()
            
def test_map_metadata():
	for i in range(2):
	    base = raster_metadata(ident="soil" + str(i) + "@PERMANENT", strds_register="PERMANENT_soil_strds_register", datatype="CELL", \
			    cols=500, rows=400, number_of_cells=200000,nsres=1, ewres=1, min=0, max=33)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_datatype("FCELL")
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = raster3d_metadata(ident="soil" + str(i) + "@PERMANENT", str3ds_register="PERMANENT_soil_str3ds_register", datatype="FCELL", \
			    cols=500, rows=400, depths=20, number_of_cells=200000,nsres=1, ewres=1, tbres=10, min=0, max=33)
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_datatype("DCELL")
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = vector_metadata(ident="soil" + str(i) + "@PERMANENT", stvds_register="PERMANENT_soil_stvds_register")
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_stvds_register("PERMANENT_soil_stvds_register")
	    base.update()
	    base.select()
	    base.print_self()
            
	for i in range(2):
	    base = strds_metadata(ident="soil" + str(i) + "@PERMANENT", raster_register="PERMANENT_soil_raster_register", \
                                   title="Test", description="Test description")
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_title("More tests")
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = str3ds_metadata(ident="soil" + str(i) + "@PERMANENT", raster3d_register="PERMANENT_soil_raster3d_register", \
                                   title="Test", description="Test description")
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_title("More tests")
	    base.update()
	    base.select()
	    base.print_self()

	for i in range(2):
	    base = stvds_metadata(ident="soil" + str(i) + "@PERMANENT", vector_register="PERMANENT_soil_vector_register", \
                                   title="Test", description="Test description")
	    base.insert()
	    base.select()
	    base.print_self()
	    base.clear()
	    base.set_title("More tests")
	    base.update()
	    base.select()
	    base.print_self()
            
def test_base_absolute_time_extent_metadata():

	for i in range(10):
	    base = vector_base(ident="water" + str(i) + "@PERMANENT", name="water" + str(i), mapset="PERMANENT", creator="soeren")
	    base.insert()
	    base = raster_base(ident="water" + str(i) + "@PERMANENT", name="water" + str(i), mapset="PERMANENT", creator="soeren")
	    base.insert()
	    base = raster3d_base(ident="water" + str(i) + "@PERMANENT", name="water" + str(i), mapset="PERMANENT", creator="soeren")
	    base.insert()

	for i in range(10):
	    base = vector_base(ident="water" + str(i) + "@PERMANENT")
	    base.set_creator("rene")
	    base.update()
	    base = raster_base(ident="water" + str(i) + "@PERMANENT")
	    base.set_creator("rene")
	    base.update()
	    base = raster3d_base(ident="water" + str(i) + "@PERMANENT")
	    base.set_creator("rene")
	    base.update()

	for i in range(10):
	    base = vector_absolute_time(ident="water" + str(i) + "@PERMANENT", start_time=datetime.now(), end_time=datetime.now(), timezone=1)
	    base.insert()
	    base = raster_absolute_time(ident="water" + str(i) + "@PERMANENT", start_time=datetime.now(), end_time=datetime.now(), timezone=1)
	    base.insert()
	    base = raster3d_absolute_time(ident="water" + str(i) + "@PERMANENT", start_time=datetime.now(), end_time=datetime.now(), timezone=1)
	    base.insert()

	for i in range(10):
	    base = vector_absolute_time(ident="water" + str(i) + "@PERMANENT")
	    base.set_start_time(datetime(2010, 6, 1))
	    base.update()
	    base = raster_absolute_time(ident="water" + str(i) + "@PERMANENT")
	    base.set_start_time(datetime(2010, 6, 1))
	    base.update()
	    base = raster3d_absolute_time(ident="water" + str(i) + "@PERMANENT")
	    base.set_start_time(datetime(2010, 6, 1))
	    base.update()

	for i in range(10):
	    base = vector_spatial_extent(ident="water" + str(i) + "@PERMANENT", north=100 + i, south=10+i, east=50+i, west=20+i, top=i, bottom=0)
	    base.insert()
	    base = raster_spatial_extent(ident="water" + str(i) + "@PERMANENT", north=100 + i, south=10+i, east=50+i, west=20+i, top=0, bottom=0)
	    base.insert()
	    base = raster3d_spatial_extent(ident="water" + str(i) + "@PERMANENT", north=100 + i, south=10+i, east=50+i, west=20+i, top=i, bottom=0)
	    base.insert()

	for i in range(10):
	    base = vector_spatial_extent(ident="water" + str(i) + "@PERMANENT")
	    base.set_north(120 + i)
	    base.set_south(20 + i)
	    base.update()
	    base = raster_spatial_extent(ident="water" + str(i) + "@PERMANENT")
	    base.set_north(120 + i)
	    base.set_south(20 + i)
	    base.update()
	    base = raster3d_spatial_extent(ident="water" + str(i) + "@PERMANENT")
	    base.set_north(120 + i)
	    base.set_south(20 + i)
	    base.update()

	for i in range(10):
	    base = vector_metadata(ident="water" + str(i) + "@PERMANENT", stvds_register="PERMANENT_water_stvds_register")
	    base.insert()
	    base = raster_metadata(ident="water" + str(i) + "@PERMANENT", strds_register="PERMANENT_water_strds-register", datatype="CELL", \
			    cols=500, rows=400, number_of_cells=200000,nsres=1, ewres=1, min=0, max=33)
	    base.insert()
	    base = raster3d_metadata(ident="water" + str(i) + "@PERMANENT", str3ds_register="PERMANENT_water_str3ds-register", datatype="FCELL", \
			    cols=500, rows=400, depths=20, number_of_cells=200000,nsres=1, ewres=1, tbres=10, min=0, max=33)
	    base.insert()

	for i in range(10):
	    base = vector_metadata(ident="water" + str(i) + "@PERMANENT")
	    base.set_stvds_register("PERMANENT_water_stvds_register")
	    base.update()
	    base = raster_metadata(ident="water" + str(i) + "@PERMANENT")
	    base.set_datatype("DCELL")
	    base.update()
	    base = raster3d_metadata(ident="water" + str(i) + "@PERMANENT")
	    base.set_datatype("DCELL")
	    base.update()

	for i in range(10):
	    base = vector_base(ident="water" + str(i) + "@PERMANENT")
	    base.select()
	    base.print_self()
	    base = raster_base(ident="water" + str(i) + "@PERMANENT")
	    base.select()
	    base.print_self()
	    base = raster3d_base(ident="water" + str(i) + "@PERMANENT")
	    base.select()
	    base.print_self()

	print "Create a raster object"

	rds = raster_dataset("water0@PERMANENT")
	rds.select()
	rds.print_self()

	print rds.temporal_relation(rds)

def test_absolut_time_temporal_relations():

    A = raster_absolute_time(ident="test1@PERMANENT", start_time=datetime(year=2000, month=1, day=1), \
		                                        end_time=datetime(year=2001, month=1, day=1), timezone=1)
    B = raster_absolute_time(ident="test2@PERMANENT", start_time=datetime(year=2001, month=1, day=1), \
		                                        end_time=datetime(year=2002, month=1, day=1), timezone=1)

    print "Precedes: ", A.temporal_relation(B)
    print "Follows:  ", B.temporal_relation(A)

    A = raster_absolute_time(ident="test1@PERMANENT", start_time=datetime(year=2000, month=1, day=1), \
		                                        end_time=datetime(year=2001, month=1, day=1), timezone=1)
    B = raster_absolute_time(ident="test2@PERMANENT", start_time=datetime(year=2001, month=1, day=2), \
		                                        end_time=datetime(year=2002, month=1, day=1), timezone=1)

    print "Before:   ", A.temporal_relation(B)
    print "After:    ", B.temporal_relation(A)

    A = raster_absolute_time(ident="test1@PERMANENT", start_time=datetime(year=2000, month=1, day=1), \
		                                        end_time=datetime(year=2001, month=1, day=1), timezone=1)
    B = raster_absolute_time(ident="test2@PERMANENT", start_time=datetime(year=2000, month=1, day=1), \
		                                        end_time=datetime(year=2002, month=1, day=1), timezone=1)

    print "Starts:   ", A.temporal_relation(B)
    print "Started:  ", B.temporal_relation(A)

    A = raster_absolute_time(ident="test1@PERMANENT", start_time=datetime(year=2000, month=1, day=1), \
		                                        end_time=datetime(year=2001, month=1, day=1), timezone=1)
    B = raster_absolute_time(ident="test2@PERMANENT", start_time=datetime(year=2000, month=1, day=2), \
		                                        end_time=datetime(year=2001, month=1, day=1), timezone=1)

    print "Finished: ", A.temporal_relation(B)
    print "Finishes: ", B.temporal_relation(A)

    A = raster_absolute_time(ident="test1@PERMANENT", start_time=datetime(year=2000, month=1, day=1), \
		                                        end_time=datetime(year=2001, month=1, day=1), timezone=1)
    B = raster_absolute_time(ident="test2@PERMANENT", start_time=datetime(year=2000, month=3, day=1), \
		                                        end_time=datetime(year=2000, month=9, day=1), timezone=1)

    print "Contains: ", A.temporal_relation(B)
    print "During:   ", B.temporal_relation(A)


    A = raster_absolute_time(ident="test1@PERMANENT", start_time=datetime(year=2000, month=1, day=1), \
		                                        end_time=datetime(year=2000, month=6, day=1), timezone=1)
    B = raster_absolute_time(ident="test2@PERMANENT", start_time=datetime(year=2000, month=3, day=1), \
		                                        end_time=datetime(year=2000, month=9, day=1), timezone=1)

    print "Overlap:   ", A.temporal_relation(B)
    print "Overlapped:", B.temporal_relation(A)

    A = raster_absolute_time(ident="test1@PERMANENT", start_time=datetime(year=2000, month=1, day=1), \
		                                        end_time=datetime(year=2000, month=1, day=1), timezone=1)
    B = raster_absolute_time(ident="test2@PERMANENT", start_time=datetime(year=2000, month=1, day=1), \
		                                        end_time=datetime(year=2000, month=1, day=1), timezone=1)

    print "Equivalent:", A.temporal_relation(B)
    print "Equivalent:", B.temporal_relation(A)


def test_raster_dataset():
    
    # Create a test map
    grass.raster.mapcalc("test = sin(x()) + cos(y())", overwrite = True)
    
    name = "test"
    mapset =  grass.gisenv()["MAPSET"]
    
    print "Create a raster object"

    # We need to specify the name and the mapset as identifier
    rds = raster_dataset(name + "@" + mapset)
    
    # Load data from the raster map in the mapset
    rds.load()
    
    print "Is in db: ", rds.is_in_db()
    
    if rds.is_in_db():      
        # Remove the entry if it is in the db
        rds.delete()
    
    # Set the absolute valid time
    rds.set_absolute_time(start_time= datetime(year=2000, month=1, day=1), \
                            end_time= datetime(year=2010, month=1, day=1))
    # Insert the map data into the SQL database
    rds.insert()
    # Print self info
    rds.print_self()
    # The temporal relation must be equal
    print rds.temporal_relation(rds)

def test_raster3d_dataset():
    
    # Create a test map
    grass.raster3d.mapcalc3d("test = sin(x()) + cos(y()) + sin(z())", overwrite = True)
    
    name = "test"
    mapset =  grass.gisenv()["MAPSET"]
    
    print "Create a raster object"

    # We need to specify the name and the mapset as identifier
    r3ds = raster3d_dataset(name + "@" + mapset)
    
    # Load data from the raster map in the mapset
    r3ds.load()
    
    print "Is in db: ", r3ds.is_in_db()
    
    if r3ds.is_in_db():      
        # Remove the entry if it is in the db
        r3ds.delete()
    
    # Set the absolute valid time
    r3ds.set_absolute_time(start_time= datetime(year=2000, month=1, day=1), \
                            end_time= datetime(year=2010, month=1, day=1))
                            
    # Insert the map data into the SQL database
    r3ds.insert()
    # Print self info
    r3ds.print_self()
    # The temporal relation must be equal
    print r3ds.temporal_relation(r3ds)

def test_vector_dataset():
    
    # Create a test map
    grass.run_command("v.random", output="test", n=20, column="height", zmin=0, \
                      zmax=100, flags="z", overwrite = True)
    
    name = "test"
    mapset =  grass.gisenv()["MAPSET"]
    
    print "Create a vector object"

    # We need to specify the name and the mapset as identifier
    vds = vector_dataset(name + "@" + mapset)
    
    # Load data from the raster map in the mapset
    vds.load()
    
    print "Is in db: ", vds.is_in_db()
    
    if vds.is_in_db():      
        # Remove the entry if it is in the db
        vds.delete()
    
    # Set the absolute valid time
    vds.set_absolute_time(start_time= datetime(year=2000, month=1, day=1), \
                            end_time= datetime(year=2010, month=1, day=1))
    # Insert the map data into the SQL database
    vds.insert()
    # Print self info
    vds.print_self()
    # The temporal relation must be equal
    print vds.temporal_relation(vds)


def test_strds_dataset():
    
    name = "strds_test_1"
    mapset =  grass.gisenv()["MAPSET"]

    print "Create a strds object"

    # We need to specify the name and the mapset as identifier
    strds = space_time_raster_dataset(ident = name + "@" + mapset)
    # Check if in db
    print "Is strds in db: ", strds.is_in_db()
    # Create a new entry if not in db
    if strds.is_in_db() == False:
        strds.set_initial_values(temporal_type = "absolute", granularity="1 day",\
        semantic_type="event", title="This is a test space time raster dataset", description="A space time raster dataset for testing")
        strds.insert()
    
    # Reread the data from the db
    strds.select()
    # Print self info
    strds.print_self()

    # Create a test maps
    for i in range(11):
        i = i + 1
        grass.raster.mapcalc("test" + str(i) + " = sin(x()) + cos(y())", overwrite = True)
    
        name = "test" + str(i)
        mapset =  grass.gisenv()["MAPSET"]
        ident = name + "@" + mapset

        print "Create a raster object"

        # We need to specify the name and the mapset as identifier
        rds = raster_dataset(ident)

        # Load data from the raster map in the mapset
        rds.load()

        print "Is raster in db: ", rds.is_in_db()

        if rds.is_in_db():      
            rds.select()
            rds.print_self()
            # Remove the entry if it is in the db
            rds.delete()
            rds.reset(ident)
            rds.load()

        # Set the absolute valid time
        rds.set_absolute_time(start_time= datetime(year=2000, month=i, day=1), \
                                end_time= datetime(year=2000, month=i + 1, day=1))
        # Insert the map data into the SQL database
        rds.insert()
        # Register the map in the space time raster dataset
        strds.register_map(rds)
        # Print self info
        rds.print_self()
    
    strds.select()
    # Print self info
    strds.print_self()

#test_dict_sql_serializer()
create_temporal_database()
#test_dataset_identifer()
#test_absolute_timestamp()
#test_relative_timestamp()
#test_spatial_extent()
#test_map_metadata()
#test_base_absolute_time_extent_metadata()
#test_absolut_time_temporal_relations()
#test_raster_dataset()
#test_raster3d_dataset()
#test_vector_dataset()

test_strds_dataset()