#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"
#include "earth_sun.h"

void sensor_MSS(lsat_data * lsat)
{
    int i;

    /* green, red, near infrared, near infrared */
    int band[] = { 1, 2, 3, 4 };
    int code[] = { 4, 5, 6, 7 };
    double wmax[] = { 0.6, 0.7, 0.8, 1.1 };
    double wmin[] = { 0.5, 0.6, 0.7, 0.8 };
    /* 79-82, 79-82, 79-82, 79-82 */

    strcpy(lsat->sensor, "MSS");

    lsat->bands = 4;
    for (i = 0; i < lsat->bands; i++) {
	lsat->band[i].number = *(band + i);
	lsat->band[i].code = *(code + i);
	lsat->band[i].wavemax = *(wmax + i);
	lsat->band[i].wavemin = *(wmin + i);
	lsat->band[i].qcalmax = 255.;
	lsat->band[i].qcalmin = 0.;
	lsat->band[i].thermal = 0;
    }
    return;
}

void sensor_TM(lsat_data * lsat)
{
    int i;

    /* blue, green red, near infrared, shortwave IR, thermal IR, shortwave IR */
    int band[] = { 1, 2, 3, 4, 5, 6, 7 };
    double wmax[] = { 0.52, 0.60, 0.69, 0.90, 1.75, 12.50, 2.35 };
    double wmin[] = { 0.45, 0.52, 0.63, 0.76, 1.55, 10.40, 2.08 };
    /* 30, 30, 30, 30, 30, 120, 30 */

    if (!lsat->sensor)
      strcpy(lsat->sensor, "TM");

    lsat->bands = 7;
    for (i = 0; i < lsat->bands; i++) {
	lsat->band[i].number = *(band + i);
	lsat->band[i].code = *(band + i);
	lsat->band[i].wavemax = *(wmax + i);
	lsat->band[i].wavemin = *(wmin + i);
	lsat->band[i].qcalmax = 255.;
	lsat->band[i].qcalmin = 0.;	/* Modified in set_TM5 by date */
	lsat->band[i].thermal = (lsat->band[i].number == 6 ? 1 : 0);
    }
    return;
}

void sensor_ETM(lsat_data * lsat)
{
    int i;

    /* blue, green red, near infrared, shortwave IR, thermal IR, shortwave IR, panchromatic */
    int band[] = { 1, 2, 3, 4, 5, 6, 6, 7, 8 };
    int code[] = { 1, 2, 3, 4, 5, 61, 62, 7, 8 };
    double wmax[] = { 0.515, 0.605, 0.690, 0.90, 1.75, 12.50, 2.35, 0.90 };
    double wmin[] = { 0.450, 0.525, 0.630, 0.75, 1.55, 10.40, 2.09, 0.52 };
    /* 30, 30, 30, 30, 30, 60, 30, 15 */

    strcpy(lsat->sensor, "ETM+");

    lsat->bands = 9;
    for (i = 0; i < lsat->bands; i++) {
	lsat->band[i].number = *(band + i);
	lsat->band[i].code = *(code + i);
	lsat->band[i].wavemax = *(wmax + i);
	lsat->band[i].wavemin = *(wmin + i);
	lsat->band[i].qcalmax = 255.;
	lsat->band[i].qcalmin = 1.;
	lsat->band[i].thermal = (lsat->band[i].number == 6 ? 1 : 0);
    }
    return;
}


/** **********************************************
 ** Before access to this function ...
 ** store previously
 ** >>> adquisition date,
 ** >>> creation date, and
 ** >>> sun_elev
 ** **********************************************/

/****************************************************************************
 * PURPOSE:     Store values of Landsat-1 MSS
 *              July 23, 1972 to January 7, 1978
 *****************************************************************************/
void set_MSS1(lsat_data * lsat)
{
    int i, j;

    /** Markham and Barker. EOSAT Landsat Technical Notes, No. 1, 1986;
        Chander, Markham and Helder. Remote Sensing of Environment, 113 (2009)*/

    /* Spectral radiances at detector */
    double lmax[] = { 248., 200., 176., 153. };
    double lmin[] = { 0., 0., 0., 0. };

    /* Solar exoatmospheric spectral irradiances */
    double esun[] = { 1823., 1559., 1276., 880.1 };

    lsat->number = 1;
    sensor_MSS(lsat);

    lsat->dist_es = earth_sun(lsat->date);

    for (i = 0; i < lsat->bands; i++) {
	j = lsat->band[i].number - 1;
	lsat->band[i].esun = *(esun + j);
	lsat->band[i].lmax = *(lmax + j);
	lsat->band[i].lmin = *(lmin + j);
    }
    return;
}

/****************************************************************************
 * PURPOSE:     Store values of Landsat-2 MSS
 *              January 22, 1975 to February 25, 1982
 *****************************************************************************/
