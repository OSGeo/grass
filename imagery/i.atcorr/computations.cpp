#include <cstring>

extern "C" {
#include <grass/gis.h>
#include <grass/glocale.h>
}

#include "common.h"
#include "GeomCond.h"
#include "AtmosModel.h"
#include "AerosolModel.h"
#include "AerosolConcentration.h"
#include "Altitude.h"
#include "Iwave.h"
#include "Gauss.h"
#include "Transform.h"
#ifdef WIN32
#pragma warning (disable : 4305)
#endif

struct OpticalAtmosProperties
{
    float rorayl, romix, roaero;
    float ddirtr, ddiftr;
    float ddirtt, ddiftt;
    float ddirta, ddifta;
    float udirtr, udiftr;
    float udirtt, udiftt;
    float udirta, udifta;
    float sphalbr, sphalbt, sphalba;
};

/* To compute the molecular optical depth as a function of wavelength for any
   atmosphere defined by the pressure and temperature profiles. */
float odrayl(const AtmosModel &atms, const float wl)
{
    /* air refraction index edlen 1966 / metrologia,2,71-80  putting pw=0 */
    float ak=1/wl;
    double awl= wl*wl*wl*wl;
    double a1 = 130 - ak * ak;
    double a2 = 38.9 - ak * ak;
    double a3 = 2406030 / a1;
    double a4 = 15997 / a2;
    double an = (8342.13 + a3 + a4) * 1.0e-08 + 1;
    double a = (24 * M_PI * M_PI * M_PI) * ((an * an - 1) * (an * an - 1))
	* (6 + 3 * delta) / (6 - 7 * delta) / ((an * an + 2) * (an * an + 2));

    float tray = 0;
    for(int k = 0; k < 33; k++)
    {
	double dppt = (288.15 / 1013.25) * (atms.p[k] / atms.t[k] + atms.p[k+1] / atms.t[k+1]) / 2;
	double sr = a * dppt / awl / 0.0254743;
	tray += (float)((atms.z[k+1] - atms.z[k]) * sr);
    }

    return tray;
}

/*
  decompose the aerosol phase function in series of Legendre polynomial used in
  OS.f and ISO.f and compute truncation coefficient f to modify aerosol optical thickness t and single
  scattering albedo w0 according to:
  t' = (1-w0 f) t

  w0' =  w0 (1- f)
  --------
  (1-w0 f)
*/
float trunca()
{
    float ptemp[83];
    float cosang[80];
    float weight[80];
    float rmu[83];
    float ga[83];

    int i;
    for(i = 0; i < 83; i++) ptemp[i] = sixs_trunc.pha[i];

    Gauss::gauss(-1,1,cosang,weight,80);

    for(i = 0; i < 40; i++)
    {
	rmu[i+1] = cosang[i];
	ga[i+1] = weight[i];
    }

    rmu[0] = -1;
    ga[0] = 0;
    rmu[41] = 0;
    ga[41] = 0;

    for(i = 40; i < 80; i++)
    {
	rmu[i+2] = cosang[i];
	ga[i+2] = weight[i];
    }

    rmu[82] = 1;
    ga[82] = 0;

    int k = 0;
    for(i = 0; i < 83; i++)
    {
	if(rmu[i] > 0.8) break;
	k = i - 1;
    }

    int kk = 0;
    for(i = 0; i < 83; i++)
    {
	if(rmu[i] > 0.94) break;
	kk = i - 1;
    }
	

    float aa = (float)((log10(sixs_trunc.pha[kk]) - log10(sixs_trunc.pha[k])) / 
		       (acos(rmu[kk]) - acos(rmu[k])));
    float x1 = (float)(log10(sixs_trunc.pha[kk]));
    float x2 = (float)acos(rmu[kk]);

    for(i = kk + 1; i < 83; i++)
    {
	double a;
	if(fabs(rmu[i] - 1) <= 1e-08) a = x1 - aa * x2;
	else a = x1 + aa * (acos(rmu[i]) - x2);
	ptemp[i] = (float)pow(10,a);
    }


    for(i = 0; i < 83; i++) sixs_trunc.pha[i] = ptemp[i];
    for(i = 0; i < 80; i++) sixs_trunc.betal[i] = 0;

    float pl[83];

#define IPL(X) ((X)+1)


    for(i = 0; i < 83; i++)
    {
	float x = sixs_trunc.pha[i] * ga[i];
	float rm = rmu[i];
	pl[IPL(-1)] = 0;
	pl[IPL(0)] = 1;

	for(int k = 0; k <= 80; k++)
	{
	    pl[IPL(k+1)] = ((2 * k + 1) * rm * pl[IPL(k)] - k * pl[IPL(k-1)]) / (k + 1);
	    sixs_trunc.betal[k] += x * pl[IPL(k)];
	}
    }

    for(i = 0; i <= 80; i++) sixs_trunc.betal[i] *= (2 * i + 1) * 0.5f;

    float z1 = sixs_trunc.betal[0];
    for(i = 0; i <= 80; i++) sixs_trunc.betal[i] /= z1;
    if(sixs_trunc.betal[80] < 0) sixs_trunc.betal[80] = 0;

    return 1 - z1;
}


/*
  Decompose the atmosphere in a finite number of layers. For each layer, DISCRE
  provides the optical thickness, the proportion of molecules and aerosols assuming an exponential
  distribution for each constituants. Figure 1 illustrate the way molecules and aerosols are mixed in a
  realistic atmosphere. For molecules, the scale height is 8km. For aerosols it is assumed to be 2km
  unless otherwise specified by the user (using aircraft measurements).
*/

float discre(const float ta, const float ha, const float tr, const float hr,
	     const int it, const int nt, const float yy, const float dd,
	     const float ppp2, const float ppp1)
{
    if( ha >= 7 ) 
    {
	G_warning(_("Check aerosol measurements or plane altitude"));
	return 0;
    }

    double dt;
    if( it == 0 ) dt = 1e-17;
    else dt = 2 * (ta + tr - yy) / (nt - it + 1);
	
    float zx; /* return value */
    float ecart = 0;
    do { 
	dt = dt / 2;
	double ti = yy + dt;
	float y1 = ppp2;
	float y2;
	float y3 = ppp1;

	while(true)
	{
	    y2 = (y1 + y3) * 0.5f;

	    double xx = -y2 / ha;
	    double x2;
	    if (xx < -18) x2 = tr * exp(-y2 / hr);
	    else x2 = ta * exp(xx) + tr * exp(-y2 / hr);

	    if(fabs(ti - x2) < 0.00001) break;

	    if(ti - x2 < 0) y3 = y2;
	    else y1 = y2;
	}

	zx = y2;
	float delta = (float)(1. / (1 + ta * hr / tr / ha * exp((zx - ppp1) * (1. / hr - 1. / ha))));
	if(dd != 0) ecart = (float)fabs((dd - delta) / dd);
    } while((ecart > 0.75) && (it != 0));
    return zx;

}


/* indexing macro for the psl variable */
#define PSI(X) ((X)+1)
/*
  Compute the values of Legendre polynomials used in the successive order of
  scattering method.
*/
void kernel(const int is, float (&xpl)[2*mu + 1], float (&bp)[26][2*mu + 1], Gauss &gauss)
{
    const double rac3 = 1.7320508075688772935274463415059;
#define PSI(X) ((X)+1)
    float psl[82][2*mu + 1];

    if(is == 0) 
    {
	for(int j = 0; j <= mu; j++)
	{
	    psl[PSI(0)][STDI(-j)] = 1;
	    psl[PSI(0)][STDI(j)] = 1;
	    psl[PSI(1)][STDI(j)] = gauss.rm[STDI(j)];
	    psl[PSI(1)][STDI(-j)] = -gauss.rm[STDI(j)];

	    double xdb = (3 * gauss.rm[STDI(j)] * gauss.rm[STDI(j)] - 1) * 0.5;
	    if(fabs(xdb) < 1e-30) xdb = 0;
	    psl[PSI(2)][STDI(-j)] = (float)xdb;
	    psl[PSI(2)][STDI(j)] = (float)xdb;
	}
	psl[PSI(1)][STDI(0)] = gauss.rm[STDI(0)];
    }
    else if(is == 1)
    {
	for(int j = 0; j <= mu; j++)
	{
	    double x = 1 - gauss.rm[STDI(j)] * gauss.rm[STDI(j)];
	    psl[PSI(0)][STDI(j)]  = 0;
	    psl[PSI(0)][STDI(-j)] = 0;
	    psl[PSI(1)][STDI(-j)] = (float)sqrt(x * 0.5);
	    psl[PSI(1)][STDI(j)]  = (float)sqrt(x * 0.5);
	    psl[PSI(2)][STDI(j)]  = (float)(gauss.rm[STDI(j)] * psl[PSI(1)][STDI(j)] * rac3);
	    psl[PSI(2)][STDI(-j)] = -psl[PSI(2)][STDI(j)];

	}
	psl[PSI(2)][STDI(0)] = -psl[PSI(2)][STDI(0)];
    }
    else
    {
	double a = 1;
	for(int i = 1; i <= is; i++) a *= sqrt((double)(i + is) / (double)i) * 0.5;
/*		double b = a * sqrt((double)is / (is + 1.)) * sqrt((is - 1.) / (is + 2.));*/

	for(int j = 0; j <= mu; j++)
	{
	    double xx = 1 - gauss.rm[STDI(j)] * gauss.rm[STDI(j)];
	    psl[PSI(is - 1)][STDI(j)] = 0;
	    double xdb = a * pow(xx, is * 0.5);
	    if(fabs(xdb) < 1e-30) xdb = 0;
	    psl[PSI(is)][STDI(-j)] = (float)xdb;
	    psl[PSI(is)][STDI(j)] = (float)xdb;
	}
    }

    int k = 2;
    int ip = 80;

    if(is > 2) k = is;
    if(k != ip)
    {
	int ig = -1;
	if( is == 1 ) ig = 1;
	for(int l = k; l < ip; l++)
	{
	    double a = (2 * l + 1.) / sqrt((l + is + 1.) * (l - is + 1.));
	    double b = sqrt(float((l + is) * (l - is))) / (2. * l + 1.);

	    for(int j = 0; j <= mu; j++)
	    {
		double xdb = a * (gauss.rm[STDI(j)] * psl[PSI(l)][STDI(j)] - b * psl[PSI(l-1)][STDI(j)]);
		if (fabs(xdb) < 1e-30) xdb = 0;
		psl[PSI(l+1)][STDI(j)] = (float)xdb;
		if(j != 0) psl[PSI(l+1)][STDI(-j)] = ig * psl[PSI(l+1)][STDI(j)];
	    }
	    ig = -ig;
	}
    }

    int j;
    for(j = -mu; j <= mu; j++) xpl[STDI(j)] = psl[PSI(2)][STDI(j)];
	
    for(j = 0; j <= mu; j++)
    {
	for(int k = -mu; k <= mu; k++)
	{
	    double sbp = 0;
	    if(is <= 80) 
	    {
		for(int l = is; l <= 80; l++)
		    sbp += psl[PSI(l)][STDI(j)] * psl[PSI(l)][STDI(k)] * sixs_trunc.betal[l];

		if(fabs(sbp) < 1e-30) sbp = 0;
		bp[j][STDI(k)] = (float)sbp;
	    }
	}
    }

}



#define accu 1e-20
#define accu2 1e-3
#define mum1 (mu - 1)

