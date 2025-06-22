## DESCRIPTION

*r.transect* outputs, in ASCII, the values in a raster map which lie
along one or more user-defined transect lines. The transects are
described by their starting coordinates, azimuth, and distance.

The **line** parameter is a definition of (each) transect line,
specified by the geographic coordinates of its starting point (*easting,
northing*), the angle and direction of its travel (*azimuth*), and its
distance (*distance*).

The *azimuth* is an angle, in degrees, measured to the east of north.
The *distance* is in map units (meters for a metered database, like
UTM).

The **null** parameter can optionally be set to change the character
string representing null values.

## NOTES

This program is a front-end to the *[r.profile](r.profile.md)* program.
It simply converts the azimuth and distance to an ending coordinate and
then runs *[r.profile](r.profile.md)*. There once were **width=** and
**result**=*raw\|median\|average* options which are not currently
implemented.

## SEE ALSO

*[r.profile](r.profile.md), [wxGUI profile tool](wxGUI.md)*

## AUTHOR

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory
