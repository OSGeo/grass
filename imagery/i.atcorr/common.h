#ifndef MY_COMMON_H
#define MY_COMMON_H

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <iostream> /* ??? */
#include <fstream>
#include <string>
#include <cmath>
#include <limits>
#include <iomanip>

using std::string;

using std::ios;
using std::ifstream;
using std::ofstream;
using std::ostringstream;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::setprecision;
using std::setw;
using std::ends;

using std::numeric_limits;

#include "output.h"

/* or better #include <grass/gis.h> ? */
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif /* M_PI */
#define M_PI2 6.283185307179586476925286766559

const long int nt	= 26;

/* Constants */
const double sigma	= 0.056032f;
const double delta	= 0.0279f;
const double xacc	= 1.e-06f;
const double step	= 0.0025f;


/* Globals */
/* not sure what the name stands for */
struct Sixs_sos
{
	double phasel[10][83];
	double cgaus[83];
	double pdgs[83];
};

struct Sixs_aer
{
	double ext[10];
	double ome[10]; 
	double gasym[10]; 
	double phase[10];
};

struct Sixs_aerbas
{
	double bdm_ph[10][83];		/* background desert model... */
	double bbm_ph[10][83];		/* biomass burning model... */
	double stm_ph[10][83];		/* stratospherique aerosol model... */
	double dust_ph[10][83];		/* dust model */
	double wate_ph[10][83];		/* water model */
	double ocea_ph[10][83];		/* ocean model */
	double soot_ph[10][83];		/* soot model */

	double usr_ph[10][83];		/* user defined model from size distribution */
	double (*ph)[10][83];		/* pointer to current active model */
};

struct Sixs_trunc
{
	double pha[83];
	double betal[81];
};

struct Sixs_disc
{
	double roatm[3][10];
	double dtdir[3][10];
	double dtdif[3][10];
	double utdir[3][10];
	double utdif[3][10];
	double sphal[3][10];
	double wldis[10];
	double trayl[10];
    double traypl[10];
};

extern Sixs_sos sixs_sos;
extern Sixs_aer sixs_aer;
extern Sixs_aerbas sixs_aerbas;
extern Sixs_trunc sixs_trunc;
extern Sixs_disc sixs_disc;

#endif /* MY_COMMON_H */