void os(const float tamoy, const float trmoy, const float pizmoy, 
	const float tamoyp, const float trmoyp,	float (&xl)[2*mu + 1][np],
        Gauss &gauss, const Altitude &alt, const GeomCond &geom)
{
    float trp = trmoy - trmoyp;
    float tap = tamoy - tamoyp;
    int iplane = 0;

    /* if plane observations recompute scale height for aerosol knowing:
       the aerosol optical depth as measure from the plane 	= tamoyp
       the rayleigh scale   height = 			= hr (8km)
       the rayleigh optical depth  at plane level 		= trmoyp
       the altitude of the plane 				= palt
       the rayleigh optical depth for total atmos		= trmoy
       the aerosol  optical depth for total atmos		= tamoy
       if not plane observations then ha is equal to 2.0km
       ntp local variable: if ntp=nt     no plane observation selected
       ntp=nt-1   plane observation selected
       it's a mixing rayleigh+aerosol */

    float ha = 2;
    int snt = nt;
    int ntp = snt;
    if(alt.palt <= 900 && alt.palt > 0)
    {
	if(tap > 1.e-03) ha = -alt.palt / (float)log(tap / tamoy);
	ntp = snt - 1;
    } 

    float xmus = -gauss.rm[STDI(0)];

    /* compute mixing rayleigh, aerosol
       case 1: pure rayleigh
       case 2: pure aerosol
       case 3: mixing rayleigh-aerosol */

    float h[31];
    memset(h, 0, sizeof(h));
    float ch[31];
    float ydel[31];

    float xdel[31];
    float altc[31];
    if( (tamoy <= accu2) && (trmoy > tamoy) ) 
    {
	for(int j = 0; j <= ntp; j++)
	{
	    h[j] = j * trmoy / ntp;
	    ch[j]= (float)exp(-h[j] / xmus) / 2;
	    ydel[j] = 1;
	    xdel[j] = 0;

	    if (j == 0) altc[j] = 300;
	    else altc[j] = -(float)log(h[j] / trmoy) * 8;
	}
    }


    if( (trmoy <= accu2) && (tamoy > trmoy) )
    {
	for(int j = 0; j <= ntp; j++)
	{
	    h[j] = j * tamoy / ntp;
	    ch[j]= (float)exp(-h[j] / xmus) / 2;
	    ydel[j] = 0;
	    xdel[j] = pizmoy;
      
	    if (j == 0) altc[j] = 300;
	    else altc[j] = -(float)log(h[j] / tamoy) * ha;
	}
    }

    if(trmoy > accu2 && tamoy > accu2)
    {
	ydel[0] = 1;
	xdel[0] = 0;
	h[0] = 0;
	ch[0] = 0.5;
	altc[0] = 300;
	float zx = 300;
	iplane = 0;

	for(int it = 0; it <= ntp; it++)
	{
	    if(it == 0) zx = discre(tamoy, ha, trmoy, 8.0, it, ntp, 0, 0, 300, 0);
	    else zx = discre(tamoy, ha, trmoy, 8.0, it, ntp, h[it - 1], ydel[it - 1], 300, 0);

	    double xx = -zx / ha;
	    float ca;
	    if( xx <= -20 ) ca = 0;
	    else ca = tamoy * (float)exp(xx);

	    xx = -zx / 8;
	    float cr = trmoy * (float)exp(xx);
	    h[it] = cr + ca;


	    altc[it] = zx;
	    ch[it] = (float)exp(-h[it] / xmus) / 2;
	    cr = cr / 8;
	    ca = ca / ha;
	    float ratio = cr / (cr + ca);
	    xdel[it] = (1 - ratio) * pizmoy;
	    ydel[it] = ratio;
	}
    }

    /* update plane layer if necessary */
    if (ntp == (snt - 1)) 
    {
	/* compute position of the plane layer */
        float taup = tap + trp;
        iplane = -1;
	for(int i = 0; i <= ntp; i++) if (taup >= h[i]) iplane = i;

	/* update the layer from the end to the position to update if necessary */
	float xt1 = (float)fabs(h[iplane] - taup);
        float xt2 = (float)fabs(h[iplane+1] - taup);

        if ((xt1 > 0.0005) && (xt2 > 0.0005))
	{
	    for(int i = snt; i >= iplane+1; i--)
	    {
		xdel[i] = xdel[i-1];
		ydel[i] = ydel[i-1];
		h[i] = h[i-1];
		altc[i] = altc[i-1];
		ch[i] = ch[i-1];
	    }
	}
        else
	{
	    snt = ntp;
	    if (xt2 > xt1) iplane++;

	}
		
	h[iplane] = taup;
	if ( trmoy > accu2 && tamoy > accu2 )
	{

	    float ca = tamoy * (float)exp(-alt.palt / ha);
	    float cr = trmoy * (float)exp(-alt.palt / 8);
	    h[iplane] = ca + cr;
	    cr = cr / 8;
	    ca = ca / ha;
	    float ratio = cr / (cr + ca);
	    xdel[iplane] = (1 - ratio) * pizmoy;
	    ydel[iplane] = ratio;
	    altc[iplane] = alt.palt;
	    ch[iplane] = (float)exp(-h[iplane] / xmus) / 2;
	}

	if ( trmoy > accu2 && tamoy <= accu2 )
	{
	    ydel[iplane] = 1;
	    xdel[iplane] = 0;
	    altc[iplane] = alt.palt;
	}
	        
	if ( trmoy <= accu2 && tamoy > accu2 ) 
	{
	    ydel[iplane] = 0;
	    xdel[iplane] = pizmoy;
	    altc[iplane] = alt.palt;
	}
    }

	
    float phi = (float)geom.phirad;
    int i;
    for(i = 0; i < np; i++) for(int m = -mu; m <= mu; m++) xl[STDI(m)][i] = 0;

    /* ************ incident angle mus ******* */

    float aaaa = delta / (2 - delta);
    float ron = (1 - aaaa) / (1 + 2 * aaaa);

    /* rayleigh phase function */

    float beta0 = 1;
    float beta2 = 0.5f * ron;

    /* fourier decomposition */
    float i1[31][2*mu + 1];
    float i2[31][2*mu + 1];
    float i3[2*mu + 1];
    float i4[2*mu + 1];
    float in[2*mu + 1];
    float inm1[2*mu + 1];
    float inm2[2*mu + 1];
    for(i = -mu; i <= mu; i++) i4[STDI(i)] = 0;

    int iborm = 80;
    if(fabs(xmus - 1.000000) < 1e-06) iborm = 0;
   
    for(int is = 0; is <= iborm; is++)
    {
	/* primary scattering */
	int ig = 1;
	float roavion0 = 0;
	float roavion1 = 0;
	float roavion2 = 0;
	float roavion = 0;

	int j;
	for(j = -mu; j <= mu; j++) i3[STDI(j)] = 0;

	/* kernel computations */
	float xpl[2*mu + 1];
	float bp[26][2*mu + 1];
	memset(xpl, 0, sizeof(float)*(2*mu+1));
	memset(bp, 0, sizeof(float)*26*(2*mu+1));
	
	kernel(is,xpl,bp,gauss);

	if(is > 0) beta0 = 0;

	for(j = -mu; j <= mu; j++)
	{
	    float sa1;
	    float sa2;

	    if((is - 2) <= 0)
	    {
		float spl = xpl[STDI(0)];
		sa1 = beta0 + beta2 * xpl[STDI(j)] * spl;
		sa2 = bp[0][STDI(j)];
	    } 
	    else 
	    {
		sa2 = bp[0][STDI(j)];
		sa1 = 0;
	    }
	    /* primary scattering source function at every level within the layer */

	    for(int k = 0; k <= snt; k++)
	    {
		float c = ch[k];
		double a = ydel[k];
		double b = xdel[k];
		i2[k][STDI(j)] = (float)(c * (sa2 * b + sa1 * a));
	    }
	}
	  
	int k;
	/* vertical integration, primary upward radiation */
	for(k = 1; k <= mu; k++)
	{
	    i1[snt][STDI(k)] = 0;
	    float zi1 = i1[snt][STDI(k)];

	    for(int i = snt - 1; i >= 0; i--)
	    {
		float f = h[i + 1] - h[i];
		double a = (i2[i + 1][STDI(k)] - i2[i][STDI(k)]) / f;
		double b = i2[i][STDI(k)] - a * h[i];
		float c = (float)exp(-f / gauss.rm[STDI(k)]);

		double xx = h[i] - h[i + 1] * c;
		zi1 = (float)(c * zi1 + ((1 - c) * (b + a * gauss.rm[STDI(k)]) + a * xx) / 2);
		i1[i][STDI(k)] = zi1;
	    }
	}

	/* vertical integration, primary downward radiation */
	for(k = -mu; k <= -1; k++)
	{
	    i1[0][STDI(k)] = 0;
	    float zi1 = i1[0][STDI(k)];
      
	    for(int i = 1; i <= snt; i++)
	    {
		float f = h[i] - h[i - 1];
		float c = (float)exp(f / gauss.rm[STDI(k)]);
		double a = (i2[i][STDI(k)] -i2[i - 1][STDI(k)]) / f;
		double b = i2[i][STDI(k)] - a * h[i];
		double xx = h[i] - h[i - 1] * c;
		zi1 = (float)(c * zi1 + ((1 - c) * (b + a * gauss.rm[STDI(k)]) + a * xx)/ 2);
		i1[i][STDI(k)] = zi1;
	    }
	}

	/* inm2 is inialized with scattering computed at n-2
	   i3 is inialized with primary scattering */
	for(k = -mu; k <= mu; k++)
	{
	    if(k < 0) 
	    {
		inm1[STDI(k)] = i1[snt][STDI(k)];
		inm2[STDI(k)] = i1[snt][STDI(k)];
		i3[STDI(k)] = i1[snt][STDI(k)];
	    }
	    else if(k > 0) 
	    {
		inm1[STDI(k)] = i1[0][STDI(k)];
		inm2[STDI(k)] = i1[0][STDI(k)];
		i3[STDI(k)] = i1[0][STDI(k)];
	    }
	}
	roavion2 = i1[iplane][STDI(mu)];
	roavion = i1[iplane][STDI(mu)];

        do
	{
	    /* loop on successive order */
	    ig++;
		
	    /* successive orders
	       multiple scattering source function at every level within the laye
	       if is < ou = 2 kernels are a mixing of aerosols and molecules kern
	       if is >2 aerosols kernels only */

	    if(is - 2 <= 0)
	    {
		for(int k = 1; k <= mu; k++)
		{     
		    for(int i = 0; i <= snt; i++)
		    {
			double ii1 = 0;
			double ii2 = 0;

			for(int j = 1; j <= mu; j++)
			{
			    double bpjk = bp[j][STDI(k)] * xdel[i] + ydel[i] * (beta0 + beta2 * xpl[STDI(j)] * xpl[STDI(k)]);
			    double bpjmk = bp[j][STDI(-k)] * xdel[i] + ydel[i] * (beta0 + beta2 * xpl[STDI(j)] * xpl[STDI(-k)]);
			    double xdb = gauss.gb[STDI(j)] * (i1[i][STDI(j)] * bpjk + i1[i][STDI(-j)] * bpjmk);
			    ii2 += xdb;
			    xdb = gauss.gb[STDI(j)] * (i1[i][STDI(j)] * bpjmk + i1[i][STDI(-j)] * bpjk);
			    ii1 += xdb;
			}
					
			if (ii2 < 1e-30) ii2 = 0;
			if (ii1 < 1e-30) ii1 = 0;
			i2[i][STDI(k)] = (float)ii2;
			i2[i][STDI(-k)]= (float)ii1;
		    }
		}
	    } 
	    else
	    {
		for(int k = 1; k <= mu; k++)
		{
		    double ii1;
		    double ii2;

		    for(int i = 0; i <= snt; i++)
		    {
			ii1 = 0;
			ii2 = 0;

			for(int j = 1; j <= mu; j++)
			{
			    double bpjk = bp[j][STDI(k)] * xdel[i];
			    double bpjmk = bp[j][STDI(-k)] * xdel[i];
			    double xdb = gauss.gb[STDI(j)] * (i1[i][STDI(j)] * bpjk + i1[i][STDI(-j)] * bpjmk);
			    ii2 += xdb;
			    xdb = gauss.gb[STDI(j)] * (i1[i][STDI(j)] * bpjmk + i1[i][STDI(-j)] * bpjk);
			    ii1 += xdb;
			}

			if (ii2 < 1e-30) ii2 = 0;
			if (ii1 < 1e-30) ii1 = 0;
			i2[i][STDI(k)]  = (float)ii2;
			i2[i][STDI(-k)] = (float)ii1;
		    }
		}
	    }

			
	    /* vertical integration, upward radiation */
	    int k;
	    for(k = 1; k <= mu; k++)
	    {
		i1[snt][STDI(k)] = 0;
		float zi1 = i1[snt][STDI(k)];

		for(int i = snt-1; i >= 0; i--)
		{
		    float f = h[i + 1] - h[i];
		    double a = (i2[i + 1][STDI(k)] - i2[i][STDI(k)]) / f;
		    double b = i2[i][STDI(k)] - a * h[i];
		    float c = (float)exp(-f / gauss.rm[STDI(k)]);
		    double xx = h[i] - h[i + 1] * c;
		    zi1 = (float)(c * zi1 + ((1 - c) * (b + a * gauss.rm[STDI(k)]) + a * xx) / 2);
		    if (fabs(zi1) <= 1e-20) zi1 = 0;
		    i1[i][STDI(k)] = zi1;
		}
	    }

	    /* vertical integration, downward radiation */
	    for(k = -mu; k <= -1; k++)
	    {
		i1[0][STDI(k)] = 0;
		float zi1 = i1[0][STDI(k)];

		for(int i = 1; i <= snt; i++)
		{
		    float f = h[i] - h[i - 1];
		    float c = (float)exp(f / gauss.rm[STDI(k)]);
		    double a = (i2[i][STDI(k)] - i2[i - 1][STDI(k)]) / f;
		    double b = i2[i][STDI(k)] - a * h[i];
		    double xx = h[i] - h[i - 1] * c;
		    zi1 = (float)(c * zi1 + ((1 - c) * (b + a * gauss.rm[STDI(k)]) + a * xx) / 2);

		    if (fabs(zi1) <= 1e-20) zi1 = 0;
		    i1[i][STDI(k)] = zi1;
		}
	    }

	    /* in is the nieme scattering order */
	    for(k = -mu; k <= mu; k++)
	    {
		if(k < 0) in[STDI(k)] = i1[snt][STDI(k)];
		else if(k > 0) in[STDI(k)] = i1[0][STDI(k)];
	    }
	    roavion0 = i1[iplane][STDI(mu)];

	    /*  convergence test (geometrical serie) */
	    if(ig > 2)
	    {
		float a1 = roavion2;
		float d1 = roavion1;

		float g1 = roavion0;


		double z = 0;
		if(a1 >= accu && d1 >= accu && roavion >= accu)
		{
		    double y = ((g1 / d1 - d1 / a1) / ((1 - g1 / d1) * (1 - g1 / d1)) * (g1 / roavion));
		    y = fabs(y);
		    z = y > z ? y : z;
		}

		for(int l = -mu; l <= mu; l++)
		{
		    if (l == 0) continue;
		    a1 = inm2[STDI(l)];
		    d1 = inm1[STDI(l)];
		    g1 = in[STDI(l)];
		    if(a1 <= accu) continue;
		    if(d1 <= accu) continue;
		    if(i3[STDI(l)] <= accu) continue;
      
		    double y = ((g1 / d1 - d1 / a1) / ((1 - g1 / d1) * (1 - g1 / d1)) * (g1 / i3[STDI(l)]));
		    y = fabs(y);
		    z = y > z ? y : z;
		}

		if(z < 0.0001)
		{
		    /* successful test (geometrical serie) */
		    float y1;

		    for(int l = -mu; l <= mu; l++)
		    {
			y1 = 1;
			d1 = inm1[STDI(l)];
			g1 = in[STDI(l)];
			if(d1 <= accu) continue;
			y1 = 1 - g1 / d1;
			if(fabs(g1 - d1) <= accu) continue;
			g1 /= y1;
			i3[STDI(l)] += g1;
		    }


		    d1 = roavion1;
		    g1 = roavion0;
		    y1 = 1;
		    if(d1 >= accu)
		    {
			if(fabs(g1 - d1) >= accu)
			{
			    y1 = 1 - g1 / d1;
			    g1 /= y1;
			}

			roavion += g1;
		    }

		    break;	/* break out of the while loop */
		}

		/* inm2 is the (n-2)ieme scattering order */
		for(int k = -mu; k <= mu; k++) inm2[STDI(k)] = inm1[STDI(k)];
		roavion2 = roavion1;
	    }

	    /* inm1 is the (n-1)ieme scattering order */
	    for(k = -mu; k <= mu; k++) inm1[STDI(k)] = in[STDI(k)];
	    roavion1 = roavion0;

	    /* sum of the n-1 orders */
	    for(k = -mu; k <= mu; k++) i3[STDI(k)] += in[STDI(k)];
	    roavion += roavion0;

	    /* stop if order n is less than 1% of the sum */
	    double z = 0;
	    for(k = -mu; k <= mu; k++)
	    {
		if (fabs(i3[STDI(k)]) >= accu)
		{
		    double y = fabs(in[STDI(k)] / i3[STDI(k)]);
		    z = z >= y ? z : y;
		}
	    }
	    if(z < 0.00001) break;

	} while( ig <= 20 );	/* stop if order n is greater than 20 in any case */
        
	/* sum of the fourier component s */
	float delta0s = 1;
	if(is != 0) delta0s = 2;
	for(k = -mu; k <= mu; k++) i4[STDI(k)] += delta0s * i3[STDI(k)];

	/* stop of the fourier decomposition */
	int l;
	for(l = 0; l < np; l++)
	{
	    phi = gauss.rp[l];

	    for(int m = -mum1; m <= mum1; m++)
	    {
		if(m > 0) xl[STDI(m)][l] += (float)(delta0s * i3[STDI(m)] * cos(is * (phi + M_PI)));
		else xl[STDI(m)][l] += (float)(delta0s * i3[STDI(m)] * cos(is * phi));
	    }
	}

	if(is == 0)
	    for(int k = 1; k <= mum1; k++) xl[STDI(0)][0] += gauss.rm[STDI(k)] * gauss.gb[STDI(k)] * i3[STDI(-k)];
    
	xl[STDI(mu)][0] += (float)(delta0s * i3[STDI(mu)] * cos(is * (geom.phirad + M_PI)));
	xl[STDI(-mu)][0] += (float)(delta0s * roavion * cos(is * (geom.phirad + M_PI)));

	double z = 0;
	for(l = -mu; l <= mu; l++)
	{
            if(l == 0) continue;
	    if (fabs(i4[STDI(l)]) > accu) continue;
	    double x = fabs(i3[STDI(l)] / i4[STDI(l)]);
	    z = z > x ? z : x;
	}

	if(z <= 0.001) break;
    }
}


