#ifndef MY_COMMON_H
#define MY_COMMON_H

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <limits>
#include <iomanip>
#include "Output.h"
using namespace std;

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif /* M_PI */
#define M_PI2 6.283185307179586476925286766559
#define MIN(X,Y) ((X) <= (Y) ? (X) : (Y))
#define ROUND(X) ((X) - (int)(X) < 0.5 ? (int)(X) : (int)((X)+1))

const long int nt	= 26;

/* Constants */
const float sigma	= 0.056032f;
const float delta	= 0.0279f;
const float xacc	= 1.e-06f;
const float step	= 0.0025f;


/* Globals */
/* not sure what the name stands for */
struct Sixs_sos
{
	float phasel[10][83];
	float cgaus[83];
	float pdgs[83];
};

struct Sixs_aer
{
	float ext[10];
	float ome[10]; 
	float gasym[10]; 
	float phase[10];
};

struct Sixs_aerbas
{
	float bdm_ph[10][83];		/* background desert model... */
	float bbm_ph[10][83];		/* biomass burning model... */
	float stm_ph[10][83];		/* stratospherique aerosol model... */
	float dust_ph[10][83];		/* dust model */
	float wate_ph[10][83];		/* water model */
	float ocea_ph[10][83];		/* ocean model */
	float soot_ph[10][83];		/* soot model */

	float usr_ph[10][83];		/* user defined model from size distribution */
	float (*ph)[10][83];		/* pointer to current active model */
};

struct Sixs_trunc
{
	float pha[83];
	float betal[81];
};

struct Sixs_disc
{
	float roatm[3][10];
	float dtdir[3][10];
	float dtdif[3][10];
	float utdir[3][10];
	float utdif[3][10];
	float sphal[3][10];
	float wldis[10];
	float trayl[10];
    float traypl[10];
};

extern Sixs_sos sixs_sos;
extern Sixs_aer sixs_aer;
extern Sixs_aerbas sixs_aerbas;
extern Sixs_trunc sixs_trunc;
extern Sixs_disc sixs_disc;

#endif /* MY_COMMON_H */
