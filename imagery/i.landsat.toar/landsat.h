#ifndef _LANDSAT_H
#define _LANDSAT_H

#define UNCORRECTED     0
#define CORRECTED       1

#define DOS      		10
#define DOS1			12
#define DOS2			14
#define DOS2b			15
#define DOS3			16
#define DOS4			18

#define NOMETADATAFILE  0
#define METADATAFILE    1

/*****************************************************
 * Landsat Structures
 *
 * Lmax and Lmin in  W / (m^2 * sr * µm) -> Radiance
 * Esun in  W / (m^2 * µm)               -> Irradiance
 ****************************************************/

#define MAX_BANDS   11

typedef struct
{
    int number;			/* Band number                   */
    int code;			/* Band code                     */

    double wavemax, wavemin;	/* Wavelength in µm              */

    double esun;		/* Mean solar irradiance         */
    double lmax, lmin;		/* Spectral radiance             */
    double qcalmax, qcalmin;	/* Quantized calibrated pixel    */

    char thermal;		/* Flag to thermal band          */
    double gain, bias;		/* Gain and Bias of sensor       */
    double K1, K2;		/* Thermal calibration or
				   Rad2Ref constants             */

} band_data;

typedef struct
{
    int flag;
    unsigned char number;	/* Landsat number                */

    char creation[11];		/* Image production date         */
    char date[11];		/* Image acquisition date        */

    double dist_es;		/* Distance Earth-Sun            */
    double sun_elev;		/* Sun elevation                 */
    double sun_az;		/* Sun Azimuth                   */
    double time;		/* Image Acquisition Time        */

    char sensor[10];		/* Type of sensor: MSS, TM, ETM+, OLI/TIRS */
    int bands;			/* Total number of bands         */
    band_data band[MAX_BANDS];	/* Data for each band            */
} lsat_data;


/*****************************************************************************
 * Landsat Equations Prototypes
 *****************************************************************************/

double lsat_qcal2rad(double, band_data *);
double lsat_rad2ref(double, band_data *);
double lsat_rad2temp(double, band_data *);

void lsat_bandctes(lsat_data *, int, char, double, int, double);

#endif
