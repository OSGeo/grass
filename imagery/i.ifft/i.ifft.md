## DESCRIPTION

*i.ifft* is an image processing program based on the algorithm given by
Frigo et al. (1998), that converts real and imaginary frequency space
images (produced by *[i.fft](i.fft.html)*) into a normal image.

## NOTES

The current mask is respected when reading the real and imaginary
component files; thus, creating a mask is a primary step for selecting
the portion of the frequency space data to be included in the inverse
transform. The module *[wxGUI vector digitizer](wxGUI.vdigit.html)* can
be used to create masks while viewing the real or imaginary component
image. Alternatively *r.circle* can be used to generate high-, low- and
donut filters specifying the DC point as circle/ring center. When
*i.ifft* is executed, it (automatically) uses the same GRASS region
definition setting that was used during the original transformation done
with *[i.fft](i.fft.html)*.

## REFERENCES

-   M. Frigo and S. G. Johnson (1998): \"FFTW: An Adaptive Software
    Architecture for the FFT\". See
    [www.fftw.org](http://www.fftw.org/): FFTW is a C subroutine library
    for computing the Discrete Fourier Transform (DFT) in one or more
    dimensions, of both real and complex data, and of arbitrary input
    size.
-   Richards, J.A (1986): **Remote Sensing Digital Image Analysis**,
    Springer-Verlag, 1986.
-   Personal communication, between program author and Ali R. Vali,
    Space Research Center, University of Texas, Austin, 1990.

## SEE ALSO

*[i.cca](i.cca.html), [g.gui.iclass](g.gui.iclass.html),
[i.fft](i.fft.html), [i.pca](i.pca.html), [r.circle](r.circle.html),
[wxGUI vector digitizer](wxGUI.vdigit.html)*

## AUTHORS

David Satnik, GIS Laboratory, Central Washington University\
Glynn Clements (FFTW support)
