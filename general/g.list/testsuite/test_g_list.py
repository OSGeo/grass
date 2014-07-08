
import gunittest
from grass.script import read_command

LIST_RASTERS = """----------------------------------------------

raster files available in mapset <PERMANENT>:
aspect                    elevation_shade           lsat7_2002_70
basin_50K                 facility                  lsat7_2002_80
boundary_county_500m      geology_30m               ncmask_500m
cfactorbare_1m            lakes                     ortho_2001_t792_1m
cfactorgrow_1m            landclass96               roadsmajor
el_D782_6m                landcover_1m              slope
el_D783_6m                landuse96_28m             soilsID
el_D792_6m                lsat7_2002_10             soils_Kfactor
el_D793_6m                lsat7_2002_20             streams_derived
elev_lid792_1m            lsat7_2002_30             towns
elev_ned_30m              lsat7_2002_40             urban
elev_srtm_30m             lsat7_2002_50             zipcodes
elev_state_500m           lsat7_2002_61             zipcodes_dbl
elevation                 lsat7_2002_62

"""

LIST_VECTORS = """----------------------------------------------

vector files available in mapset <PERMANENT>:
P079214                   elev_lid792_bepts         poi_names_wake
P079215                   elev_lid792_cont1m        precip_30ynormals
P079218                   elev_lid792_randpts       precip_30ynormals_3d
P079219                   elev_lidrural_mrpts       railroads
boundary_county           elev_lidrural_mrptsft     roadsmajor
boundary_municp           elev_ned10m_cont10m       schools_wake
bridges                   firestations              soils_general
busroute1                 geodetic_pts              soils_wake
busroute11                geodetic_swwake_pts       streams
busroute6                 geology                   streets_wake
busroute_a                geonames_NC               swwake_10m
busroutesall              geonames_wake             urbanarea
busstopsall               hospitals                 usgsgages
census_wake2000           lakes                     zipcodes_wake
censusblk_swwake          nc_state
comm_colleges             overpasses

"""

LIST_GROUPS = """----------------------------------------------
imagery group files available in mapset <landsat>:
lsat7_2000

"""

LIST_RASTERS_MAPSET = """----------------------------------------------
raster files available in mapset <landsat>:
lsat5_1987_10   lsat5_1987_40   lsat5_1987_70   lsat7_2000_30   lsat7_2000_61
lsat5_1987_20   lsat5_1987_50   lsat7_2000_10   lsat7_2000_40   lsat7_2000_70
lsat5_1987_30   lsat5_1987_60   lsat7_2000_20   lsat7_2000_50   lsat7_2000_80

"""

LIST_RASTERS_TITLES = """----------------------------------------------

raster files available in mapset <PERMANENT>:
aspect             South-West Wake county: Aspect [degrees from east]
basin_50K          South-West Wake county: Watersheds derived from 30m NED
boundary_county_500m North Carolina county boundaries
cfactorbare_1m     Rural area: C-factor with fields bare
cfactorgrow_1m     Rural area: C-factor for growing season
el_D782_6m         NC Flood lidar-based 6m(20ft) DEM
el_D783_6m         NC Flood lidar-based 6m(20ft) DEM
el_D792_6m         NC Flood lidar-based 6m(20ft) DEM
el_D793_6m         NC Flood lidar-based 6m(20ft) DEM
elev_lid792_1m     Rural area: Lidar-based 1m DEM
elev_ned_30m       South-West Wake county: National Elevation Data 30m
elev_srtm_30m      South-West Wake county: SRTM-V1 30m terrain surface model
elev_state_500m    North Carolina DEM 500m
elevation          South-West Wake county: Elevation NED 10m
elevation_shade    South-West Wake county: Shaded relief
facility           Rural area: Footprint of planned facility
geology_30m        South-West Wake county: geology derived from vector map
lakes              South-West Wake county: Wake county lakes
landclass96        South-West Wake county: Simplified landuse classes
landcover_1m       Rural area: Landcover
landuse96_28m      South-West Wake county: NC Land Use 1996 clipped
lsat7_2002_10      LANDSAT-TM7 Band 1 Visible (0.45-0.52um) 30m
lsat7_2002_20      LANDSAT-TM7 Band 2 Visible (0.52-0.60um) 30m
lsat7_2002_30      LANDSAT-TM7 Band 3 Visible (0.63-0.69um) 30m
lsat7_2002_40      LANDSAT-TM7 Band 4 Near Infrared (NIR) (0.76-0.90um) 30m
lsat7_2002_50      LANDSAT-TM7 Band 5 Near Infrared (NIR) (1.55-1.75um) 30m
lsat7_2002_61      LANDSAT-TM7 Band 6 Thermal (10.40-12.50um) 60m Low Gain
lsat7_2002_62      LANDSAT-TM7 Band 6 Thermal (10.40-12.50um) 60m High Gain
lsat7_2002_70      LANDSAT-TM7 Band 7 Mid Infrared (MIR) (2.08-2.35um) 30m
lsat7_2002_80      LANDSAT-TM7 Band 8 Panchromatic (PAN) (0.52-0.90um (15m)
ncmask_500m        North Carolina boundary MASK
ortho_2001_t792_1m Rural area (792 tile) NC Flood Orthophoto 2001
roadsmajor         South-West Wake county: roadsmajor
slope              South-West Wake county: slope in degrees
soilsID            South-West Wake county: Soils: ID
soils_Kfactor      Rural area: Soils: K-factor
streams_derived    South-West Wake county: Streams derived from 10m DEM
towns              South West Wake: Cities and towns derived from zipcodes
urban              South West Wake: Urban areas derived from vector map
zipcodes           South West Wake: Zipcode areas derived from vector map
zipcodes_dbl       South West Wake: Zipcode areas from vector map, fp

"""