void set_MSS2(lsat_data * lsat)
{
    int i, j;
    double julian, *lmax, *lmin;

    /** Markham and Barker. EOSAT Landsat Technical Notes, No. 1, 1986;
        Chander, Markham and Helder. Remote Sensing of Environment, 113 (2009)*/

    /* Spectral radiances at detector */
    double Lmax[][4] = {
	{210., 156., 140., 138.},	/* before      July 16, 1975 */
	{263., 176., 152., 130.333}	/* on or after July 16, 1975 */
    };
    double Lmin[][4] = {
	{10., 7., 7., 5.},
	{8., 6., 6., 3.667}
    };

    /* Solar exoatmospheric spectral irradiances */
    double esun[] = { 1829., 1539., 1268., 886.6 };

    julian = julian_char(lsat->creation);
    if (julian < julian_char("1975-07-16"))
	i = 0;
    else
	i = 1;
    lmax = Lmax[i];
    lmin = Lmin[i];

    lsat->number = 2;
    sensor_MSS(lsat);

    lsat->dist_es = earth_sun(lsat->date);

    for (i = 0; i < lsat->bands; i++) {
	j = lsat->band[i].number - 1;
	lsat->band[i].esun = *(esun + j);
	lsat->band[i].lmax = *(lmax + j);
	lsat->band[i].lmin = *(lmin + j);
    }
    return;
}

/****************************************************************************
 * PURPOSE:     Store values of Landsat-3 MSS
 *              March 5, 1978 to March 31, 1983
 *
 *              tiene una banda 8 thermal
 *****************************************************************************/
void set_MSS3(lsat_data * lsat)
{
    int i, j;
    double julian, *lmax, *lmin;

    /** Markham and Barker. EOSAT Landsat Technical Notes, No. 1, 1986;
        Chander, Markham and Helder. Remote Sensing of Environment, 113 (2009)*/
    /* Spectral radiances at detector */
    double Lmax[][4] = {
	{220., 175., 145., 147.},	/* before      June 1, 1978 */
	{259., 179., 149., 128.}	/* on or after June 1, 1978 */
    };
    double Lmin[][4] = {
	{4., 3., 3., 1.},
	{4., 3., 3., 1.}
    };
    /* Solar exoatmospheric spectral irradiances */
    double esun[] = { 1839., 1555., 1291., 887.9 };

    julian = julian_char(lsat->creation);
    if (julian < julian_char("1978-06-01"))
	i = 0;
    else
	i = 1;
    lmax = Lmax[i];
    lmin = Lmin[i];

    lsat->number = 3;
    sensor_MSS(lsat);

    lsat->dist_es = earth_sun(lsat->date);

    for (i = 0; i < lsat->bands; i++) {
	j = lsat->band[i].number - 1;
	lsat->band[i].esun = *(esun + j);
	lsat->band[i].lmax = *(lmax + j);
	lsat->band[i].lmin = *(lmin + j);
    }
    return;
}

/****************************************************************************
 * PURPOSE:     Store values of Landsat-4 MSS/TM
 *              July 16, 1982 to June 15, 2001
 *****************************************************************************/
void set_MSS4(lsat_data * lsat)
{
    int i, j;
    double julian, *lmax, *lmin;

    /** Markham and Barker. EOSAT Landsat Technical Notes, No. 1, 1986;
        Chander, Markham and Helder. Remote Sensing of Environment, 113 (2009)*/

    /* Spectral radiances at detector */
    double Lmax[][4] = {
	{250., 180., 150., 133.},	/* before      August 26, 1982 */
	{230., 180., 130., 133.},	/* between                     */
	{238., 164., 142., 116.}	/* on or after April 1, 1983   */
    };
    double Lmin[][4] = {
	{2., 4., 4., 3.},
	{2., 4., 4., 3.},
	{4., 4., 5., 4.}
    };

    /* Solar exoatmospheric spectral irradiances */
    double esun[] = { 1827., 1569., 1260., 866.4 };

    julian = julian_char(lsat->creation);
    if (julian < julian_char("1982-08-26"))
	i = 0;
    else if (julian < julian_char("1983-03-31"))
	i = 1;
    else
	i = 2;
    lmax = Lmax[i];
    lmin = Lmin[i];

    lsat->number = 4;
    sensor_MSS(lsat);

    lsat->dist_es = earth_sun(lsat->date);

    for (i = 0; i < lsat->bands; i++) {
	j = lsat->band[i].number - 1;
	lsat->band[i].esun = *(esun + j);
	lsat->band[i].lmax = *(lmax + j);
	lsat->band[i].lmin = *(lmin + j);
    }
    return;
}

