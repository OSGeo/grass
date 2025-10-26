## DESCRIPTION

*r.out.pov* converts a user-specified raster map layer (map==name) into
a height-field file for POVray (tga==name). The hftype==value option
(where value is either 0 or 1) specifies the height-field type. When the
user enters 0 the output will be actual heights. If entered 1 the
cell-values will be normalized. If hftype is 0 (actual heights) the
bias==value can be used to add or subtract a value from heights. Use
scale==value to scale your heights by value. The GRASS program r.out.pov
can be used to create height- field files for Persistence of Vision
(POV) raytracer. POV can use a height-field defined in Targa (.TGA)
image file format where the RGB pixel values are 24 bits (3 bytes). A 16
bit unsigned integer height-field value is assigned as follows: RED =
high byte, GREEN = low byte, BLUE = empty.

## EXAMPLE

An example Povray script file may look like this:

```sh
#include "shapes.inc"
#include "colors.inc"
#include "textures.inc"

#declare Scale = 7;

light_source { <40000, Scale*3000, 5000> color MainLight }

camera {
   location < 23000, Scale*2000, 0>
   angle  90
   look_at < 23000, Scale*1400, 5000>
}

height_field  {
   tga "dem.lr.tga"
   smooth
   water_level 0.11  // 726 / 6553.6 = 0.111
    texture {
      pigment {
          image_map { // image is always projected from -z, with front facing  +z, top to +Y
             ppm "map.lr.ppm"
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
   scale < 14500, Scale*6553.6, 13000 >
   translate <18300, 0, 1100>
}
```

## AUTHOR

Klaus D. Meyer, GEUM.tec GbR, eMail: *<GEUM.tec@geum.de>*