/*
  Compute the atmospheric transmission for either a satellite or aircraft observation
  as well as the spherical albedo of the atmosphere.
*/
void iso(const float tamoy, const float trmoy, const float pizmoy, 
	 const float tamoyp, const float trmoyp, float (&xf)[3],
         Gauss &gauss, const Altitude &alt)
{
    /* molecular ratio within the layer
       computations are performed assuming a scale of 8km for
       molecules and 2km for aerosols */

    /* the optical thickness above plane are recomputed to give o.t above pla */
    float trp = trmoy - trmoyp;
    float tap = tamoy - tamoyp;

    /* if plane observations recompute scale height for aerosol knowing:
       the aerosol optical depth as measure from the plane 	= tamoyp
       the rayleigh scale   height = 			= hr (8km)
       the rayleigh optical depth  at plane level 		= trmoyp
       the altitude of the plane 				= palt
       the rayleigh optical depth for total atmos		= trmoy
       the aerosol  optical depth for total atmos		= tamoy
       if not plane observations then ha is equal to 2.0km
       sntp local variable: if sntp=snt     no plane observation selected
       sntp=snt-1   plane observation selected */
	
    /* it's a mixing rayleigh+aerosol */
    int snt = nt;
    int iplane = 0;
    int ntp = snt;
    float ha = 2.0;
    if(alt.palt <= 900. && alt.palt > 0.0)
    {
	if (tap > 1.e-03) ha = (float)(-alt.palt / log(tap / tamoy));
        else ha = 2.;
	ntp = snt - 1;
    } 

    /* compute mixing rayleigh, aerosol
       case 1: pure rayleigh
       case 2: pure aerosol
       case 3: mixing rayleigh-aerosol */

    float h[31];
    float ydel[31];
    float xdel[31];
    float altc[31];

    if((tamoy <= accu2) && (trmoy > tamoy)) 
    {
	for(int j = 0; j <= ntp; j++)
	{
	    h[j] = j * trmoy / ntp;
	    ydel[j] = 1.0;
	    xdel[j] = 0.0;
	}
    }

    if((trmoy <= accu2) && (tamoy > trmoy)) 
    {
	for(int j = 0; j <= ntp; j++)
	{
	    h[j] = j * tamoy / ntp;
	    ydel[j] = 0.0;
	    xdel[j] = pizmoy;
	}
    }

    if(trmoy > accu2 && tamoy > accu2)
    {
	ydel[0] = 1.0;
	xdel[0] = 0.0;
	h[0] = 0;
	altc[0] = 300;
	float zx = 300;
	iplane = 0;

	for(int it = 0; it <= ntp; it++)

	{
	    if (it == 0) zx = discre(tamoy,ha,trmoy,8.0,it,ntp,0,0,300.0,0.0);
	    else zx = discre(tamoy,ha,trmoy,8.0,it,ntp,h[it-1],ydel[it-1],300.0,0.0);
      
	    float ca;
	    if ((-zx / ha) < -18) ca = 0;
	    else ca = (float)(tamoy * exp(-zx / ha));

	    float cr = (float)(trmoy * exp(-zx / 8.0));
	    h[it] = cr + ca;
	    altc[it] = zx;

	    cr = cr / 8;
	    ca = ca / ha;
	    float ratio = cr / (cr + ca);
	    xdel[it] = (1 - ratio) * pizmoy;
	    ydel[it] = ratio;

	}
    }
    
    /* update plane layer if necessary */
    if (ntp == (snt-1))
    {
	/* compute position of the plane layer */
	float taup = tap + trp;
        iplane = -1;
        for(int i = 0; i <= ntp; i++) if (taup >= h[i]) iplane = i;

	/* update the layer from the end to the position to update if necessary */
        float xt1 = (float)fabs(h[iplane] - taup);
        float xt2 = (float)fabs(h[iplane + 1] - taup);
        if ((xt1 > 0.005) && (xt2 > 0.005))
	{
	    for(int i = snt; i >= iplane + 1; i--)
	    {
		xdel[i] = xdel[i-1];


		ydel[i] = ydel[i-1];
		h[i] = h[i-1];
            	altc[i] = altc[i-1];
	    }
	}
        else
	{
	    snt = ntp;
	    if (xt2 < xt1) iplane = iplane + 1;
	}
         
	h[iplane] = taup;
	if ( trmoy > accu2 && tamoy > accu2) 
	{
	    float ca = (float)(tamoy * exp(-alt.palt / ha));
	    float cr = (float)(trmoy * exp(-alt.palt / 8.0));
	    cr = cr / 8;
	    ca = ca / ha;
	    float ratio = cr / (cr + ca);
	    xdel[iplane] = (1 - ratio) * pizmoy;

	    ydel[iplane] = ratio;
	    altc[iplane] = alt.palt;
	}

	if ( trmoy > accu2 && tamoy <= accu2)
	{
	    ydel[iplane] = 1;
	    xdel[iplane] = 0;
	    altc[iplane] = alt.palt;
	}

	if ( trmoy <= accu2 && tamoy > accu2) 
	{
	    ydel[iplane] = 0;
	    xdel[iplane] = 1 * pizmoy;
	    altc[iplane] = alt.palt;
	}
    }

    float aaaa = delta / (2-delta);
    float ron = (1 - aaaa) / (1 + 2 * aaaa);

    /* rayleigh phase function */
    float beta0 = 1;
    float beta2 = 0.5f * ron;

    /* primary scattering */
    int ig = 1;
    float tavion0 = 0;
    float tavion1 = 0;
    float tavion2 = 0;
    float tavion = 0;

    float i1[31][2*mu + 1];
    float i2[31][2*mu + 1];
    float i3[2*mu + 1];
    float in[2*mu + 1];
    float inm1[2*mu + 1];
    float inm2[2*mu + 1];
    int j;
    for(j = -mu; j <= mu; j++) i3[STDI(j)] = 0;

    /* kernel computations */
    float xpl[2*mu + 1];
    float bp[26][2*mu + 1];
    kernel(0, xpl, bp, gauss);

    for(j = -mu; j <= mu; j++)
	for(int k = 0; k <= snt; k++) i2[k][STDI(j)] = 0;

    /* vertical integration, primary upward radiation */
    int k;
    for(k = 1; k <= mu; k++)
    {
	i1[snt][STDI(k)] = 1.0;
	for(int i = snt-1; i >= 0; i--) 
	    i1[i][STDI(k)] = (float)(exp(-(tamoy + trmoy - h[i]) / gauss.rm[STDI(k)]));
    }


    /* vertical integration, primary downward radiation */
    for(k = -mu; k <= -1; k++)
	for(int i = 0; i <= snt; i++) i1[i][STDI(k)] = 0.0;

    /* inm2 is inialized with scattering computed at n-2
       i3 is inialized with primary scattering */
    for(k = -mu; k <= mu; k++)
    {
	if(k == 0) continue;
	if(k < 0) 
	{
	    inm1[STDI(k)] = i1[snt][STDI(k)];
	    inm2[STDI(k)] = i1[snt][STDI(k)];
	    i3[STDI(k)] = i1[snt][STDI(k)];
	}
	else
	{
	    inm1[STDI(k)] = i1[0][STDI(k)];
	    inm2[STDI(k)] = i1[0][STDI(k)];
	    i3[STDI(k)] = i1[0][STDI(k)];
	}
    }
    tavion = i1[iplane][STDI(mu)];
    tavion2 = i1[iplane][STDI(mu)];

    do {
	/* loop on successive order */
	ig++;

	/* successive orders
	   multiple scattering source function at every level within the laye */
	for(k = 1; k <= mu; k++)
	{
	    for(int i = 0; i <= snt; i++)
	    {
		double ii1 = 0;
		double ii2 = 0;
		float x = xdel[i];
		float y = ydel[i];
      
		for(int j = 1; j <= mu; j++)
		{
		    float bpjk = bp[j][STDI(k)] * x + y * (beta0 + beta2 * xpl[STDI(j)] * xpl[STDI(k)]);
		    float bpjmk= bp[j][STDI(-k)] * x + y * (beta0 + beta2 * xpl[STDI(j)] * xpl[STDI(-k)]);
		    ii2 += gauss.gb[STDI(j)] * (i1[i][STDI(j)] * bpjk + i1[i][STDI(-j)] * bpjmk);
		    ii1 += gauss.gb[STDI(j)] * (i1[i][STDI(j)] * bpjmk + i1[i][STDI(-j)] * bpjk);
		}
      
		i2[i][STDI(k)] = (float)ii2;
		i2[i][STDI(-k)] = (float)ii1;
	    }
	}

	/* vertical integration, upward radiation */
	for(k = 1; k <= mu; k++)
	{
	    i1[snt][STDI(k)] = 0.0;
	    float zi1 = i1[snt][STDI(k)];

	    for(int i = snt-1; i >= 0; i--)
	    {
		float f = h[i+1] - h[i];
		float a = (i2[i+1][STDI(k)] -i2[i][STDI(k)]) / f;
		float b = i2[i][STDI(k)] - a * h[i];
		float c = (float)exp(-f / gauss.rm[STDI(k)]);
		float xx = h[i] - h[i+1] * c;

		zi1 = c * zi1 + ((1 - c) * (b + a * gauss.rm[STDI(k)]) + a * xx) / 2;
		i1[i][STDI(k)] = zi1;
	    }
	}

	/* vertical integration, downward radiation */
	for(k = -mu; k <= -1; k++)
	{
	    i1[0][STDI(k)] = 0;
	    float zi1 = i1[0][STDI(k)];

	    for(int i = 1; i <= snt; i++)
	    {
		float f = h[i] - h[i-1];
		float c = (float)exp(f / gauss.rm[STDI(k)]);
		float a = (i2[i][STDI(k)] - i2[i-1][STDI(k)]) / f;
		float b = i2[i][STDI(k)] - a * h[i];
		float xx = h[i] - h[i-1] * c;
		zi1 = c * zi1 + ((1 - c) * (b + a * gauss.rm[STDI(k)]) + a * xx) / 2;
		i1[i][STDI(k)] = zi1;
	    }
	}
   
	/* in is the nieme scattering order */
	for(k = -mu; k <= mu; k++)
	{
	    if(k == 0) continue;
	    if(k < 0) in[STDI(k)] = i1[snt][STDI(k)];
	    else in[STDI(k)] = i1[0][STDI(k)];
	}
	tavion0 = i1[iplane][STDI(mu)];

	/* convergence test (geometrical serie) */
	if(ig > 2) 
	{
	    float z = 0;
	    float a1 = tavion2;
	    float d1 = tavion1;
	    float g1 = tavion0;
	    if (a1 >= accu && d1 >= accu && tavion >= accu)
	    {
		float y = ((g1 / d1 - d1 / a1) / ((1 - g1 / d1) * (1 - g1 / d1)) * (g1 / tavion));
		y = (float)fabs(y);
		z = z >= y ? z : y;
	    }
      
	    for(int l = -mu; l <= mu; l++)
	    {
		if (l == 0) continue;
		a1 = inm2[STDI(l)];
		d1 = inm1[STDI(l)];
		g1 = in[STDI(l)];
		if(a1 == 0) continue;
		if(d1 == 0) continue;
		if(i3[STDI(l)] == 0) continue;

		float y = ((g1 / d1 - d1 / a1) / ((1 - g1 / d1) * (1 - g1 / d1)) * (g1 / i3[STDI(l)]));
		y = (float)fabs(y);
		z = z >= y ? z : y;
	    }
      
	    if(z < 0.0001)
	    {
		/* successful test (geometrical serie) */

		for(int l = -mu; l <= mu; l++)
		{
		    if (l == 0) continue;
		    float y1 = 1;
		    d1 = inm1[STDI(l)];
		    g1 = in[STDI(l)];
		    if(d1 == 0) continue;
		    y1 = 1 - g1 / d1;
		    g1 = g1 / y1;
		    i3[STDI(l)] += g1;
		}
	
		d1 = tavion1;
		g1 = tavion0;
		if (d1 >= accu) 
		{
		    if (fabs(g1 - d1) >= accu)
		    {
			float y1 = 1 - g1 / d1;
			g1 = g1 / y1;
		    }
		    tavion = tavion + g1;

		}

		break; /* go to 505 */
	    }

	    /* inm2 is the (n-2)ieme scattering order */
	    for(k = -mu; k <= mu; k++) inm2[STDI(k)] = inm1[STDI(k)];
	    tavion2 = tavion1;
	}

	/* inm1 is the (n-1)ieme scattering order */
	for(k = -mu; k <= mu; k++) inm1[STDI(k)] = in[STDI(k)];
	tavion1 = tavion0;

	/* sum of the n-1 orders */
	for(k = -mu; k <= mu; k++) i3[STDI(k)] += in[STDI(k)];
	tavion = tavion + tavion0;

	/* stop if order n is less than 1% of the sum */
	float z = 0;
	for(k = -mu; k <= mu; k++)
	{
	    if(i3[STDI(k)] != 0)
	    {
		float y = (float)fabs(in[STDI(k)] / i3[STDI(k)]);
		z = z >= y ? z : y;
	    }
	}
	if(z < 0.00001) break;

	/* stop if order n is greater than 20 in any case */
    } while(ig <= 20);

    /* dimension for os computation */
    xf[0] = tavion;
    xf[1] = 0;
    xf[2] = 0;

    xf[2] += i3[STDI(mu)];
    for(k = 1; k <= mu; k++) xf[1] += gauss.rm[STDI(k)] * gauss.gb[STDI(k)] * i3[STDI(-k)];

}

