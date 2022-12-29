#include "common.h"
#include "interp.h"

void interp (const int iaer, const int idatmp, 
	     const double wl, const double taer55, 
	     const double taer55p, const double xmud, 
	     InterpStruct& is)
{
/*     that for the atmosphere :
       the reflectances
       rayleigh                             = rorayl
       aerosols                             = roaero
       mixing                               = romix
       the downward transmittances
       rayleigh                             = dtotr
       aerosols                             = dtota
       total                                = dtott
       the upward transmittances
       rayleigh                             = utotr
       aerosols                             = utota
       total                                = utott
       the spherical albedos
       rayleigh                             = asray
       aerosols                             = asaer
       total                                = astot
       the optical thickness of total atmosphere
       rayleigh                             = tray
       aerosols                             = taer
       the optical thickness of the atmosphere above the plane
       rayleigh                             = is.trayp
       aerosols                             = taerp
       the tsca of the aerosols (god dammed it)
       total atmosphere                     = tsca */
      
    int linf = 0;
    for(int i = 0; i < 9; i++) if(wl > sixs_disc.wldis[i] && wl <= sixs_disc.wldis[i+1]) linf = i;
	
    if(wl > sixs_disc.wldis[9]) linf = 8;
    int lsup = linf + 1;


    /*    interpolation in function of wavelength for scattering
	  atmospheric functions from discrete values at sixs_disc.wldis */
 
    double alphaa = 0;
    double betaa = 0;
    double alphar = 0;
    double betar = 0;
    double alphac = 0;
    double betac = 0;
    is.phaa = 0;
    is.roaero = 0;
    is.dtota = 1;
    is.utota = 1;
    is.asaer = 0;
    is.taer = 0;
    is.taerp = 0;
    double coef = (double)log(sixs_disc.wldis[lsup] / sixs_disc.wldis[linf]);
    double wlinf = sixs_disc.wldis[linf];

    if(iaer != 0)
    {
	alphaa = (double)(log(sixs_aer.phase[lsup] / sixs_aer.phase[linf]) / coef);
	betaa = (double)(sixs_aer.phase[linf] / pow(wlinf,alphaa));
	is.phaa = (double)(betaa * pow(wl,alphaa));
    }

    double d2 = 2 + delta;
    is.phar = (2 * (1 - delta) / d2) * .75f * (1 + xmud * xmud) + 3 * delta / d2;
    if(idatmp == 0)
    {
	betar = 0;
        betaa = 0;
        betac = 0;
    }
    else
    {
	if(sixs_disc.roatm[0][linf] < 0.001)
	{
	    is.rorayl = sixs_disc.roatm[0][linf] + (sixs_disc.roatm[0][lsup] - sixs_disc.roatm[0][linf])
		* (wl - sixs_disc.wldis[linf]) / (sixs_disc.wldis[lsup] - sixs_disc.wldis[linf]);
	}
	else
	{
	    alphar = (double)(log(sixs_disc.roatm[0][lsup] / sixs_disc.roatm[0][linf]) / coef);
	    betar = (double)(sixs_disc.roatm[0][linf] / pow(wlinf,alphar));
	    is.rorayl = (double)(betar * pow(wl,alphar));
	}

	if(sixs_disc.roatm[1][linf] < 0.001)
	{
	    is.romix = sixs_disc.roatm[1][linf] + (sixs_disc.roatm[1][lsup] - sixs_disc.roatm[1][linf])
		* (wl - sixs_disc.wldis[linf]) / (sixs_disc.wldis[lsup] - sixs_disc.wldis[linf]);
	}
        else
	{
	    alphac = (double)(log(sixs_disc.roatm[1][lsup] / sixs_disc.roatm[1][linf]) / coef);
	    betac = (double)(sixs_disc.roatm[1][linf] / pow(wlinf,alphac));
	    is.romix = (double)(betac * pow(wl,alphac));
	}

	if(iaer != 0)
	{

	    if(sixs_disc.roatm[2][linf] < 0.001)
	    {
		is.roaero = sixs_disc.roatm[2][linf]+(sixs_disc.roatm[2][lsup] - sixs_disc.roatm[2][linf])
		    * (wl - sixs_disc.wldis[linf]) / (sixs_disc.wldis[lsup] - sixs_disc.wldis[linf]);
	    }
	    else
	    {
		alphaa = (double)(log(sixs_disc.roatm[2][lsup] / sixs_disc.roatm[2][linf]) / coef);
		betaa = (double)(sixs_disc.roatm[2][linf] / pow(wlinf,alphaa));
		is.roaero = (double)(betaa * pow(wl,alphaa));
	    }
	}
    }

    alphar = (double)(log(sixs_disc.trayl[lsup] / sixs_disc.trayl[linf]) / coef);
    betar = (double)(sixs_disc.trayl[linf] / pow(wlinf,alphar));
    is.tray = (double)(betar * pow(wl,alphar));
    
    if (idatmp != 0)
    {
	alphar = (double)(log(sixs_disc.traypl[lsup] / sixs_disc.traypl[linf]) / coef);
        betar = (double)(sixs_disc.traypl[linf] / pow(wlinf,alphar));
        is.trayp = (double)(betar * pow(wl,alphar));
    }
    else is.trayp = 0;

    if(iaer != 0)
    {
	alphaa = (double)(log(sixs_aer.ext[lsup] * sixs_aer.ome[lsup] / (sixs_aer.ext[linf] * sixs_aer.ome[linf])) / coef);
	betaa = (double)(sixs_aer.ext[linf] * sixs_aer.ome[linf] / pow(wlinf,alphaa));
	is.tsca = (double)(taer55 * betaa * pow(wl,alphaa) / sixs_aer.ext[3]);
	alphaa = (double)(log(sixs_aer.ext[lsup] / sixs_aer.ext[linf]) / coef);
	betaa = (double)(sixs_aer.ext[linf] / pow(wlinf,alphaa));
	is.taerp = (double)(taer55p * betaa * pow(wl,alphaa) / sixs_aer.ext[3]);
	is.taer = (double)(taer55 * betaa * pow(wl,alphaa) / sixs_aer.ext[3]);
    }

    double drinf = sixs_disc.dtdif[0][linf] + sixs_disc.dtdir[0][linf];
    double drsup = sixs_disc.dtdif[0][lsup] + sixs_disc.dtdir[0][lsup];
    alphar = (double)(log(drsup / drinf) / coef);
    betar = (double)(drinf / pow(wlinf,alphar));
    is.dtotr = (double)(betar * pow(wl,alphar));
    double dtinf = sixs_disc.dtdif[1][linf] + sixs_disc.dtdir[1][linf];
    double dtsup = sixs_disc.dtdif[1][lsup] + sixs_disc.dtdir[1][lsup];
    alphac = (double)(log((dtsup * drinf) / (dtinf * drsup)) / coef);
    betac = (double)((dtinf / drinf) / pow(wlinf,alphac));
    double dtotc = (double)(betac * pow(wl,alphac));
    double dainf = sixs_disc.dtdif[2][linf] + sixs_disc.dtdir[2][linf];
    double dasup = sixs_disc.dtdif[2][lsup] + sixs_disc.dtdir[2][lsup];

    if(iaer != 0) 
    {
	alphaa = (double)(log(dasup / dainf) / coef);
	betaa = (double)(dainf / pow(wlinf,alphaa));
	is.dtota = (double)(betaa * pow(wl,alphaa));
    }

    is.dtott = dtotc * is.dtotr;
    double urinf = sixs_disc.utdif[0][linf] + sixs_disc.utdir[0][linf];
    double ursup = sixs_disc.utdif[0][lsup] + sixs_disc.utdir[0][lsup];
    alphar = (double)(log(ursup / urinf) / coef);
    betar = (double)(urinf / pow(wlinf,alphar));
    is.utotr = (double)(betar * pow(wl,alphar));
    double utinf = sixs_disc.utdif[1][linf] + sixs_disc.utdir[1][linf];
    double utsup = sixs_disc.utdif[1][lsup] + sixs_disc.utdir[1][lsup];
    alphac = (double)(log((utsup * urinf) / (utinf * ursup)) / coef);
    betac = (double)((utinf / urinf) / pow(wlinf,alphac));
    double utotc = (double)(betac * pow(wl,alphac));
    double uainf = sixs_disc.utdif[2][linf] + sixs_disc.utdir[2][linf];
    double uasup = sixs_disc.utdif[2][lsup] + sixs_disc.utdir[2][lsup];
    is.utott = utotc * is.utotr;

    if(iaer != 0)
    {
	alphaa = (double)(log(uasup / uainf) / coef);
	betaa = (double)(uainf / pow(wlinf,alphaa));
	is.utota = (double)(betaa * pow(wl,alphaa));
    }

    double arinf = sixs_disc.sphal[0][linf];
    double arsup = sixs_disc.sphal[0][lsup];
    alphar = (double)(log(arsup / arinf) / coef);
    betar = (double)(arinf / pow(wlinf,alphar));
    is.asray = (double)(betar * pow(wl,alphar));
    double atinf = sixs_disc.sphal[1][linf];
    double atsup = sixs_disc.sphal[1][lsup];
    alphac = (double)(log(atsup / atinf) / coef);
    betac = (double)(atinf / pow(wlinf,alphac));
    is.astot = (double)(betac * pow(wl,alphac));
    double aainf = sixs_disc.sphal[2][linf];
    double aasup = sixs_disc.sphal[2][lsup];

    if(iaer != 0)
    {
	alphaa = (double)(log(aasup / aainf) / coef);
	betaa = (double)(aainf / pow(wlinf,alphaa));
	is.asaer = (double)(betaa * pow(wl,alphaa));
    }
}
