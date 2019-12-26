#ifndef GEOMETRICAL_CONTITIONS_H
#define GEOMETRICAL_CONTITIONS_H

/* ************************************************************************/
/*       igeom               geometrical conditions                       */
/*               --------------------------------------                   */
/*                                                                        */
/*                                                                        */
/*   you choose your own conditions; igeom=0                              */
/*         0     enter solar zenith angle   (in degrees )                 */
/*                     solar azimuth angle        "                       */
/*                     satellite zenith angle     "                       */
/*                     satellite azimuth angle    "                       */
/*                     month                                              */
/*                     day of the month                                   */
/*                                                                        */
/*   or you select one of the following satellite conditions:igeom=1to7   */
/*         1       meteosat observation                                   */
/*                 enter month,day,decimal hour (universal time-hh.ddd)   */
/*                       n. of column,n. of line.(full scale 5000*2500)   */
/*                                                                        */
/*         2       goes east observation                                  */
/*                 enter month,day,decimal hour (universal time-hh.ddd)   */
/*                      n. of column,n. of line.(full scale 17000*12000)  */
/*                                                                        */
/*         3       goes west observation                                  */
/*                 enter month,day,decimal hour (universal time-hh.ddd)   */
/*                      n. of column,n. of line.(full scale 17000*12000)  */
/*                                                                        */
/*         4       avhrr ( PM noaa )                                      */
/*                 enter month,day,decimal hour (universal time-hh.ddd)   */
/*                       n. of column(1-2048),xlonan,hna                  */
/*                       give long.(xlonan) and overpass hour (hna) at    */
/*                       the ascendant node at equator                    */
/*                                                                        */
/*         5       avhrr ( AM noaa )                                      */
/*                 enter month,day,decimal hour (universal time-hh.ddd)   */
/*                       n. of column(1-2048),xlonan,hna                  */
/*                       give long.(xlonan) and overpass hour (hna) at    */
/*                       the ascendant node at equator                    */
/*                                                                        */
/*         6       hrv   ( spot )    * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         7       tm    ( landsat ) * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         8       etm+  ( landsat7) * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         9       liss  ( IRC 1C )  * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         10      aster             * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         11      avnir             * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         12      ikonos            * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         13      rapideye          * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         14      vgt1_spot4        * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         15      vgt2_spot5        * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         16      worldview2        * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         17      quickbird2        * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         18      Landsat 8         * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         19      geoeye1           * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         20      spot6             * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         21      spot7             * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         22      pleiades1a        * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         23      pleiades1b        * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         24      worldview3        * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         25      sentinel2a        * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         26      sentinel2b        * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         27      planetscope0c0d   * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         28      planetscope0e     * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         29      planetscope0f10   * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*         30      worldview4        * enter month,day,hh.ddd,long.,lat.  */
/*                                                                        */
/*     note:       for hrv and tm experiments long. and lat. are the      */
/*                 coordinates of the scene center.                       */
/*                 lat. must be > 0 for north lat., < 0 for south lat.    */
/*                 long. must be > 0 for east long., < 0 for west long.   */
/*                                                                        */
/*                 solar and viewing positions are computed               */
/*                                                                        */
/* ************************************************************************/

struct GeomCond
{
	long int igeom;	/* geometrical conditions */

	/* primary */
	double asol;
	double phi0;
	double avis;
	double phiv;
	long int month;
	long int jday;
	double xlon;
	double xlat;

	/* some vars */
	double phi;
	double phirad;
	double xmus; 
	double xmuv; 
	double xmup; 
	double xmud;
	double adif;

    double dsol;

	void  print();

private:
	/* conversion routines */
	void possol(double tu);
	void landsat(double tu);
	void posobs(double tu, int nc, int nl);
	void posnoa(double tu, int nc, double xlonan, double campm, double hna);

	void day_number(long int ia, long int& j);
	void pos_fft (long int j, double tu);

	double varsol();	/* returns dsol as in fortran proggie */
	void parse();
public:
	static GeomCond Parse();
};

#endif /* GEOMETRICAL_CONTITIONS_H */