/*
  To compute the atmospheric reflectance for the molecular atmosphere in 
  case of satellite observation.  
*/
float chand(const float xtau, const GeomCond &geom)
{
    /* input parameters: xphi,xmus,xmuv,xtau
       xphi: azimuthal difference between sun and observation (xphi=0,
       in backscattering) and expressed in degree (0.:360.)
       xmus: cosine of the sun zenith angle
       xmuv: cosine of the observation zenith angle
       xtau: molecular optical depth
       output parameter: xrray : molecular reflectance (0.:1.)
       constant : xdep: depolarization factor (0.0279) */

    const float xdep = 0.0279;

    static const float as0[10] = {
	.33243832,-6.777104e-02,.16285370,1.577425e-03,-.30924818,
	-1.240906e-02,-.10324388,3.241678e-02,.11493334,-3.503695e-02
    };

    static const float as1[2] = { .19666292, -5.439061e-02 };
    static const float as2[2] = { .14545937,-2.910845e-02 };

    double phios = (180 - geom.phi);
    double xcosf1 = 1;
    double xcosf2 = cos(phios * M_PI / 180);
    double xcosf3 = cos(2 * phios * M_PI / 180);


    double xfd = xdep / (2 - xdep);
    xfd = (1 - xfd) / (1 + 2 * xfd);

    double xph1 = 1 + (3 * geom.xmus * geom.xmus - 1) * (3 * geom.xmuv * geom.xmuv - 1) * xfd / 8;
    double xph2 = -geom.xmus * geom.xmuv * sqrt(1 - geom.xmus * geom.xmus) * sqrt(1 - geom.xmuv * geom.xmuv);
    xph2 *= xfd * 0.75;
    double xph3 = (1 - geom.xmus * geom.xmus) * (1 - geom.xmuv * geom.xmuv);
    xph3 *= xfd * 0.1875;

    double xitm = (1 - exp(-xtau * (1 / geom.xmus + 1 / geom.xmuv))) * geom.xmus / (4 * (geom.xmus + geom.xmuv));
    double xp1 = xph1 * xitm;
    double xp2 = xph2 * xitm;
    double xp3 = xph3 * xitm;
	
    xitm = (1 - exp(-xtau / geom.xmus)) * (1 - exp(-xtau / geom.xmuv));
    double cfonc1 = xph1 * xitm;
    double cfonc2 = xph2 * xitm;
    double cfonc3 = xph3 * xitm;
    double xlntau = log(xtau);

    double pl[10];	
    pl[0] = 1;
    pl[1] = xlntau;
    pl[2] = geom.xmus + geom.xmuv;
    pl[3] = xlntau * pl[2];
    pl[4] = geom.xmus * geom.xmuv;
    pl[5] = xlntau * pl[4];
    pl[6] = geom.xmus * geom.xmus + geom.xmuv * geom.xmuv;
    pl[7] = xlntau * pl[6];
    pl[8] = geom.xmus * geom.xmus * geom.xmuv * geom.xmuv;
    pl[9] = xlntau * pl[8];
	
    double fs0 = 0;
    for(int i = 0; i < 10; i++) fs0 += pl[i] * as0[i];
	
    double fs1 = pl[0] * as1[0] + pl[1] * as1[1];
    double fs2 = pl[0] * as2[0] + pl[1] * as2[1];
    double xitot1 = xp1 + cfonc1 * fs0 * geom.xmus;
    double xitot2 = xp2 + cfonc2 * fs1 * geom.xmus;
    double xitot3 = xp3 + cfonc3 * fs2 * geom.xmus;
	
    float xrray = (float)(xitot1 * xcosf1);
    xrray += (float)(xitot2 * xcosf2 * 2);

    xrray += (float)(xitot3 * xcosf3 * 2);
    xrray /= (float)geom.xmus;

    return xrray;
}

