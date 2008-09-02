#include "common.h"
#include "Interp.h"

void interp (const int iaer, const int idatmp, 
	     const float wl, const float taer55, 
	     const float taer55p, const float xmud, 
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
 
    float alphaa = 0;
    float betaa = 0;
    float alphar = 0;
    float betar = 0;
    float alphac = 0;
    float betac = 0;
    is.phaa = 0;
    is.roaero = 0;
    is.dtota = 1;
    is.utota = 1;
    is.asaer = 0;
    is.taer = 0;
    is.taerp = 0;
    float coef = (float)log(sixs_disc.wldis[lsup] / sixs_disc.wldis[linf]);
    float wlinf = sixs_disc.wldis[linf];

    if(iaer != 0)
    {
	alphaa = (float)(log(sixs_aer.phase[lsup] / sixs_aer.phase[linf]) / coef);
	betaa = (float)(sixs_aer.phase[linf] / pow(wlinf,alphaa));
	is.phaa = (float)(betaa * pow(wl,alphaa));
    }

    float d2 = 2 + delta;
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
	    alphar = (float)(log(sixs_disc.roatm[0][lsup] / sixs_disc.roatm[0][linf]) / coef);
	    betar = (float)(sixs_disc.roatm[0][linf] / pow(wlinf,alphar));
	    is.rorayl = (float)(betar * pow(wl,alphar));
	}

	if(sixs_disc.roatm[1][linf] < 0.001)
	{
	    is.romix = sixs_disc.roatm[1][linf] + (sixs_disc.roatm[1][lsup] - sixs_disc.roatm[1][linf])
		* (wl - sixs_disc.wldis[linf]) / (sixs_disc.wldis[lsup] - sixs_disc.wldis[linf]);
	}
        else
	{
	    alphac = (float)(log(sixs_disc.roatm[1][lsup] / sixs_disc.roatm[1][linf]) / coef);
	    betac = (float)(sixs_disc.roatm[1][linf] / pow(wlinf,alphac));
	    is.romix = (float)(betac * pow(wl,alphac));
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
		alphaa = (float)(log(sixs_disc.roatm[2][lsup] / sixs_disc.roatm[2][linf]) / coef);
		betaa = (float)(sixs_disc.roatm[2][linf] / pow(wlinf,alphaa));
		is.roaero = (float)(betaa * pow(wl,alphaa));
	    }
	}
    }

    alphar = (float)(log(sixs_disc.trayl[lsup] / sixs_disc.trayl[linf]) / coef);
    betar = (float)(sixs_disc.trayl[linf] / pow(wlinf,alphar));
    is.tray = (float)(betar * pow(wl,alphar));
    
    if (idatmp != 0)
    {
	alphar = (float)(log(sixs_disc.traypl[lsup] / sixs_disc.traypl[linf]) / coef);
        betar = (float)(sixs_disc.traypl[linf] / pow(wlinf,alphar));
        is.trayp = (float)(betar * pow(wl,alphar));
    }
    else is.trayp = 0;

    if(iaer != 0)
    {
	alphaa = (float)(log(sixs_aer.ext[lsup] * sixs_aer.ome[lsup] / (sixs_aer.ext[linf] * sixs_aer.ome[linf])) / coef);
	betaa = (float)(sixs_aer.ext[linf] * sixs_aer.ome[linf] / pow(wlinf,alphaa));
	is.tsca = (float)(taer55 * betaa * pow(wl,alphaa) / sixs_aer.ext[3]);
	alphaa = (float)(log(sixs_aer.ext[lsup] / sixs_aer.ext[linf]) / coef);
	betaa = (float)(sixs_aer.ext[linf] / pow(wlinf,alphaa));
	is.taerp = (float)(taer55p * betaa * pow(wl,alphaa) / sixs_aer.ext[3]);
	is.taer = (float)(taer55 * betaa * pow(wl,alphaa) / sixs_aer.ext[3]);
    }

    float drinf = sixs_disc.dtdif[0][linf] + sixs_disc.dtdir[0][linf];
    float drsup = sixs_disc.dtdif[0][lsup] + sixs_disc.dtdir[0][lsup];
    alphar = (float)(log(drsup / drinf) / coef);
    betar = (float)(drinf / pow(wlinf,alphar));
    is.dtotr = (float)(betar * pow(wl,alphar));
    float dtinf = sixs_disc.dtdif[1][linf] + sixs_disc.dtdir[1][linf];
    float dtsup = sixs_disc.dtdif[1][lsup] + sixs_disc.dtdir[1][lsup];
    alphac = (float)(log((dtsup * drinf) / (dtinf * drsup)) / coef);
    betac = (float)((dtinf / drinf) / pow(wlinf,alphac));
    float dtotc = (float)(betac * pow(wl,alphac));
    float dainf = sixs_disc.dtdif[2][linf] + sixs_disc.dtdir[2][linf];
    float dasup = sixs_disc.dtdif[2][lsup] + sixs_disc.dtdir[2][lsup];

    if(iaer != 0) 
    {
	alphaa = (float)(log(dasup / dainf) / coef);
	betaa = (float)(dainf / pow(wlinf,alphaa));
	is.dtota = (float)(betaa * pow(wl,alphaa));
    }

    is.dtott = dtotc * is.dtotr;
    float urinf = sixs_disc.utdif[0][linf] + sixs_disc.utdir[0][linf];
    float ursup = sixs_disc.utdif[0][lsup] + sixs_disc.utdir[0][lsup];
    alphar = (float)(log(ursup / urinf) / coef);
    betar = (float)(urinf / pow(wlinf,alphar));
    is.utotr = (float)(betar * pow(wl,alphar));
    float utinf = sixs_disc.utdif[1][linf] + sixs_disc.utdir[1][linf];
    float utsup = sixs_disc.utdif[1][lsup] + sixs_disc.utdir[1][lsup];
    alphac = (float)(log((utsup * urinf) / (utinf * ursup)) / coef);
    betac = (float)((utinf / urinf) / pow(wlinf,alphac));
    float utotc = (float)(betac * pow(wl,alphac));
    float uainf = sixs_disc.utdif[2][linf] + sixs_disc.utdir[2][linf];
    float uasup = sixs_disc.utdif[2][lsup] + sixs_disc.utdir[2][lsup];
    is.utott = utotc * is.utotr;

    if(iaer != 0)
    {
	alphaa = (float)(log(uasup / uainf) / coef);
	betaa = (float)(uainf / pow(wlinf,alphaa));
	is.utota = (float)(betaa * pow(wl,alphaa));
    }

    float arinf = sixs_disc.sphal[0][linf];
    float arsup = sixs_disc.sphal[0][lsup];
    alphar = (float)(log(arsup / arinf) / coef);
    betar = (float)(arinf / pow(wlinf,alphar));
    is.asray = (float)(betar * pow(wl,alphar));
    float atinf = sixs_disc.sphal[1][linf];
    float atsup = sixs_disc.sphal[1][lsup];
    alphac = (float)(log(atsup / atinf) / coef);
    betac = (float)(atinf / pow(wlinf,alphac));
    is.astot = (float)(betac * pow(wl,alphac));
    float aainf = sixs_disc.sphal[2][linf];
    float aasup = sixs_disc.sphal[2][lsup];

    if(iaer != 0)
    {
	alphaa = (float)(log(aasup / aainf) / coef);
	betaa = (float)(aainf / pow(wlinf,alphaa));
	is.asaer = (float)(betaa * pow(wl,alphaa));
    }
}
