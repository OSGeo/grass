#ifndef IWAVE_H
#define IWAVE_H


/***********************************************************************
 *       iwave input of the spectral conditions
 *          --------------------------------                         
 *
 *  you choose to define your own spectral conditions: iwave=-1,0 or 1 
 *                    (three user s conditions )
 *       -2  enter wlinf, wlsup, the filter function will be equal to 1
 *           over the whole band (as iwave=0) but step by step output
 *           will be printed
 *       -1  enter wl (monochr. cond,  gaseous absorption is included)
 * 
 *        0  enter wlinf, wlsup. the filter function will be equal to 1
 *           over the whole band.
 * 
 *        1  enter wlinf, wlsup and user's filter function s(lambda)
 *                         ( by step of 0.0025 micrometer).
 * 
 * 
 *  or you select one of the following satellite spectral band
 *  with indication in brackets of the band limits used in the code :
 *                                               iwave=2 to 60
 *        2  vis band of meteosat     ( 0.350-1.110 )
 *        3  vis band of goes east    ( 0.490-0.900 )
 *        4  vis band of goes west    ( 0.490-0.900 )
 *        5  1st band of avhrr(noaa6) ( 0.550-0.750 )
 *        6  2nd      "               ( 0.690-1.120 )
 *        7  1st band of avhrr(noaa7) ( 0.500-0.800 )
 *        8  2nd      "               ( 0.640-1.170 )
 *        9  1st band of avhrr(noaa8) ( 0.540-1.010 )
 *        [...] - see iwave.cpp
 * 
 *  note: wl has to be in micrometer
***********************************************************************/

struct IWave
{
	int iwave;
	int iinf;
	int isup;

	double wl;
	double wlmoy;

	
	struct FFu
	{
		double s[1501];
		double wlinf;
		double wlsup;
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
	void worldview2(int iwa);
	void quickbird2(int iwa);
	void landsat_8(int iwa);
	void geoeye1(int iwa);
	void spot6(int iwa);
	void spot7(int iwa);
	void pleiades1a(int iwa);
	void pleiades1b(int iwa);
	void worldview3(int iwa);
	void sentinel2a(int iwa);
	void sentinel2b(int iwa);
	void planetscope0c0d(int iwa);
	void planetscope0e(int iwa);
	void planetscope0f10(int iwa);
	void worldview4(int iwa);

public:
	/* To compute the equivalent wavelength needed for the calculation of the
	  downward radiation field used in the computation of the non lambertian 
	  target contribution (main.f). */
	double equivwl() const;

	/* To read the solar irradiance (in Wm-2mm-1) from 250 nm to 4000 nm by 
	steps of 2.5 nm, The total solar irradiance is put equal to 1372 Wm-2. 
	Between 250 and 4000 nm we have 1358 Wm-2. */
	double solirr(double wl) const;

	void print();
	static IWave Parse();
};

#endif /* IWAVE_H */
