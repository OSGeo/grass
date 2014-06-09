#ifndef IWAVE_H
#define IWAVE_H


/**********************************************************************c
c      iwave input of the spectral conditions                          c
c            --------------------------------                          c
c                                                                      c
c  you choose to define your own spectral conditions: iwave=-1,0 or 1  c
c                   (three user s conditions )                         c
c        -2  enter wlinf, wlsup, the filter function will be equal to 1c
c            over the whole band (as iwave=0) but step by step output  c
c            will be printed                                           c
c        -1  enter wl (monochr. cond,  gaseous absorption is included) c
c                                                                      c
c         0  enter wlinf, wlsup. the filter function will be equal to 1c
c            over the whole band.                                      c
c                                                                      c
c         1  enter wlinf, wlsup and user's filter function s(lambda)   c
c                          ( by step of 0.0025 micrometer).            c
c                                                                      c
c                                                                      c
c   or you select one of the following satellite spectral band         c
c   with indication in brackets of the band limits used in the code :  c
c                                                iwave=2 to 60         c
c         2  vis band of meteosat     ( 0.350-1.110 )                  c
c         3  vis band of goes east    ( 0.490-0.900 )                  c
c         4  vis band of goes west    ( 0.490-0.900 )                  c
c         5  1st band of avhrr(noaa6) ( 0.550-0.750 )                  c
c         6  2nd      "               ( 0.690-1.120 )                  c
c         7  1st band of avhrr(noaa7) ( 0.500-0.800 )                  c
c         8  2nd      "               ( 0.640-1.170 )                  c
c         9  1st band of avhrr(noaa8) ( 0.540-1.010 )                  c
c        10  2nd      "               ( 0.680-1.120 )                  c
c        11  1st band of avhrr(noaa9) ( 0.530-0.810 )                  c
c        12  2nd      "               ( 0.680-1.170 )                  c
c        13  1st band of avhrr(noaa10 ( 0.530-0.780 )                  c
c        14  2nd      "               ( 0.600-1.190 )                  c
c        15  1st band of avhrr(noaa11 ( 0.540-0.820 )                  c
c        16  2nd      "               ( 0.600-1.120 )                  c
c        17  1st band of hrv1(spot1)  ( 0.470-0.650 )                  c
c        18  2nd      "               ( 0.600-0.720 )                  c
c        19  3rd      "               ( 0.730-0.930 )                  c
c        20  pan      "               ( 0.470-0.790 )                  c
c        21  1st band of hrv2(spot1)  ( 0.470-0.650 )                  c
c        22  2nd      "               ( 0.590-0.730 )                  c
c        23  3rd      "               ( 0.740-0.940 )                  c
c        24  pan      "               ( 0.470-0.790 )                  c
c        25  1st band of tm(landsat5) ( 0.430-0.560 )                  c
c        26  2nd      "               ( 0.500-0.650 )                  c
c        27  3rd      "               ( 0.580-0.740 )                  c
c        28  4th      "               ( 0.730-0.950 )                  c
c        29  5th      "               ( 1.5025-1.890 )                 c
c        30  7th      "               ( 1.950-2.410 )                  c
c        31  1st band of mss(landsat5)( 0.475-0.640 )                  c
c        32  2nd      "               ( 0.580-0.750 )                  c
c        33  3rd      "               ( 0.655-0.855 )                  c
c        34  4th      "               ( 0.785-1.100 )                  c
c        35  1st band of MAS (ER2)    ( 0.5025-0.5875)                 c
c        36  2nd      "               ( 0.6075-0.7000)                 c
c        37  3rd      "               ( 0.8300-0.9125)                 c
c        38  4th      "               ( 0.9000-0.9975)                 c
c        39  5th      "               ( 1.8200-1.9575)                 c
c        40  6th      "               ( 2.0950-2.1925)                 c
c        41  7th      "               ( 3.5800-3.8700)                 c
c        42  MODIS   band 1           ( 0.6100-0.6850)                 c
c        43  MODIS   band 2           ( 0.8200-0.9025)                 c
c        44  MODIS   band 3           ( 0.4500-0.4825)                 c
c        45  MODIS   band 4           ( 0.5400-0.5700)                 c
c        46  MODIS   band 5           ( 1.2150-1.2700)                 c
c        47  MODIS   band 6           ( 1.6000-1.6650)                 c
c        48  MODIS   band 7           ( 2.0575-2.1825)                 c
c        49  1st band of avhrr(noaa12 ( 0.500-1.000 )                  c
c        50  2nd      "               ( 0.650-1.120 )                  c
c        51  1st band of avhrr(noaa14 ( 0.500-1.110 )                  c
c        52  2nd      "               ( 0.680-1.100 )                  c
c        53  POLDER  band 1           ( 0.4125-0.4775)                 c
c        54  POLDER  band 2 (non polar( 0.4100-0.5225)                 c
c        55  POLDER  band 3 (non polar( 0.5325-0.5950)                 c
c        56  POLDER  band 4   P1      ( 0.6300-0.7025)                 c
c        57  POLDER  band 5 (non polar( 0.7450-0.7800)                 c
c        58  POLDER  band 6 (non polar( 0.7000-0.8300)                 c
c        59  POLDER  band 7   P1      ( 0.8100-0.9200)                 c
c        60  POLDER  band 8 (non polar( 0.8650-0.9400)                 c
c        61  1st band of etm+(landsat7( 0.435-0.520 )                  c
c        62  2nd      "               ( 0.506-0.621 )                  c
c        63  3rd      "               ( 0.622-0.702 )                  c
c        64  4th      "               ( 0.751-0.911 )                  c
c        65  5th      "               ( 1.512-1.792 )                  c
c        66  7th      "               ( 2.020-2.380 )                  c
c        67  8th      "               ( 0.504-0.909 )                  c
c        68  2nd band of liss (IRC 1C)( 0.502-0.620 )                  c
c        69  3rd      "               ( 0.612-0.700 )                  c
c        70  4th      "               ( 0.752-0.880 )                  c
c        71  5th      "               ( 1.452-1.760 )                  c
c        72  1st band of aster        ( 0.480-0.645 )                  c
c        73  2nd      "               ( 0.588-0.733 )                  c
c        74  3N       "               ( 0.723-0.913 )                  c
c        75  4th      "               ( 1.530-1.750 )                  c
c        76  5th      "               ( 2.103-2.285 )                  c
c        77  6th      "               ( 2.105-2.298 )                  c
c        78  7th      "               ( 2.200-2.393 )                  c
c        79  8th      "               ( 2.248-2.475 )                  c
c        80  9th      "               ( 2.295-2.538 )                  c
c        81  1st band of avnir        ( 0.390-0.550 )                  c
c        82  2nd      "               ( 0.485-0.695 )                  c
c        83  3rd      "               ( 0.545-0.745 )                  c
c        84  4th      "               ( 0.700-0.925 )                  c
c        85  Green band of IKONOS     ( 0.350-1.035 )                  c
c        86  Red      "               ( 0.350-1.035 )                  c
c        87  NIR      "               ( 0.350-1.035 )                  c
c        88  Blue band of RapidEye    ( 0.438-0.513 )                  c
c        89  Green    "               ( 0.463-0.594 )                  c
c        90  Red      "               ( 0.624-0.690 )                  c
c        91  RedEdge  "               ( 0.500-0.737 )                  c
c        92  NIR      "               ( 0.520-0.862 )                  c
c        93  B0 band of VGT1 (SPOT4)  ( 0.400-0.500 )                  c
c        94  B2       "               ( 0.580-0.782 )                  c
c        95  B3       "               ( 0.700-1.030 )                  c
c        96  MIR      "               ( 1.450-1.800 )                  c
c        97  B0 band of VGT2 (SPOT5)  ( 0.400-0.550 )                  c
c        98  B2       "               ( 0.580-0.780 )                  c
c        99  B3       "               ( 0.700-1.000 )                  c
c        100 MIR      "               ( 1.450-1.800 )                  c
c  note: wl has to be in micrometer                                    c
c**********************************************************************/