LIST_RASTERS_TITLES_MAPSET = """----------------------------------------------
raster files available in mapset <landsat>:
lsat5_1987_10      LANDSAT-TM5 Band 1 Visible (0.45-0.52um) 30m
lsat5_1987_20      LANDSAT-TM5 Band 2 Visible (0.52-0.60um) 30m
lsat5_1987_30      LANDSAT-TM5 Band 3 Visible (0.63-0.69um) 30m
lsat5_1987_40      LANDSAT-TM5 Band 4 Near Infrared (NIR) (0.76-0.90um) 30m
lsat5_1987_50      LANDSAT-TM5 Band 5 Near Infrared (NIR) (1.55-1.75um) 30m
lsat5_1987_60      LANDSAT-TM5 Band 6 Thermal (10.40-12.50um) 120m
lsat5_1987_70      LANDSAT-TM5 Band 7 Mid Infrared (MIR) (2.08-2.35um) 30m
lsat7_2000_10      LANDSAT-TM7 Band 1 Visible (0.45-0.52um) 30m
lsat7_2000_20      LANDSAT-TM7 Band 2 Visible (0.52-0.60um) 30m
lsat7_2000_30      LANDSAT-TM7 Band 3 Visible (0.63-0.69um) 30m
lsat7_2000_40      LANDSAT-TM7 Band 4 Near Infrared (NIR) (0.76-0.90um) 30m
lsat7_2000_50      LANDSAT-TM7 Band 5 Near Infrared (NIR) (1.55-1.75um) 30m
lsat7_2000_61      LANDSAT-TM7 Band 6 Thermal (10.40-12.50um) 60m Low Gain
lsat7_2000_70      LANDSAT-TM7 Band 7 Mid Infrared (MIR) (2.08-2.35um) 30m
lsat7_2000_80      LANDSAT-TM7 Band 8 Panchromatic (PAN) (0.52-0.90um (15m)

"""


class GListTest(gunittest.TestCase):

    def test_list_rasters(self):
        """Test human readable list of rasters.

        Supposing we are in user1 of NC and have access
           to landsat (besides PERMANENT).
        """
        stdout = read_command('g.list', type='rast')
        self.assertMultiLineEqual(stdout, LIST_RASTERS)

    def test_list_vectors(self):
        """Test human readable list of vectors.

        Supposing we are in user1 of NC and have access
           to landsat (besides PERMANENT).
        """
        stdout = read_command('g.list', type='vect')
        self.assertMultiLineEqual(stdout, LIST_VECTORS)

    def test_list_groups_in_mapset(self):
        """Test human readable list of imagery groups in a specific mapset.

        Supposing we are in user1 of NC and have access
        to landsat (besides PERMANENT).
        """
        stdout = read_command('g.list', type='group', mapset='landsat')
        self.assertMultiLineEqual(stdout, LIST_GROUPS)

    def test_list_rasters_in_mapset(self):
        """Test human readable list of rasters in a specific mapset.

        Supposing we are in user1 of NC and have access
        to landsat (besides PERMANENT).
        """
        stdout = read_command('g.list', type='rast', mapset='landsat')
        self.assertMultiLineEqual(stdout, LIST_RASTERS_MAPSET)

    def test_list_rasters_titles(self):
        """Test human readable list of rasters with titles.
        """
        stdout = read_command('g.list', flags='f', type='rast')
        self.assertMultiLineEqual(stdout, LIST_RASTERS_TITLES)

    def test_list_rasters_titles_in_mapset(self):
        """Test human readable list of rasters with titles
        in a specific mapset.
        """
        stdout = read_command('g.list', flags='f', type='rast',
                              mapset='landsat')
        self.assertMultiLineEqual(stdout, LIST_RASTERS_TITLES_MAPSET)


if __name__ == '__main__':
    gunittest.test()