/*
  To compute the atmospheric reflectance for the molecular and aerosol atmospheres
  and the mixed atmosphere. In 6S instead of an approximation as in 5S, we use the scalar Successive
  Order of Scattering method (subroutine OS.f). The polarization terms of aerosol or rayleigh phase
  are not accounted for in the computation of the aerosol reflectance and the mixed Rayleigh-aerosol
  reflectance. The polarization is addressed in computing the Rayleigh reflectance (Subroutine
  CHAND.f) by semi-empirical fitting of the vectorized Successive Orders of Scattering method
  (Deuzé et al, 1989).
*/
void atmref(const float tamoy, const float trmoy, const float pizmoy, 
	    const float tamoyp, const float trmoyp, OpticalAtmosProperties &oap,
            Gauss &gauss, const GeomCond &geom, const AerosolModel &aero,
            const Altitude &alt)
{
    float xlm1[2 * mu + 1][np];
    float xlm2[2 * mu + 1][np];
    
    /* atmospheric reflectances */	
    oap.rorayl = 0;
    oap.roaero = 0;

    /* rayleigh reflectance 3 cases (satellite,plane,ground) */
    if(alt.palt < 900 && alt.palt > 0)
    {
	gauss.rm[STDI(-mu)] = -(float)geom.xmuv;
	gauss.rm[STDI(mu)] = (float)geom.xmuv;
	gauss.rm[STDI(0)] = -(float)geom.xmus;

	os(0, trmoy, pizmoy, 0, trmoyp, xlm1, gauss, alt, geom);
		
	oap.rorayl = (float)(xlm1[STDI(-mu)][0] / geom.xmus);
    }
    else if(alt.palt <= 0) oap.rorayl = 0;
    else oap.rorayl = chand(trmoy, geom);
	
    if (aero.iaer == 0)
    {
	oap.romix = oap.rorayl;
	return;
    }

    /* rayleigh+aerosol=romix,aerosol=roaero reflectance computed
       using sucessive order of scattering method
       3 cases: satellite,plane,ground */
    if (alt.palt > 0) 
    {
	gauss.rm[STDI(-mu)] = -(float)geom.xmuv;
	gauss.rm[STDI(mu)] = (float)geom.xmuv;
	gauss.rm[STDI(0)] = -(float)geom.xmus;

	os(tamoy, trmoy, pizmoy, tamoyp, trmoyp, xlm2, gauss, alt, geom);
	oap.romix = (float)(xlm2[STDI(-mu)][0] / geom.xmus);

	os(tamoy, 0, pizmoy, tamoyp, 0, xlm2, gauss, alt, geom);
	oap.roaero = (float)(xlm2[STDI(-mu)][0] / geom.xmus);
    }
    else
    {
	oap.roaero = 0;
	oap.romix = 0;
    }
}


float fintexp1(const float xtau)
{
    /* accuracy 2e-07... for 0<xtau<1 */
    float a[6] = { -.57721566,0.99999193,-0.24991055,0.05519968,-0.00976004,0.00107857 };
    float xftau = 1;
    float xx = a[0];
    for(int i = 1; i <= 5; i++)
    {
	xftau *= xtau;
	xx += a[i] * xftau;
    }	
    return (float)(xx - log(xtau));
}

float fintexp3(const float xtau)
{
    return (float)((exp(-xtau) * (1. - xtau) + xtau * xtau * fintexp1(xtau)) / 2.);
}

/* To compute the spherical albedo of the molecular layer. */
void csalbr(const float xtau, float& xalb)
{    
    xalb = (float)((3. * xtau - fintexp3(xtau) * (4. + 2. * xtau) + 2. * exp(-xtau)));
    xalb = (float)(xalb / (4. + 3. * xtau));
}

