
********************************************************************************
 6s - Second Simulation of Satellite Signal in the Solar Spectrum.

 To be adapted by Christo A. Zietsman for conversion from Fortran to C.
 November 2002.

 Adapted by Mauro A. Homem Antunes for atmopheric corrections of 
 remotely sensed images in raw format (.RAW) of 8 bits. 
 April 4, 2001.

 Please refer to the following paper and acknowledge the authors of
 the model:

 Vermote, E.F., Tanre, D., Deuze, J.L., Herman, M., and Morcrette,
    J.J., (1997), Second simulation of the satellite signal in 
    the solar spectrum, 6S: An overview., IEEE Trans. Geosc.
    and Remote Sens. 35(3):675-686.

 The code is provided as is and is not to be sold. See notes on
 http://loasys.univ-lille1.fr/informatique/sixs_gb.html
 and on http://www.ltid.inpe.br/dsr/mauro/6s/index.html

********************************************************************************

ETM4_atmospheric_input.txt                                                       = File with the atmospheric and sensor conditions
ETM4.res                                                                         = Output file showing the atmospheric and sensor conditions
ETM4_400x400.raw                                                                 = Image input file
ETM4_400x400_atms_corr.raw                                                       = Image output file




* ****************************** 6s version 4.2b ****************************** *
*                        geometrical conditions identity                        *
*                        -------------------------------                        *
*                        etm+     observation                                   *
*                                                                               *
*    month: 2 day: 19                                                           *
*    solar zenith angle:   35.65 deg  solar azimuthal angle:       80.87 deg    *
*    view zenith angle:     0.00 deg  view azimuthal angle:         0.00 deg    *
*    scattering angle:    144.35 deg  azimuthal angle difference:  80.87 deg    *
*                                                                               *
*                        atmospheric model description                          *
*                        -----------------------------                          *
*            atmospheric model identity :                                       *
*                tropical            (uh2o=4.12g/cm2,uo3=.247cm-atm)            *
*                                                                               *
*            aerosols type identity :                                           *
*                    Continental aerosols model                                 *
*                                                                               *
*            optical condition identity :                                       *
*                 visibility :   15.00 km  opt. thick. 550nm :   0.3158         *
*                                                                               *
*                        spectral condition                                     *
*                        ------------------                                     *
*            etm+ 4                                                             *
*                value of filter function :                                     *
*                 wl inf=    0.740 mic   wl sup=    0.913 mic                   *
*                                                                               *
*                        target type                                            *
*                        -----------                                            *
*            homogeneous ground                                                 *
*              constant reflectance over the spectra     0.000                  *
*                                                                               *
*                        target elevation description                           *
*                        ----------------------------                           *
*            ground pressure  [mb]        946.12                                *
*            ground altitude  [km]        0.600                                 *
*                 gaseous content at target level:                              *
*                 uh2o=    3.104 g/cm2        uo3=    0.246 cm-atm              *
*                                                                               *
*                         atmospheric correction activated                      *
*                         --------------------------------                      *
