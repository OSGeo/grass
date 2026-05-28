## DESCRIPTION

*g.pnmcomp* isn't meant for end users. It's an internal tool for use by
*[wxGUI](wxGUI.md)*.

In essence, *g.pnmcomp* generates a PPM image by overlaying a series of
PPM/PGM pairs (PPM = RGB image, PGM = alpha channel).

## NOTES

The intention is that *d.\** modules will emit PPM/PGM pairs (by way of
the PNG-driver code being integrated into Display Library). The GUI will
manage a set of layers; each layer consists of the data necessary to
generate a PPM/PGM pair. Whenever the layer "stack" changes (by adding,
removing, hiding, showing or re-ordering layers), the GUI will render
any layers for which it doesn't already have the PPM/PGM pair, then
re-run *g.pnmcomp* to generate the final image (just redoing the
composition is a lot faster than redrawing everything).

A C/C++ GUI would either have *g.pnmcomp's* functionality (image
composition) built-in, or would use the system's graphics API to perform
composition (for translucent layers, you would need OpenGL or the Render
extension, or something else which supports translucent rendering).

Tk doesn't support transparent (masked) true-colour images (it does
support transparent GIFs, but that's limited to 256 colours), and an
image composition routine in Tcl would be unacceptably slow, hence the
existence of *g.pnmcomp*.

## SEE ALSO

*[g.cairocomp](g.cairocomp.md)*

## AUTHOR

Glynn Clements