void scatra(const float taer, const float taerp, 
	    const float tray, const float trayp,
	    const float piza, OpticalAtmosProperties& oap,
            Gauss &gauss, const GeomCond &geom, const Altitude &alt)
{
    /* computations of the direct and diffuse transmittances
       for downward and upward paths , and spherical albedo */
    float tamol,tamolp;
    float xtrans[3];

    oap.ddirtt = 1;	oap.ddiftt = 0;
    oap.udirtt = 1;	oap.udiftt = 0;
    oap.ddirtr = 1;	oap.ddiftr = 0;
    oap.udirtr = 1;	oap.udiftr = 0;
    oap.ddirta = 1;	oap.ddifta = 0;
    oap.udirta = 1;	oap.udifta = 0;
    oap.sphalbt = 0;
    oap.sphalbr = 0;
    oap.sphalba = 0;

    for(int it = 1; it <= 3; it++)

    {
	/* it=1 rayleigh only, it=2 aerosol only, it=3 rayleigh+aerosol */
	if (it == 2 && taer <= 0) continue;

	/* compute upward,downward diffuse transmittance for rayleigh,aerosol */
	if (it == 1)
	{
	    if (alt.palt > 900)
	    {
		oap.udiftt = (float)((2. / 3. + geom.xmuv) + (2. / 3. - geom.xmuv) * exp(-tray / geom.xmuv));

		oap.udiftt = (float)(oap.udiftt / ((4. / 3.) + tray) - exp(-tray / geom.xmuv));
		oap.ddiftt = (float)((2. / 3. + geom.xmus) + (2. / 3. - geom.xmus) * exp(-tray / geom.xmus));
		oap.ddiftt = (float)(oap.ddiftt / ((4. / 3.) + tray) - exp(-tray / geom.xmus));
		oap.ddirtt = (float)exp(-tray / geom.xmus);
		oap.udirtt = (float)exp(-tray / geom.xmuv);

		csalbr(tray, oap.sphalbt);
	    } 
	    else if (alt.palt > 0 && alt.palt <= 900)
	    {
		tamol = 0;
		tamolp= 0;
		gauss.rm[STDI(-mu)] = -(float)geom.xmuv;
		gauss.rm[STDI(mu)] = (float)geom.xmuv;
		gauss.rm[STDI(0)] = (float)geom.xmus;

		iso(tamol, tray, piza, tamolp, trayp, xtrans, gauss, alt);
				
		oap.udiftt = (float)(xtrans[0] - exp(-trayp / geom.xmuv));
		oap.udirtt = (float)exp(-trayp / geom.xmuv);
		gauss.rm[STDI(-mu)] = -(float)geom.xmus;
		gauss.rm[STDI(mu)] = (float)geom.xmus;
		gauss.rm[STDI(0)] = (float)geom.xmus;
				
		oap.ddiftt = (float)((2. / 3. + geom.xmus) + (2. / 3. - geom.xmus) * exp(-tray / geom.xmus));
		oap.ddiftt = (float)(oap.ddiftt / ((4. / 3.) + tray) - exp(-tray / geom.xmus));
		oap.ddirtt = (float)exp(-tray / geom.xmus);
		oap.udirtt = (float)exp(-tray / geom.xmuv);
         
		csalbr(tray, oap.sphalbt);
	    } 
	    else if (alt.palt <= 0.)
	    {
		oap.udiftt = 0;
		oap.udirtt = 1;
	    }

	    oap.ddirtr = oap.ddirtt;
	    oap.ddiftr = oap.ddiftt;
	    oap.udirtr = oap.udirtt;
	    oap.udiftr = oap.udiftt;
	    oap.sphalbr = oap.sphalbt;
	} 

	else if (it == 2)
	{
	    tamol = 0;
	    tamolp= 0;
	    gauss.rm[STDI(-mu)] = -(float)geom.xmuv;
	    gauss.rm[STDI(mu)] = (float)geom.xmuv;
	    gauss.rm[STDI(0)] = (float)geom.xmus;

	    iso(taer, tamol, piza, taerp, tamolp, xtrans, gauss, alt);

	    oap.udiftt = (float)(xtrans[0] - exp(-taerp / geom.xmuv));
	    oap.udirtt = (float)exp(-taerp / geom.xmuv);
	    gauss.rm[STDI(-mu)] = -(float)geom.xmus;
	    gauss.rm[STDI(mu)] = (float)geom.xmus;
	    gauss.rm[STDI(0)] = (float)geom.xmus;

	    float tmp_alt = alt.palt;
	    alt.palt = 999;
	    iso(taer, tamol, piza, taerp, tamolp, xtrans, gauss, alt);
	    alt.palt = tmp_alt;

	    oap.ddirtt = (float)exp(-taer / geom.xmus);
	    oap.ddiftt = (float)(xtrans[2] - exp(-taer / geom.xmus));
	    oap.sphalbt= (float)(xtrans[1] * 2);

	    if(alt.palt <= 0)
	    {
		oap.udiftt = 0;
		oap.udirtt = 1;
	    }

	    oap.ddirta = oap.ddirtt;
	    oap.ddifta = oap.ddiftt;
	    oap.udirta = oap.udirtt;
	    oap.udifta = oap.udiftt;
	    oap.sphalba = oap.sphalbt;
	}
	else if(it == 3)
	{
	    gauss.rm[STDI(-mu)] = -(float)geom.xmuv;
	    gauss.rm[STDI(mu)] = (float)geom.xmuv;
	    gauss.rm[STDI(0)] = (float)geom.xmus;

	    iso(taer, tray, piza, taerp, trayp, xtrans, gauss, alt);

	    oap.udirtt = (float)exp(-(taerp + trayp) / geom.xmuv);
	    oap.udiftt = (float)(xtrans[0] - exp(-(taerp + trayp) / geom.xmuv));
	    gauss.rm[STDI(-mu)] = -(float)geom.xmus;
	    gauss.rm[STDI(mu)] = (float)geom.xmus;
	    gauss.rm[STDI(0)] = (float)geom.xmus;

	    float tmp_alt = alt.palt;
	    alt.palt = 999;
	    iso(taer, tray, piza, taerp, trayp, xtrans, gauss, alt);
	    alt.palt = tmp_alt;

	    oap.ddiftt = (float)(xtrans[2] - exp(-(taer + tray) / geom.xmus));
	    oap.ddirtt = (float)exp(-(taer + tray) / geom.xmus);

	    oap.sphalbt= (float)(xtrans[1] * 2);

	    if (alt.palt <= 0)
	    {
		oap.udiftt = 0;
		oap.udirtt = 1;
	    }
	}
    }
}

/* To compute the optical properties of the atmosphere at the 10 discrete
   wavelengths. */
void discom(const GeomCond &geom, const AtmosModel &atms,
            const AerosolModel &aero, const AerosolConcentration &aerocon,
            const Altitude &alt, const IWave &iwave)
{
    OpticalAtmosProperties oap;
    memset(&oap, 0, sizeof(oap));

    Gauss gauss;
    gauss.init();   /* discom is the only function that uses the gauss data */
    memset(&sixs_trunc, 0, sizeof(sixs_trunc));  /* clear this to keep preconditions the same and output consistent */

/*    computation of all scattering parameters at wavelength 
      discrete values,so we
      can interpolate at any wavelength */
    int i;
    for(i = 0; i < 10; i++)
    {
	if(!((((i < 2) && iwave.ffu.wlsup < sixs_disc.wldis[0])) || ((iwave.ffu.wlinf > sixs_disc.wldis[9]) && (i >= 8))))		
	    if (((i < 9) && (sixs_disc.wldis[i] < iwave.ffu.wlinf) && (sixs_disc.wldis[i+1] < iwave.ffu.wlinf)) || 
		((i > 0) && (sixs_disc.wldis[i] > iwave.ffu.wlsup) && (sixs_disc.wldis[i-1] > iwave.ffu.wlsup))) continue;

	float wl = sixs_disc.wldis[i];
	/* computation of rayleigh optical depth at wl */
	float tray = odrayl(atms, wl);
	float trayp;

	/* plane case discussed here above */
	if (alt.idatmp == 0) trayp = 0;
	else if (alt.idatmp == 4) trayp = tray;
	else trayp = tray * alt.ftray;

	sixs_disc.trayl[i] = tray;
	sixs_disc.traypl[i] = trayp;

	/* computation of aerosol optical properties at wl */

	float taer = aerocon.taer55 * sixs_aer.ext[i] / sixs_aer.ext[3];
	float taerp = alt.taer55p * sixs_aer.ext[i] / sixs_aer.ext[3];
	float piza = sixs_aer.ome[i];
 
	/* computation of atmospheric reflectances

	rorayl is rayleigh ref
	roaero is aerosol ref
	call plegen to decompose aerosol phase function in Betal */
		
	float coeff = 0;
	if(aero.iaer != 0)
	{
	    for(int k = 0; k < 83; k++) sixs_trunc.pha[k] = sixs_sos.phasel[i][k];
	    coeff = trunca();
	}

	float tamoy = taer * (1 - piza * coeff);
	float tamoyp = taerp * (1 - piza * coeff);
	float pizmoy = piza * (1 - coeff) / (1 - piza * coeff);

	atmref(tamoy, tray, pizmoy, tamoyp, trayp, oap, gauss, geom, aero, alt);

	/* computation of scattering transmitances (direct and diffuse)
	   first time for rayleigh ,next total (rayleigh+aerosols) */

	scatra(tamoy, tamoyp, tray, trayp, pizmoy, oap, gauss, geom, alt);
		
	sixs_disc.roatm[0][i] = oap.rorayl;
	sixs_disc.roatm[1][i] = oap.romix;
	sixs_disc.roatm[2][i] = oap.roaero;
	sixs_disc.dtdir[0][i] = oap.ddirtr;
	sixs_disc.dtdif[0][i] = oap.ddiftr;
	sixs_disc.dtdir[1][i] = oap.ddirtt;
	sixs_disc.dtdif[1][i] = oap.ddiftt;
	sixs_disc.dtdir[2][i] = oap.ddirta;
	sixs_disc.dtdif[2][i] = oap.ddifta;
	sixs_disc.utdir[0][i] = oap.udirtr;
	sixs_disc.utdif[0][i] = oap.udiftr;
	sixs_disc.utdir[1][i] = oap.udirtt;
	sixs_disc.utdif[1][i] = oap.udiftt;
	sixs_disc.utdir[2][i] = oap.udirta;
	sixs_disc.utdif[2][i] = oap.udifta;
	sixs_disc.sphal[0][i] = oap.sphalbr;
	sixs_disc.sphal[1][i] = oap.sphalbt;
	sixs_disc.sphal[2][i] = oap.sphalba;
    }

}