void set_TM4(lsat_data * lsat)
{
    int i, j;
    double julian, *lmax, *lmin;

    /** Brian L. Markham and John L. Barker.
        EOSAT Landsat Technical Notes, No. 1, 1986 */
    /* Spectral radiances at detector */
    double Lmax[][7] = {
	{158.42, 308.17, 234.63, 224.32, 32.42, 15.64, 17.00},	/* before August 1983      */
	{142.86, 291.25, 225.00, 214.29, 30.00, 12.40, 15.93},	/* before January 15, 1984 */
	{152.10, 296.81, 204.30, 206.20, 27.19, 15.3032, 14.38}	/* after  Jaunary 15, 1984 */
    };
    double Lmin[][7] = {
	{-1.52, -2.84, -1.17, -1.51, -0.37, 2.00, -0.15},
	{0.00, 0.00, 0.00, 0.00, 0.00, 4.84, 0.00},
	{-1.50, -2.80, -1.20, -1.50, -0.37, 1.2378, -0.15}
    };

    /** Chander, Markham and Helder. Remote Sensing of Environment, 113 (2009) */

    /* Solar exoatmospheric spectral irradiances */
    double esun[] = { 1983., 1795., 1539., 1028., 219.8, 0., 83.49 };

    /* Thermal band calibration constants: K1 = 671.62   K2 = 1284.30 */

    julian = julian_char(lsat->creation);
    if (julian < julian_char("1983-08-01"))
	i = 0;
    else if (julian < julian_char("1984-01-15"))
	i = 1;
    else
	i = 2;
    lmax = Lmax[i];
    lmin = Lmin[i];

    lsat->number = 4;
    sensor_TM(lsat);

    lsat->dist_es = earth_sun(lsat->date);

    for (i = 0; i < lsat->bands; i++) {
	j = lsat->band[i].number - 1;
	lsat->band[i].esun = *(esun + j);
	lsat->band[i].lmax = *(lmax + j);
	lsat->band[i].lmin = *(lmin + j);
	if (lsat->band[i].thermal) {
	    lsat->band[i].K1 = 671.62;
	    lsat->band[i].K2 = 1284.30;
	}
    }
    return;
}


/****************************************************************************
 * PURPOSE:     Store values of Landsat-5 MSS/TM
 *              March 1, 1984 to today
 *****************************************************************************/
void set_MSS5(lsat_data * lsat)
{
    int i, j;
    double julian, *lmax, *lmin;

    /** Brian L. Markham and John L. Barker.
        EOSAT Landsat Technical Notes, No. 1, 1986 */
    /* Spectral radiances at detector */
    double Lmax[][4] = {
	{240., 170., 150., 127.},	/* before   April 6, 1984    */
	{268., 179., 159., 123.},	/* betweeen                  */
	{268., 179., 148., 123.}	/* after    November 9, 1984 */
    };
    double Lmin[][4] = {
	{4., 3., 4., 2.},
	{3., 3., 4., 3.},
	{3., 3., 5., 3.}
    };

    /* Solar exoatmospheric spectral irradiances */
    double esun[] = { 1824., 1570., 1249., 853.4 };

    julian = julian_char(lsat->creation);
    if (julian < julian_char("1984-04-06"))
	i = 0;
    else if (julian < julian_char("1984-11-09"))
	i = 1;
    else
	i = 2;
    lmax = Lmax[i];
    lmin = Lmin[i];

    lsat->number = 5;
    sensor_MSS(lsat);

    lsat->dist_es = earth_sun(lsat->date);

    for (i = 0; i < lsat->bands; i++) {
	j = lsat->band[i].number - 1;
	lsat->band[i].esun = *(esun + j);
	lsat->band[i].lmax = *(lmax + j);
	lsat->band[i].lmin = *(lmin + j);
    }
    return;
}

