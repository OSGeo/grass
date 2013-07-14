#!/usr/bin/env python
#
############################################################################
#
# MODULE:      g.isis3mt
# AUTHOR(S):   Alessandro Frigeri; 
# PURPOSE:     Generates an ISIS3 map template file according to the current
#              GRASS projection parameters
# COPYRIGHT:   (C) 2009,2010 by Alessandro Frigeri for the GRASS Development Team
#
#              This program is free software under the GNU General Public
#              License (>=v2). Read the file COPYING that comes with GRASS
#              for details.
# HISTORY:     Apr 15th, 2010: Time for the first public release
#              Sep 15th, 2010: added LatLon Range and mpp/ppd resolution 
#                              options
# 
#############################################################################

# TODO
#  Add a note to the user to use  matchmap=yes in cam2map
 
#%module
#%  description: Generates an ISIS3 map template file according to the current GRASS coordinate reference system
#%  keywords: generic, projection
#%end

#%option
#% key: body
#% type: string
#% description: Target planetary body
#% options: MOON,MERCURY,VENUS,MARS,Phobos,Deimos,JUPITER,Amalthea,Io,Europa,Ganymede,Callisto,SATURN,Janus,Epimetheus,Mimas,Enceladus,Tethys,Dione,Rhea,Titan,Hyperion,Iapetus,URANUS,Miranda,Ariel,Umbriel,Titania,Oberon,NEPTUNE
#% required : yes
#%end

#%option
#% key: out
#% type: string
#% gisprompt: new_file,file,output
#% description: Filename for the ISIS3 maptemplate to be generated
#% required : yes
#%end

#%option
#% key: outres
#% type: double
#% description: Resolution of the ISIS3 projected data (default is: camera resolution) 
#% required : no
#%end

#%Option
#% key: restype
#% type: string
#% required: no
#% multiple: no
#% options: mpp,ppd
#% description: Resolution type: meters per pixel (mpp) or pixels per degree (ppd)
#% answer: mpp
#%End

#%flag
#% key: a
#% description: Align ISIS3 data to the extents of the current GRASS region 
#%end

import subprocess,sys,os,platform
import grass.script as grass
from grass.lib import gis as grasslib
from grass.lib import proj as grassproj
from UserDict import *
import re,string

if "GISBASE" not in os.environ:
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

# initialize
grasslib.G_gisinit('')


projdict = {'sinu':'Sinusoidal',
            'merc':'Mercator',
            'tmerc':'TransverseMercator',
            'ortho':'Orthographic',
            'stere':'PolarStereographic',
            'cc':'SimpleCylindrical',
            'eqc':'Equirectangular',
            'lcc':'LambertConformal'
           }

paradict = {'proj' :'ProjectionName',         # Name of the projection
            'lon_0':'CenterLongitude',        # Center Longitude
            'lat_0':'CenterLatitude',         # Center Latitude
            'k':'ScaleFactor',                # Scale Factor
            'lat_1':'FirstStandardParallel',  # First standard parallel
            'lat_2':'SecondStandardParallel', # Second standard parallel
            'a':'EquatorialRadius',           # semi major axis
            'b':'PolarRadius',                # semi minor axis
            'to_meter':None,                  # no map on isis3
            'y_0':None,  	              # false northing
            'x_0':None,                       # false easting
           }
           
class IsisMapTemplate(IterableUserDict):
   def __init__(self,isisdict):      
      UserDict.__init__(self)      
      if isisdict is not None:
          UserDict.__init__(self,isisdict)    
      self['LatitudeType'] = 'Planetographic'
      self['LongitudeDirection'] = 'PositiveEast'
      self['LongitudeDomain'] = '180'

   def dump(self,out):
       out.write("Group = Mapping\n")
       keys = self.keys()
       for k in self.keys():
           if k != None:               
              myk = string.rjust(k, 30)
              myv = string.ljust(self[k], 20)
              out.write("%s = %s\n"%(myk,myv))
       out.write("End_Group\nEnd")
       

def main():
    isis3 = {}
    outfile = options['out']
    body = options['body']
    pj = grasslib.G_get_projinfo()
    if options['outres'] and options['restype']=='mpp' : isis3['PixelResolution'] = "%f <meters>"%float(options['outres'])
    if options['outres'] and options['restype']=='ppd' : isis3['Scale'] = "%f <pixel/degree>"%float(options['outres'])
    isis3['TargetName'] = body
    if flags['a']:
        ret = grass.start_command("g.region", flags="gl", stdout = subprocess.PIPE)
        exec(ret.communicate()[0])
        isis3['MinimumLatitude'] = "%f"%min(se_lat,sw_lat)
        isis3['MaximumLatitude'] = "%f"%max(ne_lat,nw_lat)
        isis3['MinimumLongitude'] = "%f"%min(sw_long,nw_long)
        isis3['MaximumLongitude'] = "%f"%max(se_long,ne_long)
        
    for p in paradict.keys():
        if grasslib.G_find_key_value(p, pj):
            k = paradict[p]
            v = grasslib.G_find_key_value(p, pj)
            if p == 'a' or p == 'b': v = v + ' <meters>'
            if p == 'proj' and v == 'll': sys.exit("This GRASS location is in LatLong, no cartographic projection is set.\nExiting...")
            if p == 'proj':  k,v = 'ProjectionName', projdict[v]                   
            isis3[k] = v
    isis3mt =  IsisMapTemplate(isis3)
    of = open(outfile,'w')
    isis3mt.dump(of)
    sys.stderr.write("Done writing %s ISIS3 MapTemplate file\n"%outfile)     
    
if __name__ == "__main__":
    options, flags = grass.parser()
    main()