struct IWave
{
	int iwave;
	int iinf;
	int isup;

	float wl;
	float wlmoy;

	
	struct FFu
	{
		float s[1501];
		float wlinf;
		float wlsup;
	} ffu;

private:	
	void parse();

	void meteo();
	void goes_east();
	void goes_west();
	void avhrr(int iwa);
	void hrv(int iwa);
	void tm(int iwa);
	void mss(int iwa);
	void mas(int iwa);
	void modis(int iwa);
	void polder(int iwa);
	void etmplus(int iwa);
	void irs_1c_liss(int iwa);
	void aster(int iwa);
	void avnir(int iwa);
	void ikonos(int iwa);
	void rapideye(int iwa);
	void vgt1_spot4(int iwa);
	void vgt2_spot5(int iwa);


public:
	/* To compute the equivalent wavelength needed for the calculation of the
	  downward radiation field used in the computation of the non lambertian 
	  target contribution (main.f). */
	float equivwl() const;

	/* To read the solar irradiance (in Wm-2mm-1) from 250 nm to 4000 nm by 
	steps of 2.5 nm, The total solar irradiance is put equal to 1372 Wm-2. 
	Between 250 and 4000 nm we have 1358 Wm-2. */
	float solirr(float wl) const;

	void print();
	static IWave Parse();
};

#endif /* IWAVE_H */