void set_TM5(lsat_data * lsat)
{
    int i, j;
    double julian, *lmax, *lmin, jbuf;

    /** Gyanesh Chander and Brian Markham.
        IEEE Transactions On Geoscience And Remote Sensing, Vol. 41, No. 11, November 2003 */

    /* Spectral radiances at detector */
    double Lmax[][7] = {
	{152.10, 296.81, 204.30, 206.20, 27.19, 15.303, 14.38},	/* before May 4, 2003 */
	{193.00, 365.00, 264.00, 221.00, 30.20, 15.303, 16.50},	/* after May 4, 2003 */
	{169.00, 333.00, 264.00, 221.00, 30.20, 15.303, 16.50}	/* after April 2, 2007 */
    };
    double Lmin[][7] = {
	{-1.52, -2.84, -1.17, -1.51, -0.37, 1.2378, -0.15},
	{-1.52, -2.84, -1.17, -1.51, -0.37, 1.2378, -0.15},
	{-1.52, -2.84, -1.17, -1.51, -0.37, 1.2378, -0.15}
    };

	/** Chander, Markham and Helder. Remote Sensing of Environment, 113 (2009) */

    /* Solar exoatmospheric spectral irradiances */
    double esun[] = { 1983., 1796., 1536., 1031., 220.0, 0., 83.44 };

    /* Thermal band calibration constants: K1 = 607.76   K2 = 1260.56 */

    julian = julian_char(lsat->creation);
    if (julian < julian_char("2003-05-04"))
	i = 0;
    else if (julian < julian_char("2007-04-02"))
	i = 1;
    else
	i = 2;
    lmax = Lmax[i];
    lmin = Lmin[i];
    if (i == 2) {		/* in Chander, Markham and Barsi 2007 */
	julian = julian_char(lsat->date);	/* Yes, here acquisition date */
	if (julian >= julian_char("1992-01-01")) {
	    lmax[0] = 193.0;
	    lmax[1] = 365.0;
	}
    }

    jbuf = julian_char("2004-04-04");
    if (julian >= jbuf) {
	G_warning
	    ("Using QCalMin=1.0 as a NLAPS product processed after 4/4/2004");
	G_warning("Discard this message if using the L5_MTL (-t) flag");
    }
    lsat->number = 5;
    sensor_TM(lsat);

    lsat->dist_es = earth_sun(lsat->date);

    for (i = 0; i < lsat->bands; i++) {
	j = lsat->band[i].number - 1;
	if (julian >= jbuf)
	    lsat->band[i].qcalmin = 1.;
	lsat->band[i].esun = *(esun + j);
	lsat->band[i].lmax = *(lmax + j);
	lsat->band[i].lmin = *(lmin + j);
	if (lsat->band[i].thermal) {
	    lsat->band[i].K1 = 607.76;
	    lsat->band[i].K2 = 1260.56;
	}
    }
    return;
}


/****************************************************************************
 * PURPOSE:     Store values of Landsat-7 ETM+
 *              April 15, 1999 to today
 *****************************************************************************/
void set_ETM(lsat_data * lsat, char gain[])
{
    int i, k, j;
    double julian, *lmax, *lmin;

    /** Richard Irish.
        Landsat 7. Science Data Users Handbook. Last update: February 17, 2007 */

    /* Spectral radiances at detector */
    /* - LOW GAIN - */
    double LmaxL[][8] = {
	{297.5, 303.4, 235.5, 235.0, 47.70, 17.04, 16.60, 244.0},	/* before      July 1, 2000 */
	{293.7, 300.9, 234.4, 241.1, 47.57, 17.04, 16.54, 243.1}	/* on or after July 1, 2000 */
    };
    double LminL[][8] = {
	{-6.2, -6.0, -4.5, -4.5, -1.0, 0.0, -0.35, -5.0},
	{-6.2, -6.4, -5.0, -5.1, -1.0, 0.0, -0.35, -4.7}
    };
    /* - HIGH GAIN - */
    double LmaxH[][8] = {
	{194.3, 202.4, 158.6, 157.5, 31.76, 12.65, 10.932, 158.4},
	{191.6, 196.5, 152.9, 157.4, 31.06, 12.65, 10.80, 158.3}
    };
    double LminH[][8] = {
	{-6.2, -6.0, -4.5, -4.5, -1.0, 3.2, -0.35, -5.0},
	{-6.2, -6.4, -5.0, -5.1, -1.0, 3.2, -0.35, -4.7}
    };

	/** Chander, Markham and Helder. Remote Sensing of Environment, 113 (2009) */

    /* Solar exoatmospheric spectral irradiances */
    double esun[] = { 1997., 1812., 1533., 1039., 230.8, 0., 84.90, 1362. };

    /*  Thermal band calibration constants: K1 = 666.09   K2 = 1282.71 */

    julian = julian_char(lsat->creation);
    if (julian < julian_char("2000-07-01"))
	k = 0;
    else
	k = 1;

    lsat->number = 7;
    sensor_ETM(lsat);

    lsat->dist_es = earth_sun(lsat->date);

    for (i = 0; i < lsat->bands; i++) {
	j = lsat->band[i].number - 1;
	lsat->band[i].esun = *(esun + j);
	if (gain[i] == 'H' || gain[i] == 'h') {
	    lmax = LmaxH[k];
	    lmin = LminH[k];
	}
	else {
	    lmax = LmaxL[k];
	    lmin = LminL[k];
	}
	lsat->band[i].lmax = *(lmax + j);
	lsat->band[i].lmin = *(lmin + j);
	if (lsat->band[i].thermal) {
	    lsat->band[i].K1 = 666.09;
	    lsat->band[i].K2 = 1282.71;
	}
    }
    return;
}
