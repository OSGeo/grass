#include <math.h>
#include "transform.h"

void EtmDN(int iwave, double asol, bool before, double &lmin, double &lmax)
{
    if (before)		/* ETM+ digital numbers taken before July 1, 2000 */
    {
	switch(iwave)
	{
	case 61:
	{
	    lmin = -6.2f;
	    lmax = 194.3f;
	    break;
	}

	case 62:
	{
	    lmin = -6.0f;
	    lmax = 202.4f;
	    break;
	}

	case 63:
	{
	    lmin = -4.5f;
	    lmax = 158.6f;
	    break;
	}

	case 64:
	{
	    if (asol < 45.)
	    {
		lmin = -4.5f;
		lmax = 235.0f;
	    }
	    else
	    {
		lmin = -4.5f;
		lmax = 157.5f;
	    }
	    break;
	}

	case 65:
	{
	    lmin = -1.0f;
	    lmax = 31.76f;
	    break;
	}

	case 66:
	{
	    lmin = -0.35f;
	    lmax = 10.932f;
	    break;
	}

	case 67:
	{
	    lmin = -5.0f;
	    lmax = 244.00f;
	    break;
	}
	}
    }
    else		/* ETM+ digital numbers taken after July 1, 2000 */
    {
	switch(iwave)
	{
	case 61:
	{
	    lmin = -6.2f;
	    lmax = 191.6f;
	    break;
	}

	case 62:
	{
	    lmin = -6.4f;
	    lmax = 196.5f;
	    break;
	}

	case 63:
	{
	    lmin = -5.0f;
	    lmax = 152.9f;
	    break;
	}

	case 64:
	{
	    if (asol < 45.)
	    {
		lmin = -5.1f;
		lmax = 241.1f;
	    }
	    else
	    {
		lmin = -5.1f;
		lmax = 157.4f;
	    }
	    break;
	}

	case 65:
	{
	    lmin = -1.0f;
	    lmax = 31.06f;
	    break;
	}

	case 66:
	{
	    lmin = -0.35f;
	    lmax = 10.80f;
	    break;
	}

	case 67:
	{
	    lmin = -4.7f;
	    lmax = 243.1f;
	    break;
	}
	}
    }
}

/* Assuming input value between 0 and 1
   if rad is true, idn should first be converted to a reflectance value
   returns adjusted value also between 0 and 1 */
double transform(const TransformInput ti, InputMask imask, double idn)
{
    /* convert from radiance to reflectance */
    if((imask & ETM_BEFORE) || (imask & ETM_AFTER))
    {
        /* http://ltpwww.gsfc.nas */
        double lmin, lmax;
        EtmDN(ti.iwave, ti.asol, imask & ETM_BEFORE, lmin, lmax);

        /* multiply idn by 255.f to correct precondition that idn lies in [0, 255] */
        idn = (lmax - lmin) / 254.f * (idn * 255.f - 1.f) + lmin;
        if (idn < 0.f) idn = 0.f;
        idn /= 255.f;
    }
    if(imask & RADIANCE) idn += (double)M_PI * idn * 255.f * ti.sb / ti.xmus / ti.seb;
          
    double rapp = idn;
    double ainrpix = ti.ainr[0][0];
    /*
    double xa = 0.0f;
    double xb = 0.0f;
    double xc = 0.0f;
    */
    double rog = rapp / ti.tgasm;
    /* The if below was added to avoid ground reflectances lower than
       zero when ainr(1,1) greater than rapp/tgasm
       In such case either the choice of atmospheric model was not
       adequate for that image or the calculated apparent reflectance
       was too low. Run the model again for other conditions.
       The lines below just decrease ainr(1,1)/tgasm to avoid too
       bright pixels in the image. Check the output file to see if that
       has happened. */

    double decrfact = 1.0f;
    if (rog < (ainrpix / ti.tgasm))
    {
	do
	{
	    decrfact = decrfact - 0.1f;
	    ainrpix = decrfact * ainrpix;
	}
	while(rog < (ainrpix / ti.tgasm));
    }

    rog = (rog - ainrpix / ti.tgasm) / ti.sutott / ti.sdtott;
    rog = rog / (1.f + rog * ti.sast);
    /*
    xa = (double)M_PI * ti.sb / ti.xmus / ti.seb / ti.tgasm / ti.sutott / ti.sdtott;
    xb = ti.srotot / ti.sutott / ti.sdtott / ti.tgasm;
    xc = ti.sast;
    */

    if (rog > 1) rog = 1;
    if (rog < 0) rog = 0;

    return rog;
}