/*
  To compute the atmospheric properties at the equivalent wavelength (see
  EQUIVWL.f) needed for the calculation of the downward radiation field used
  in the computation of the non lambertian target contribution (main.f).
*/
void specinterp(const float wl, float& tamoy, float& tamoyp, float& pizmoy, float& pizmoyp,
                const AerosolConcentration &aerocon, const Altitude &alt)
{

    int linf = 0;
    for(int i = 0; i < 9; i++) 
	if(wl >= sixs_disc.wldis[i] && wl <= sixs_disc.wldis[i+1]) linf = i;
    if(wl > sixs_disc.wldis[9]) linf = 8;

    int lsup = linf + 1;
    float coef = (float)log(sixs_disc.wldis[lsup] / sixs_disc.wldis[linf]);
    float wlinf = sixs_disc.wldis[linf];

    float alphaa = (float)(log(sixs_aer.ext[lsup] * sixs_aer.ome[lsup] / 
			       (sixs_aer.ext[linf] * sixs_aer.ome[linf])) / coef);
    float betaa = (float)(sixs_aer.ext[linf] * sixs_aer.ome[linf] / pow(wlinf,alphaa));
    float tsca = (float)(aerocon.taer55 * betaa * pow(wl,alphaa) / sixs_aer.ext[3]);
    alphaa = (float)(log(sixs_aer.ext[lsup] / sixs_aer.ext[linf]) / coef);
    betaa = (float)(sixs_aer.ext[linf] / pow(wlinf,alphaa));
    tamoy = (float)(aerocon.taer55 * betaa * pow(wl,alphaa) / sixs_aer.ext[3]);
    tamoyp= (float)(alt.taer55p * betaa * pow(wl,alphaa) / sixs_aer.ext[3]);
    pizmoy= tsca / tamoy;
    pizmoyp = pizmoy;

    for(int k = 0; k < 83; k++)
    {
	alphaa = (float)log(sixs_sos.phasel[lsup][k] / sixs_sos.phasel[linf][k]) / coef;
	betaa = (float)(sixs_sos.phasel[linf][k] / pow(wlinf,alphaa));
	sixs_trunc.pha[k] = (float)(betaa * pow(wl,alphaa));
    }

    float coeff = trunca();

    tamoy *= 1 - pizmoy * coeff;
    tamoyp *= 1 - pizmoyp * coeff;
    pizmoy *= (1 - coeff) / (1 - pizmoy * coeff);
}

/**********************************************************************c
c                                                                      c
c                     start of computations                            c
c                                                                      c
c**********************************************************************/

/*
  To compute the environment functions F(r) which allows us to account for an
  inhomogeneous ground.
*/
void enviro (const float difr, const float difa, const float r, const float palt,
	     const float xmuv, float& fra, float& fae, float& fr)
{
    float fae0, fra0, xlnv;
    static const float alt[16] = {
	0.5,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,
	10.0,12.0,14.0,16.0,18.0,20.0,60.0
    };

    static const float cfr1[16] = {
	0.730,0.710,0.656,0.606,0.560,0.516,0.473,
	0.433,0.395,0.323,0.258,0.209,0.171,0.142,0.122,0.070
    };

    static const float cfr2[16] = {
	2.8,1.51,0.845,0.634,0.524,0.465,0.429,
	0.405,0.390,0.386,0.409,0.445,0.488,0.545,0.608,0.868
    };

    static const float cfa1[16] = {
	0.239,0.396,0.588,0.626,0.612,0.505,0.454,
	0.448,0.444,0.445,0.444,0.448,0.448,0.448,0.448,0.448
    };

    static const float cfa2[16] = {
	1.40,1.20,1.02,0.86,0.74,0.56,0.46,0.42,
	0.38,0.34,0.3,0.28,0.27,0.27,0.27,0.27
    };

    static const float cfa3[16] = {
	9.17,6.26,5.48,5.16,4.74,3.65,3.24,3.15,
	3.07,2.97,2.88,2.83,2.83,2.83,2.83,2.83
    };


/*     calculation of the environmental function for
       rayleigh and aerosols contribution.

       this calculation have been done for nadir observation
       and are corrected of the effect of the view zenith angle. */

    const float a0 = 1.3347;
    const float b0 = 0.57757;
    const float a1 = -1.479;
    const float b1 = -1.5275;

    if (palt >= 60)
    {
	fae0 = (float)(1. - 0.448 * exp( -r * 0.27) - 0.552 * exp( -r * 2.83));
	fra0 = (float)(1. - 0.930 * exp( -r * 0.080) - 0.070 * exp( -r * 1.100));
    }
    else
    {
	int i;
	for(i = 0; palt >= alt[i]; i++);
	float xcfr1 = 0, xcfr2 = 0, xcfa1 = 0, xcfa2 = 0, xcfa3 = 0;

	if ((i > 0) && (i < 16))
	{
	    float zmin = alt[i - 1];
	    float zmax = alt[i];
	    xcfr1 = cfr1[i - 1] + (cfr1[i] - cfr1[i - 1]) * (palt - zmin) / (zmax - zmin);
	    xcfr2 = cfr2[i - 1] + (cfr2[i] - cfr2[i - 1]) * (palt - zmin) / (zmax - zmin);
	    xcfa1 = cfa1[i - 1] + (cfa1[i] - cfa1[i - 1]) * (palt - zmin) / (zmax - zmin);
	    xcfa2 = cfa2[i - 1] + (cfa2[i] - cfa2[i - 1]) * (palt - zmin) / (zmax - zmin);
	    xcfa3 = cfa3[i - 1] + (cfa3[i] - cfa3[i - 1]) * (palt - zmin) / (zmax - zmin);
	}

	if (i == 0)
	{
	    xcfr1 = cfr1[0];
	    xcfr2 = cfr2[0];
	    xcfa1 = cfa1[0];
	    xcfa2 = cfa2[0];
	    xcfa3 = cfa3[0];
	}

	fra0 = (float)(1. - xcfr1 * exp(-r * xcfr2) - (1. - xcfr1) * exp(-r * 0.08));
	fae0 = (float)(1. - xcfa1 * exp(-r * xcfa2) - (1. - xcfa1) * exp(-r * xcfa3));
    }

    /* correction of the effect of the view zenith angle */
    xlnv = (float)log(xmuv);
    fra = (float)(fra0 * (xlnv * (1 - fra0) + 1));
    fae = (float)(fae0 * ((1 + a0 * xlnv + b0 * xlnv * xlnv) + fae0 * (a1 * xlnv + b1 * xlnv * xlnv) + 
			  fae0 * fae0 * ((-a1 - a0) * xlnv + ( - b1 - b0) * xlnv * xlnv)));

    if ((difa + difr) > 1e-03) fr = (fae * difa + fra * difr) / (difa + difr);
    else fr = 1;
}


