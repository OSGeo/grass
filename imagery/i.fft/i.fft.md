## DESCRIPTION

*i.fft* is an image processing program based on the FFT algorithm given
by Frigo et al. (1998), that processes a single input raster map layer
(**input**) and constructs the real and imaginary Fourier components in
frequency space.

## NOTES

The real and imaginary components are stored into the **real** and
**imaginary** raster map layers. In these raster map layers the low
frequency components are in the center and the high frequency components
are toward the edges. The **input** need not be square. A color table is
assigned to the resultant map layer.

The current geographic region and mask settings are respected when
reading the input file. The presence of nulls or a mask will make the
resulting fast Fourier transform invalid.

## EXAMPLE

North Carolina example:

```sh
g.region raster=lsat7_2002_70
i.fft input=lsat7_2002_70 real=lsat7_2002_70.real imaginary=lsat7_2002_70.imag

# set region to resulting FFT output map (due to new FFT coordinate space):
g.region raster=lsat7_2002_70.real -p
d.mon x0
d.rast lsat7_2002_70.real
d.rast lsat7_2002_70.imag
```

## REFERENCES

- M. Frigo and S. G. Johnson (1998): "FFTW: An Adaptive Software
  Architecture for the FFT". See [www.FFTW.org](https://fftw.org): FFTW
  is a C subroutine library for computing the Discrete Fourier Transform
  (DFT) in one or more dimensions, of both real and complex data, and of
  arbitrary input size.
- John A. Richards, 1986. Remote Sensing Digital Image Analysis,
  Springer-Verlag.
- Personal communication, between program author and Ali R. Vali, Space
  Research Center, [University of Texas](https://www.utexas.edu),
  Austin, 1990.

## SEE ALSO

*[i.cca](i.cca.md), [g.gui.iclass](g.gui.iclass.md),
[i.ifft](i.ifft.md), [i.pca](i.pca.md)*

## AUTHORS

David Satnik, GIS Laboratory, Central Washington University  
Glynn Clements (FFTW support)
