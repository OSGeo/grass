#ifndef ATMOSPHERIC_MODEL_H
#define ATMOSPHERIC_MODEL_H

/* **********************************************************************c */
/*       idatm      atmospheric model                                   c */
/*                 --------------------                                 c */
/*                                                                      c */
/*                                                                      c */
/*  you select one of the following standard atmosphere: idatm=0 to 6   c */
/*         0    no gaseous absorption                                   c */
/*         1    tropical                )                               c */
/*         2    midlatitude summer      )                               c */
/*         3    midlatitude winter      )                               c */
/*         4    subarctic summer        )      from lowtran             c */
/*         5    subarctic winter        )                               c */
/*         6    us standard 62          )                               c */
/*                                                                      c */
/*  or you define your own atmospheric model idatm=7 or 8               c */
/*         7    user profile  (radiosonde data on 34 levels)            c */
/*              enter altitude       (  in km )                         c */
/*                    pressure       (  in mb )                         c */
/*                    temperature    (  in k  )                         c */
/*                    h2o density    (in  g/m3)                         c */
/*                    o3  density    (in  g/m3)                         c */
/*                                                                      c */
/*           for example, altitudes are  from  0 to 25km step of 1km    c */
/*                        from 25 to 50km step of 5km                   c */
/*                        and two values at 70km and 100km              c */
/*                        so you have 34*5 values to input.             c */
/*         8    enter water vapor and ozone contents                    c */
/*                 uw  (in  g/cm2 )                                     c */
/*                 uo3 (in  cm-atm)                                     c */
/*                 profil is taken from us62                            c */
/*                                                                      c */
/* **********************************************************************c */

struct AtmosModel
{
	long int idatm;	/* atmospheric model*/

	/* secondary */
    double uw;
    double uo3;

	/* primary */
	double z[34];
	double p[34];
	double t[34];
	double wh[34];
	double wo[34];

private:
	/* methods to initialize each model */
	void us62();
	void tropic();
	void midsum();
	void midwin();
	void subsum();
	void subwin();

	void parse();

public:
	void print();
	static AtmosModel Parse();
};

#endif /* ATMOSPHERIC_MODEL_H */
