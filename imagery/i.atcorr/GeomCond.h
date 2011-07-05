#ifndef GEOMETRICAL_CONTITIONS_H
#define GEOMETRICAL_CONTITIONS_H

/* **********************************************************************c */
/*       igeom               geometrical conditions                     c */
/*               --------------------------------------                 c */
/*                                                                      c */
/*                                                                      c */
/*   you choose your own conditions; igeom=0                            c */
/*         0     enter solar zenith angle   (in degrees )               c */
/*                     solar azimuth angle        "                     c */
/*                     satellite zenith angle     "                     c */
/*                     satellite azimuth angle    "                     c */
/*                     month                                            c */
/*                     day of the month                                 c */
/*                                                                      c */
/*   or you select one of the following satellite conditions:igeom=1to7 c */
/*         1       meteosat observation                                 c */
/*                 enter month,day,decimal hour (universal time-hh.ddd) c */
/*                       n. of column,n. of line.(full scale 5000*2500) c */
/*                                                                      c */
/*         2       goes east observation                                c */
/*                 enter month,day,decimal hour (universal time-hh.ddd) c */
/*                      n. of column,n. of line.(full scale 17000*12000)c */
/*                                                                      c */
/*         3       goes west observation                                c */
/*                 enter month,day,decimal hour (universal time-hh.ddd) c */
/*                      n. of column,n. of line.(full scale 17000*12000)c */
/*                                                                      c */
/*         4       avhrr ( PM noaa )                                    c */
/*                 enter month,day,decimal hour (universal time-hh.ddd) c */
/*                       n. of column(1-2048),xlonan,hna                c */
/*                       give long.(xlonan) and overpass hour (hna) at  c */
/*                       the ascendant node at equator                  c */
/*                                                                      c */
/*         5       avhrr ( AM noaa )                                    c */
/*                 enter month,day,decimal hour (universal time-hh.ddd) c */
/*                       n. of column(1-2048),xlonan,hna                c */
/*                       give long.(xlonan) and overpass hour (hna) at  c */
/*                       the ascendant node at equator                  c */
/*                                                                      c */
/*         6       hrv   ( spot )    * enter month,day,hh.ddd,long.,lat.c */
/*                                                                      c */
/*         7       tm    ( landsat ) * enter month,day,hh.ddd,long.,lat.c */
/*                                                                      c */
/*         8       etm+  ( landsat7) * enter month,day,hh.ddd,long.,lat.c */
/*                                                                      c */
/*         9       liss  ( IRC 1C )  * enter month,day,hh.ddd,long.,lat.c */
/*                                                                      c */
/*         10      aster             * enter month,day,hh.ddd,long.,lat.c */
/*                                                                      c */
/*         11      avnir             * enter month,day,hh.ddd,long.,lat.c */
/*                                                                      c */
/*         12      ikonos            * enter month,day,hh.ddd,long.,lat.c */
/*                                                                      c */
/*         13      rapideye          * enter month,day,hh.ddd,long.,lat.c */
/*                                                                      c */
/*         14      vgt1_spot4        * enter month,day,hh.ddd,long.,lat.c */
/*                                                                      c */
/*         15      vgt2_spot5        * enter month,day,hh.ddd,long.,lat.c */
/*                                                                      c */
/*                                                                      c */
/*     note:       for hrv and tm experiments long. and lat. are the    c */
/*                 coordinates of the scene center.                     c */
/*                 lat. must be > 0 for north lat., < 0 for south lat.  c */
/*                 long. must be > 0 for east long., <0 for west long.  c */
/*                                                                      c */
/*                 solar and viewing positions are computed             c */
/*                                                                      c */
/* **********************************************************************c */

struct GeomCond
{
	long int igeom;	/* geometrical conditions */

	/* primary */
	float asol;
	float phi0;
	float avis;
	float phiv;
	long int month;
	long int jday;
	float xlon;
	float xlat;

	/* some vars */
	float phi;
	float phirad;
	float xmus; 
	float xmuv; 
	float xmup; 
	float xmud;
	float adif;

    float dsol;

	void  print();

private:
	/* conversion routines */
	void possol(float tu);
	void landsat(float tu);
	void posobs(float tu, int nc, int nl);
	void posnoa(float tu, int nc, float xlonan, float campm, float hna);

	void day_number(long int ia, long int& j);
	void pos_fft (long int j, float tu);

	float varsol();	/* returns dsol as in fortran proggie */
	void parse();
public:
	static GeomCond Parse();
};

#endif /* GEOMETRICAL_CONTITIONS_H */
