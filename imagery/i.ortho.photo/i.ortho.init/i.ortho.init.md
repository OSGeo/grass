## DESCRIPTION

Aerial photographs may be either vertical or oblique. Vertical
photographs can be truly vertical (nadir), or slightly tilted (less than
3 degree from the vertical). Usually aerial photos are tilted to some
degree. We refer to the term *vertical photograph* up to a tilt of 3
degree.  
Oblique aerial photographs are purposely taken with an angle between 3
and 90 degree from the nadir direction.

**The use of *i.ortho.init* (menu 6) is only required when rectifying a
tilted or oblique aerial photo.**

*i.ortho.init* creates or modifies entries in a camera initial exposure
station file for imagery group referenced by a sub-block. These entries
include: the (XC,YC,ZC) standard (e.g. UTM) approximate coordinates of
the camera exposure station; initial roll, pitch, and yaw angles (in
degrees) of the cameras attitude; and the *a priori* standard deviations
for these parameters. During the imagery program, *i.ortho.rectify*, the
initial camera exposure station file is used for computation of the
ortho-rectification parameters. If no initial camera exposure station
file exist, the default values are computed from the control points file
created in *[g.gui.image2target](g.gui.image2target.md)*.

The following menu is displayed:

```sh
        Please provide the following information

    INITIAL XC: Meters                __________
    INITIAL YC: Meters                __________
    INITIAL ZC: Meters                __________
    INITIAL omega (pitch) degrees:    __________
    INITIAL phi  (roll) degrees:      __________
    INITIAL kappa  (yaw) degrees:     __________

    Standard Deviation XC: Meters     __________
    Standard Deviation YC: Meters     __________
    Standard Deviation ZC: Meters     __________
    Std. Dev. omega (pitch) degrees:  __________
    Std. Dev. phi  (roll) degrees:    __________
    Std. Dev. kappa  (yaw) degrees:   __________

        Use these values at run time? (1=yes, 0=no)

     AFTER COMPLETING ALL ANSWERS, HIT <ESC> TO CONTINUE
                  (OR <Ctrl-C> TO CANCEL)
```

The INITIAL values for (XC,YC,ZC) are expressed in standard (e.g. UTM)
coordinates, and represent an approximation for the location of the
camera at the time of exposure.

- X: East aircraft position;
- Y: North aircraft position;
- Z: Flight altitude above sea level

The INITIAL values for (omega,phi,kappa) are expressed in degrees, and
represent an approximation for the cameras attitude at the time of
exposure.

- Omega (pitch): Raising or lowering of the aircraft's front (turning
  around the wings' axis);
- Phi (roll): Raising or lowering of the wings (turning around the
  aircraft's axis);
- Kappa (yaw): Rotation needed to align the aerial photo to true north:
  needs to be denoted as +90 degree for clockwise turn and -90 degree
  for a counterclockwise turn.

If ground control points are available, the INITIAL values are
iteratively corrected. This is particularl useful when the INITIAL
values are rather rough estimates.

The standard deviations for (XC,YC,ZC) are expressed in meters, and are
used as *a priori* values for the standard deviations used in
computation of the ortho rectification parameters. Higher values improve
the refinement of the initial camera exposure. As a rule of thumb, 5% of
the estimated target extents should be used.

The standard deviations for (omega,phi,kappa) are expressed in degrees,
and are used as *a priori* values for the standard deviations used in
computation of the ortho rectification parameters. As a rule of thumb, 2
degrees should be used.

If *Use these values at run time? (1=yes, 0=no)* is set to 0, the values
in this menu are not used.

## SEE ALSO

*[i.ortho.photo](i.ortho.photo.md),
[g.gui.photo2image](g.gui.photo2image.md),
[g.gui.image2target](g.gui.image2target.md),
[i.ortho.elev](i.ortho.elev.md), [i.ortho.camera](i.ortho.camera.md),
[i.ortho.transform](i.ortho.transform.md),
[i.ortho.rectify](i.ortho.rectify.md)*

## AUTHOR

Mike Baba, DBA Systems, Inc.
