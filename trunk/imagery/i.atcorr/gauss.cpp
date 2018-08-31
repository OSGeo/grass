#include <cmath>
#include "common.h"
#include "gauss.h"

const long int mu2	= 48;

double Gauss::angmu[10] = { 85.f, 80.f, 70.f, 60.f, 50.f, 40.f, 30.f, 20.f, 10.f, 0.f };
double Gauss::angphi[13] = { 0.f, 30.f, 60.f, 90.f, 120.f, 150.f, 180.f, 210.f, 240.f, 270.f, 300.f, 330.f, 360.f };

/*  preliminary computations for gauss integration */
void Gauss::init()
{
    int j;   

    /* convert angphi and angmu to radians */
    for (j = 0; j < 13; ++j) angphi[j] = (double)(angphi[j] * M_PI / 180.f);
    for (j = 0; j < 10; ++j) angmu[j] =	 (double)cos(angmu[j] * M_PI / 180.f);

    /* calculate rm & gb */
	
    double anglem[mu2];
    double weightm[mu2];
    gauss (-1.f, 1.f, anglem, weightm, mu2);

    gb[STDI(-mu)]   = 0;
    gb[STDI(0)]     = 0;
    gb[STDI(mu)]    = 0;
    rm[STDI(-mu)]   = 0;
    rm[STDI(0)]     = 0;
    rm[STDI(mu)]    = 0;
    /* do shift into rm & gb */
    for (j = -mu+1; j <= -1; ++j)
    {
	rm[-j] = anglem[mu + j - 1];
	gb[-j] = weightm[mu + j - 1];
    }

    for (j = 1; j <= mu-1; ++j)
    {
	rm[2*mu - j] = anglem[mu + j - 2];
	gb[2*mu - j] = weightm[mu + j - 2];
    }

    /* calculate rp & gp */
    gauss (0.f, (double)2 * M_PI, rp, gp, np);
}


/*	Compute for a given n, the gaussian quadrature (the n gaussian angles and the
  their respective weights). The gaussian quadrature is used in numerical integration involving the
  cosine of emergent or incident direction zenith angle. */
void Gauss::gauss (double a, double b, double *x, double *w, long int n)
{
    int m = (n + 1) / 2;
    double xm = (b + a) / 2;
    double xl = (b - a) / 2;

    for(int i = 0; i < m; i++)
    {
	double 
	    z1, 
	    pp, 
	    z = cos(M_PI * (i + 0.75) / (n + 0.5));

	do {
	    double p1 = 1;
	    double p2 = 0;

	    for(int j = 0; j < n; j++)
	    {
		double p3 = p2;
		p2 = p1;
		p1 = ((2 * j + 1) * z * p2 - j * p3) / (j+1);
	    }

	    pp = n * (z * p1 - p2) / (z * z - 1);
	    z1 = z;
	    z = z1 - p1 / pp;
	} while(fabs(z - z1) > 3e-14);

	if (fabs(z) < 3e-14) z = 0;
	x[i] = (double)(xm - xl * z);
        x[n - 1 - i] = (double)(xm + xl * z);
	w[i] = (double)(2 * xl / ((1 - z * z) * pp * pp));
	w[n - 1 - i] = w[i];
    }
}
