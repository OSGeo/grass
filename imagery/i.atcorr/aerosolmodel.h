#ifndef AEROSOL_MODEL_H
#define AEROSOL_MODEL_H

/* **********************************************************************c */
/*                                                                      c */
/*       iaer       aerosol model(type)                                 c */
/*                  --------------                                      c */
/*                                                                      c */
/*                                                                      c */
/*  you select one of the following standard aerosol models:            c */
/*         0  no aerosols                                               c */
/*         1  continental model  )                                      c */
/*         2  maritime model     )  according to sra models             c */
/*         3  urban model        )                                      c */
/*         5  shettle model for background desert aerosol               c */
/*         6  biomass burning                                           c */
/*         7  stratospheric model                                       c */
/*                                                                      c */
/*  or you define your own model using basic components: iaer=4         c */
/*         4 enter the volumic percentage of each component             c */
/*                 c(1) = volumic % of dust-like                        c */
/*                 c(2) = volumic % of water-soluble                    c */
/*                 c(3) = volumic % of oceanic                          c */
/*                 c(4) = volumic % of soot                             c */
/*                   between 0 to 1                                     c */
/*                                                                      c */
/*  or you define your own model using size distribution function:      c */
/*         8  Multimodal Log Normal distribution (up to 4 modes)        c */
/*         9  Modified gamma  distribution                              c */
/*        10  Junge Power-Law distribution                              c */
/*                                                                      c */
/*  or you define a model using sun-photometer measurements:            c */
/*        11  Sun Photometer  distribution (50 values max)              c */
/*             you have to enter:  r and d V / d (logr)                 c */
/*                  where r is the radius (in micron) and V the volume  c */
/*                  and d V / d (logr) in (cm3/cm2/micron)              c */
/*             and then you have to enter: nr and ni for each wavelengthc */
/*                  where nr and ni are respectively the real and       c */
/*                  imaginary part of the refractive index              c */
/*                                                                      c */
/*  or you can use results computed and previously saved                c */
/*        12  Reading of data previously saved into FILE                c */
/*             you have to enter the identification name FILE in the    c */
/*             next line of inputs.                                     c */
/*                                                                      c */
/*                                                                      c */
/*  iaerp and FILE  aerosol model(type)-Printing of results             c */
/*                  ---------------------------------------             c */
/*                                                                      c */
/* For iaer=8,9,10,and 11:                                              c */
/*    results from the MIE subroutine may be saved into the file        c */
/*    FILE.mie (Extinction and scattering coefficients, single          c */
/*    scattering albedo, Asymmetry parameter, phase function at         c */
/*    predefined wavelengths) and then can be re-used with the          c */
/*    option iaer=12 where FILE is an identification name you           c */
/*    have to enter.                                                    c */
/*                                                                      c */
/*    So, if you select iaer=8,9,10,or 11, next line following the      c */
/*    requested inputs by the options 8,9,10, or 11 you have to enter   c */
/*    iaerp                                                             c */
/*                                                                      c */
/*        iaerp=0    results will not be saved                          c */
/*        iaerp=1    results will be saved into the file FILE.mie       c */
/*                    next line enter FILE                              c */
/*                                                                      c */
/*                                                                      c */
/*   example for iaer and iaerp                                         c */
/* 8                      Multimodal Log-Normale distribution selected  c */
/* 0.0001 100.0 3         Rmin, Rmax, 3 components                      c */
/* 0.5000 2.99 1.66E-7    Rmean, Sigma, percentage density-1st componentc */
/* 1.53 1.53 1.53 1.53 1.53 1.53 1.52 1.40 1.22 1.27  nr-10 wavelengths c */
/* .008 .008 .008 .008 .008 .008 .008 .008 .009 .011  ni-10 wavelengths c */
/* 0.0050 2.99 0.5945     Rmean, Sigma, percentage density-2nd componentc */
/* 1.53 1.53 1.53 1.53 1.53 1.53 1.52 1.51 1.42 1.452 nr-10 wavelengths c */
/* .005 .005 .005 .005 .006 .007 .012 .023 .010 .004  ni-10 wavelengths c */
/* 0.0118 2.00 0.4055     Rmean, Sigma, percentage density-3rd componentc */
/* 1.75 1.75 1.75 1.75 1.75 1.75 1.75 1.77 1.81 1.90  nr-10 wavelengths c */
/* .46  .45  .45  .44  .43  .43  .43  .46  .50  .57   ni-10 wavelengths c */
/* 1                      Results will be saved into FILE.mie           c */
/* URBAN-WCP112           Identification of the output file called FILE c */
/*                    -> results will be saved into URBAN-WCP112.mie    c */
/*                                                                      c */
/* **********************************************************************c */

struct AerosolModel
{
	long int iaer;	/* aerosol model */
	double c[4];

private:
	double nis;
	double sca[10];
	long int iaerp;

	/* methods */
	void aeroso(const double xmud);

	string filename;
	void load();
	void save();	/* .mie file */

	/* defined models' initilizations */
	void bdm();
	void bbm();
	void stm();
	void dust();
	void wate();
	void ocea();
	void soot();

	struct Mie_in
	{
		double rmax;
		double rmin;
		double rn[10][4];
		double ri[10][4];
		double x1[4];
		double x2[4];
		double x3[4];
		double cij[4];
		double rsunph[50];
		double nrsunph[50];

		long int icp;
		long int irsunph;
	};

	Mie_in mie_in;
	void mie(double (&ex)[4][10], double (&sc)[4][10], double (&asy)[4][10]);
	void exscphase(const double alpha, const double nr, 
				   const double ni, double& Qext, 
				   double& Qsca, double (&p11)[83]);

	void parse(const double xmud);

	/* format 132 */
	void print132(string s);
public:
	void print();
	static AerosolModel Parse(const double xmud);
};



#endif /* AEROSOL_MODEL_H */
