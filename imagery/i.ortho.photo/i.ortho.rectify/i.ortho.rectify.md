## DESCRIPTION

*i.ortho.rectify* rectifies an image by using the image to photo
coordinate transformation matrix created by
[g.gui.photo2image](g.gui.photo2image.md) and the rectification
parameters created by [g.gui.image2target](g.gui.image2target.md).
Rectification is the process by which the geometry of an image is made
planimetric. This is accomplished by mapping an image from one
coordinate system to another. In *i.ortho.rectify* the parameters
computed by [g.gui.photo2image](g.gui.photo2image.md) and
[g.gui.image2target](g.gui.image2target.md) are used in equations to
convert x,y image coordinates to standard map coordinates for each pixel
in the image. The result is an image with a standard map coordinate
system, compensated for relief distortions and photographic tilt. Upon
completion of the program the rectified image is deposited in a
previously targeted GRASS project (location).

Images can be resampled with various different interpolation methods:
nearest neighbor assignment, bilinear and bicubic interpolation. The
bilinear and bicubic interpolation methods are also available with a
fallback option. These methods "fall back" to simpler interpolation
methods along NULL borders. That is, from bicubic to bilinear to
nearest.

The process may take an hour or more depending on the size of the image,
the speed of the computer, the number files, and the size and resolution
of the selected window.

The rectified image will be located in the target project when the
program is completed. The original unrectified files are not modified or
removed.

The optional *angle* output holds the camera angle in degrees to the
local surface, considering local slope and aspect. A value of 90 degrees
indicates that the camera angle was orthogonal to the local surface, a
value of 0 degrees indicates that the camera angle was parallel to the
local surface and negative values indicate that the surface was
invisible to the camera. As a rule of thumb, values below 30 degrees
indicate problem areas where the orthorectified output will appear
blurred. Because terrain shadowing effects are not considered, areas
with high camera angles may also appear blurred if they are located
(viewed from the camera position) behind mountain ridges or peaks.

*i.ortho.rectify* can be run directly, specifying options in the command
line or the GUI, or it can be invoked as OPTION 8 through
[i.ortho.photo](i.ortho.photo.md). If invoked though
[i.ortho.photo](i.ortho.photo.md), an interactive terminal is used to
determine the options.

### Interactive mode

You are first asked if all images within the imagery group should be
rectified. If this option is not chosen, you are asked to specify for
each image within the imagery group whether it should be rectified or
not.

More than one file may be rectified at a time. Each file should have a
unique output file name. The next prompt asks you for an extension to be
appended to the rectified images.

The next prompt will ask you whether a camera angle map should be
produced and if yes, what should be its name.

After that you are asked if overwriting existing maps in the target
project and mapset should be allowed.

The next prompt asks you to select one of two windows:

```sh
      Please select one of the following options
      1.   Use the current window in the target project
      2.   Determine the smallest window which covers the image
      >
```

If you choose option 2, you can also specify a desired target
resolution.

*i.ortho.rectify* will only rectify that portion of the image that
occurs within the chosen window. Only that portion will be relocated in
the target database. It is therefore important to check the current
window in the target project if choice number one is selected.

Next you are asked to select an interpolation method.

```sh
      Please select one of the following interpolation methods
      1. nearest neighbor
      2. bilinear
      3. bicubic
      4. bilinear with fallback
      5. bicubic with fallback
      >
```

The last prompt will ask you about the amount of memory to be used by
*i.ortho.rectify*.

## SEE ALSO

*[i.ortho.photo](i.ortho.photo.md), [i.ortho.camera](i.ortho.camera.md),
[g.gui.photo2image](g.gui.photo2image.md),
[g.gui.image2target](g.gui.image2target.md),
[i.ortho.init](i.ortho.init.md), [i.rectify](i.rectify.md)*

## AUTHORS

Mike Baba, DBA Systems, Inc.  
Updated rectification and elevation map to FP 1/2002 Markus Neteler  
Bugfixes and enhancements 12/2010 Markus Metz
