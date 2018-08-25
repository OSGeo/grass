#ifndef ALTITUDE_H
#define ALTITUDE_H

/**********************************************************************c
c xps is the parameter to express the  altitude of target              c
c                                                                      c
c                                                                      c
c                  xps >=0. means the target is at the sea level       c
c                                                                      c
c                  xps <0. means you know the altitude of the target   c
c                        expressed in km and you put that value as xps c
c                                                                      c
c                                                                      c
c**********************************************************************/

/**********************************************************************c
c                                                                      c
c  xpp is the parameter to express the sensor altitude                 c
c                                                                      c
c         xpp= -1000  means that the sensor is a board a satellite     c
c         xpp=     0  means that the sensor is at the ground level     c
c                                                                      c
c                                                                      c
c     for aircraft simulations                                         c
c    -100< xpp <0  means you know the altitude of the sensor expressed c
c                  in kilometers units                                 c
C     this altitude is relative to the target altitude                 c
c                                                                      c
c     for aircraft simulations only, you have to give                  c
c	puw,po3   (water vapor content,ozone content between the       c
c                  aircraft and the surface)                           c
c	taerp     (the aerosol optical thickness at 550nm between the  c
c                  aircraft and the surface)                           c
c    if these data are not available, enter negative values for all    c
c    of them, puw,po3 will then be interpolated from the us62 standard c
C    profile according to the values at ground level. Taerp will be    c
c    computed according to a 2km exponential profile for aerosol.      c
c**********************************************************************/
struct AtmosModel;
struct AerosolConcentration;

struct Altitude
{
	double xps;
	double xpp;

	/* some vars */
	mutable double palt;
	double pps;
	int	  idatmp;
	double taer55p;
	double puw;
	double puo3;
	double ftray;

	double puwus;
	double puo3us;

	struct Plane_sim
	{
		double zpl[34];
		double ppl[34];
		double tpl[34];
		double whpl[34];
		double wopl[34];
	} plane_sim;

private:
    /* remember the original input values
     these values are set the first time when parse is called
     and used in subsequent calls to init to set xps and xpp */
    double original_xps;
    double original_xpp;
    double original_taer55p;
    double original_puw;
    double original_puo3;

	void pressure(AtmosModel& atms, double& uw, double& uo3);

	void presplane(AtmosModel& atms);

    /* Reads xps and xpp from the input file */
	void parse();

public:
	void print();

    /* Set the height to be used the next time init is called */
    void set_height(const double height) { original_xps = height; }
    /* call init only once: init parses input file */
    void init(AtmosModel& atms, const AerosolConcentration &aerocon);
    /* call update_hv whenever xps changes */
    void update_hv(AtmosModel& atms, const AerosolConcentration &aerocon);
    
	static Altitude Parse();
};


#endif /* ALTITUDE_H */
