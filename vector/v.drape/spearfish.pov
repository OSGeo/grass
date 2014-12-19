/*
# 2005, Markus Neteler
  for Spearfish 60

####### GRASS procedure:

#setup the region
g.region vector=roads align=elevation.10m -p
#export the vector data
v.drape in=roads out=roads3d rast=elevation.10m
v.out.pov roads3d out=roads3d.pov
#export the raster data
r.out.pov elevation.10m tga=elevation.tga
r.out.png landcover.30m out=landcover30m.png

#Render with:
povray +Ispearfish.pov +Opovview.png +D +P +W1100 +H900 +A0.5
*/

#include "shapes.inc"
#include "colors.inc"
#include "textures.inc"
#include "skies.inc"

#declare Sun = color rgb <0.7, 0.7, 0.7>;
#declare Scale = 1;


light_source { < 604505.0, 29000, 4899930.0 > color White }

fog {
 fog_type 2 
 fog_offset 20 
 fog_alt 65 
 color rgb <0.80, 0.88, 0.82> 
 distance 80 
 up y }

camera {
   location < 612500.0, 4000.0, 4921000.0 >  // x, z, y
   angle  40 // vertical camera aperture
   look_at  < 599500.0, 1200.0, 4921000.0 >  // x, z, y
}


height_field  {
   tga "elevation.tga"
   smooth
   texture { 
      pigment { 
          image_map { // image is always projected from -z, with front facing +z, top to +Y  			
             png "landcover30m.png" 	     
	     once  	
	  } 
	  rotate x*90 // align map to height_field 
      } 
   }
   finish {
	  ambient 0.2         // Very dark shadows
	  diffuse 0.8         // Whiten the whites
	  phong 0.2           // shiny	  
	  phong_size 100.0    // with tight highlights
	  specular 0.5
	  roughness 0.05
   }
   scale  < 20100.0, 65535.0, 14070.0 > // region extend in meters
   translate  < 589430.0, 0.0, 4914000.0 >  // shift to UTM (W,0,S - ??)
}


//background { color SkyBlue }


sky_sphere{
    pigment {planar colour_map{[0,rgb <0.9,0.9,1>][1, rgb <0.1,0.2,1>]}}
    pigment{P_Cloud4}
}

