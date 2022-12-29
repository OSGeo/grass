
#define RAD2DEG 57.295779513	/* converts from radians to degrees */
#define DEG2RAD 0.0174532925	/* converts from degrees to radians */

int dom2doy2(int year, int month, int day);

 /*============================================================================
*
*    NAME:  solpos.h
*
*    Contains:
*        S_solpos     (computes the solar position and intensity
*                      from time and place)
*            INPUTS:     (from posdata)
*                          year, month, day, hour, minute, second,
*                          latitude, longitude, timezone, interval
*            OPTIONAL:   (from posdata; defaults from S_init function)
*                            press   DEFAULT 1013.0 (standard pressure)
*                            temp    DEFAULT   10.0 (standard temperature)
*                            tilt    DEFAULT    0.0 (horizontal panel)
*                            aspect  DEFAULT  180.0 (South-facing panel)
*                            sbwid   DEFAULT    7.6 (shadowband width)
*                            sbrad   DEFAULT   31.7 (shadowband radius)
*                            sbsky   DEFAULT   0.04 (shadowband sky factor)
*
*            OUTPUTS:    (posdata) daynum, amass, ampress, azim, cosinc,
*                        elevref, etr, etrn, etrtilt, prime,
*                        sbcf, sretr, ssetr, unprime, zenref
*
*            RETURNS:   Long int status code (defined in solpos.h)
*
*    Usage:
*         In calling program, along with other 'includes', insert:
*
*              #include "solpos.h"
*
*    Martin Rymes
*    National Renewable Energy Laboratory
*    25 March 1998
*
*----------------------------------------------------------------------------*/

/*============================================================================
*
*     Define the function codes
*
*----------------------------------------------------------------------------*/
#define L_DOY    0x0001
#define L_GEOM   0x0002
#define L_ZENETR 0x0004
#define L_SSHA   0x0008
#define L_SBCF   0x0010
#define L_TST    0x0020
#define L_SRSS   0x0040
#define L_SOLAZM 0x0080
#define L_REFRAC 0x0100
#define L_AMASS  0x0200
#define L_PRIME  0x0400
#define L_TILT   0x0800
#define L_ETR    0x1000
#define L_ALL    0xFFFF

/*============================================================================
*
*     Define the bit-wise masks for each function
*
*----------------------------------------------------------------------------*/
#define S_DOY    ( L_DOY                          )
#define S_GEOM   ( L_GEOM   | S_DOY               )
#define S_ZENETR ( L_ZENETR | S_GEOM              )
#define S_SSHA   ( L_SSHA   | S_GEOM              )
#define S_SBCF   ( L_SBCF   | S_SSHA              )
#define S_TST    ( L_TST    | S_GEOM              )
#define S_SRSS   ( L_SRSS   | S_SSHA   | S_TST    )
#define S_SOLAZM ( L_SOLAZM | S_ZENETR            )
#define S_REFRAC ( L_REFRAC | S_ZENETR            )
#define S_AMASS  ( L_AMASS  | S_REFRAC            )
#define S_PRIME  ( L_PRIME  | S_AMASS             )
#define S_TILT   ( L_TILT   | S_SOLAZM | S_REFRAC )
#define S_ETR    ( L_ETR    | S_REFRAC            )
#define S_ALL    ( L_ALL                          )


/*============================================================================
*
*     Enumerate the error codes
*     (Bit positions are from least significant to most significant)
*
*----------------------------------------------------------------------------*/
/*          Code          Bit       Parameter            Range
   ===============     ===  ===================  =============   */
enum
{ S_YEAR_ERROR,			/*  0   year                  1950 -  2050   */
    S_MONTH_ERROR,		/*  1   month                    1 -    12   */
    S_DAY_ERROR,		/*  2   day-of-month             1 -    31   */
    S_DOY_ERROR,		/*  3   day-of-year              1 -   366   */
    S_HOUR_ERROR,		/*  4   hour                     0 -    24   */
    S_MINUTE_ERROR,		/*  5   minute                   0 -    59   */
    S_SECOND_ERROR,		/*  6   second                   0 -    59   */
    S_TZONE_ERROR,		/*  7   time zone              -12 -    12   */
    S_INTRVL_ERROR,		/*  8   interval (seconds)       0 - 28800   */
    S_LAT_ERROR,		/*  9   latitude               -90 -    90   */
    S_LON_ERROR,		/* 10   longitude             -180 -   180   */
    S_TEMP_ERROR,		/* 11   temperature (deg. C)  -100 -   100   */
    S_PRESS_ERROR,		/* 12   pressure (millibars)     0 -  2000   */
    S_TILT_ERROR,		/* 13   tilt                   -90 -    90   */
    S_ASPECT_ERROR,		/* 14   aspect                -360 -   360   */
    S_SBWID_ERROR,		/* 15   shadow band width (cm)   1 -   100   */
    S_SBRAD_ERROR,		/* 16   shadow band radius (cm)  1 -   100   */
    S_SBSKY_ERROR
};				/* 17   shadow band sky factor  -1 -     1   */

struct posdata
{

  /***** ALPHABETICAL LIST OF COMMON VARIABLES *****/
    /* Each comment begins with a 1-column letter code:
       I:  INPUT variable
       O:  OUTPUT variable
       T:  TRANSITIONAL variable used in the algorithm,
       of interest only to the solar radiation
       modelers, and available to you because you
       may be one of them.

       The FUNCTION column indicates which sub-function
       within solpos must be switched on using the
       "function" parameter to calculate the desired
       output variable.  All function codes are
       defined in the solpos.h file.  The default
       S_ALL switch calculates all output variables.
       Multiple functions may be or'd to create a
       composite function switch.  For example,
       (S_TST | S_SBCF). Specifying only the functions
       for required output variables may allow solpos
       to execute more quickly.

       The S_DOY mask works as a toggle between the
       input date represented as a day number (daynum)
       or as month and day.  To set the switch (to
       use daynum input), the function is or'd; to
       clear the switch (to use month and day input),
       the function is inverted and and'd.

       For example:
       pdat->function |= S_DOY (sets daynum input)
       pdat->function &= ~S_DOY (sets month and day input)

       Whichever date form is used, S_solpos will
       calculate and return the variables(s) of the
       other form.  See the soltest.c program for
       other examples. */

    /* VARIABLE        I/O  Function    Description */
    /* -------------  ----  ----------  --------------------------------------- */

    int day;			/* I/O: S_DOY      Day of month (May 27 = 27, etc.)
				   solpos will CALCULATE this by default,
				   or will optionally require it as input
				   depending on the setting of the S_DOY
				   function switch. */
    int daynum;			/* I/O: S_DOY      Day number (day of year; Feb 1 = 32 )
				   solpos REQUIRES this by default, but
				   will optionally calculate it from
				   month and day depending on the setting
				   of the S_DOY function switch. */
    int function;		/* I:              Switch to choose functions for desired
				   output. */
    int hour;			/* I:              Hour of day, 0 - 23, DEFAULT = 12 */
    int interval;		/* I:              Interval of a measurement period in
				   seconds.  Forces solpos to use the
				   time and date from the interval
				   midpoint. The INPUT time (hour,
				   minute, and second) is assumed to
				   be the END of the measurement
				   interval. */
    int minute;			/* I:              Minute of hour, 0 - 59, DEFAULT = 0 */
    int month;			/* I/O: S_DOY      Month number (Jan = 1, Feb = 2, etc.)
				   solpos will CALCULATE this by default,
				   or will optionally require it as input
				   depending on the setting of the S_DOY
				   function switch. */
    int second;			/* I:              Second of minute, 0 - 59, DEFAULT = 0 */
    int year;			/* I:              4-digit year (2-digit year is NOT
				   allowed */

    int time_updated;           /* recalculate time-dependent variables */
    int longitude_updated;      /* recalculate longitude-dependent variables */

  /***** FLOATS *****/

    float amass;		/* O:  S_AMASS    Relative optical airmass */
    float ampress;		/* O:  S_AMASS    Pressure-corrected airmass */
    float aspect;		/* I:             Azimuth of panel surface (direction it
				   faces) N=0, E=90, S=180, W=270,
				   DEFAULT = 180 */
    float azim;			/* O:  S_SOLAZM   Solar azimuth angle:  N=0, E=90, S=180,
				   W=270 */
    float cosinc;		/* O:  S_TILT     Cosine of solar incidence angle on
				   panel */
    float coszen;		/* O:  S_REFRAC   Cosine of refraction corrected solar
				   zenith angle */
    float dayang;		/* T:  S_GEOM     Day angle (daynum*360/year-length)
				   degrees */
    float declin;		/* T:  S_GEOM     Declination--zenith angle of solar noon
				   at equator, degrees NORTH */
    float eclong;		/* T:  S_GEOM     Ecliptic longitude, degrees */
    float ecobli;		/* T:  S_GEOM     Obliquity of ecliptic */
    float ectime;		/* T:  S_GEOM     Time of ecliptic calculations */
    float elevetr;		/* O:  S_ZENETR   Solar elevation, no atmospheric
				   correction (= ETR) */
    float elevref;		/* O:  S_REFRAC   Solar elevation angle,
				   deg. from horizon, refracted */
    float eqntim;		/* T:  S_TST      Equation of time (TST - LMT), minutes */
    float erv;			/* T:  S_GEOM     Earth radius vector
				   (multiplied to solar constant) */
    float etr;			/* O:  S_ETR      Extraterrestrial (top-of-atmosphere)
				   W/sq m global horizontal solar
				   irradiance */
    float etrn;			/* O:  S_ETR      Extraterrestrial (top-of-atmosphere)
				   W/sq m direct normal solar
				   irradiance */
    float etrtilt;		/* O:  S_TILT     Extraterrestrial (top-of-atmosphere)
				   W/sq m global irradiance on a tilted
				   surface */
    float gmst;			/* T:  S_GEOM     Greenwich mean sidereal time, hours */
    float hrang;		/* T:  S_GEOM     Hour angle--hour of sun from solar noon,
				   degrees WEST */
    float julday;		/* T:  S_GEOM     Julian Day of 1 JAN 2000 minus
				   2,400,000 days (in order to regain
				   single precision) */
    float latitude;		/* I:             Latitude, degrees north (south negative) */
    float longitude;		/* I:             Longitude, degrees east (west negative) */
    float lmst;			/* T:  S_GEOM     Local mean sidereal time, degrees */
    float mnanom;		/* T:  S_GEOM     Mean anomaly, degrees */
    float mnlong;		/* T:  S_GEOM     Mean longitude, degrees */
    float rascen;		/* T:  S_GEOM     Right ascension, degrees */
    float press;		/* I:             Surface pressure, millibars, used for
				   refraction correction and ampress */
    float prime;		/* O:  S_PRIME    Factor that normalizes Kt, Kn, etc. */
    float sbcf;			/* O:  S_SBCF     Shadow-band correction factor */
    float sbwid;		/* I:             Shadow-band width (cm) */
    float sbrad;		/* I:             Shadow-band radius (cm) */
    float sbsky;		/* I:             Shadow-band sky factor */
    float solcon;		/* I:             Solar constant (NREL uses 1367 W/sq m) */
    float ssha;			/* T:  S_SRHA     Sunset(/rise) hour angle, degrees */
    float sretr;		/* O:  S_SRSS     Sunrise time, minutes from midnight,
				   local, WITHOUT refraction */
    float ssetr;		/* O:  S_SRSS     Sunset time, minutes from midnight,
				   local, WITHOUT refraction */
    float temp;			/* I:             Ambient dry-bulb temperature, degrees C,
				   used for refraction correction */
    float tilt;			/* I:             Degrees tilt from horizontal of panel */
    float timezone;		/* I:             Time zone, east (west negative).
				   USA:  Mountain = -7, Central = -6, etc. */
    float tst;			/* T:  S_TST      True solar time, minutes from midnight */
    float tstfix;		/* T:  S_TST      True solar time - local standard time */
    float unprime;		/* O:  S_PRIME    Factor that denormalizes Kt', Kn', etc. */
    float utime;		/* T:  S_GEOM     Universal (Greenwich) standard time */
    float zenetr;		/* T:  S_ZENETR   Solar zenith angle, no atmospheric
				   correction (= ETR) */
    float zenref;		/* O:  S_REFRAC   Solar zenith angle, deg. from zenith,
				   refracted */
};

/* For users that wish to access individual functions, the following table
   lists all output and transition variables, the L_ mask for the function
   that calculates them, and all the input variables required by that function.
   The function variable is set to the L_ mask, which will force S_solpos to
   only call the required function.  L_ masks may be ORed as desired.

   VARIABLE      Mask       Required Variables
   ---------  ----------  ---------------------------------------
   amass      L_AMASS    zenref, press
   ampress    L_AMASS    zenref, press
   azim       L_SOLAZM   elevetr, declin, latitude, hrang
   cosinc     L_TILT     azim, aspect, tilt, zenref, coszen,etrn
   coszen     L_REFRAC   elevetr, press, temp
   dayang     L_GEOM     All date, time, and location inputs
   declin     L_GEOM     All date, time, and location inputs
   eclong     L_GEOM     All date, time, and location inputs
   ecobli     L_GEOM     All date, time, and location inputs
   ectime     L_GEOM     All date, time, and location inputs
   elevetr    L_ZENETR   declin, latitude, hrang
   elevref    L_REFRAC   elevetr, press, temp
   eqntim     L_TST      hrang, hour, minute, second, interval
   erv        L_GEOM     All date, time, and location inputs
   etr        L_ETR      coszen, solcon, erv
   etrn       L_ETR      coszen, solcon, erv
   etrtilt    L_TILT     azim, aspect, tilt, zenref, coszen, etrn
   gmst       L_GEOM     All date, time, and location inputs
   hrang      L_GEOM     All date, time, and location inputs
   julday     L_GEOM     All date, time, and location inputs
   lmst       L_GEOM     All date, time, and location inputs
   mnanom     L_GEOM     All date, time, and location inputs
   mnlong     L_GEOM     All date, time, and location inputs
   rascen     L_GEOM     All date, time, and location inputs
   prime      L_PRIME    amass
   sbcf       L_SBCF     latitude, declin, ssha, sbwid, sbrad, sbsky
   ssha       L_SRHA     latitude, declin
   sretr      L_SRSS     ssha, tstfix
   ssetr      L_SRSS     ssha, tstfix
   tst        L_TST      hrang, hour, minute, second, interval
   tstfix     L_TST      hrang, hour, minute, second, interval
   unprime    L_PRIME    amass
   utime      L_GEOM     All date, time, and location inputs
   zenetr     L_ZENETR   declination, latitude, hrang
   zenref     L_REFRAC   elevetr, press, temp

 */

/*============================================================================
*    Long int function S_solpos, adapted from the NREL VAX solar libraries
*
*    This function calculates the apparent solar position and intensity
*    (theoretical maximum solar energy) based on the date, time, and
*    location on Earth. (DEFAULT values are from the optional S_posinit
*    function.)
*
*    Requires:
*        Date and time:
*            year
*            month  (optional without daynum)
*            day    (optional without daynum)
*            daynum
*            hour
*            minute
*            second
*        Location:
*            latitude
*            longitude
*        Location/time adjuster:
*            timezone
*        Atmospheric pressure and temperature:
*            press     DEFAULT 1013.0 mb
*            temp      DEFAULT 10.0 degrees C
*        Tilt of flat surface that receives solar energy:
*            aspect    DEFAULT 180 (South)
*            tilt      DEFAULT 0 (Horizontal)
*        Shadow band parameters:
*            sbwid     DEFAULT 7.6 cm
*            sbrad     DEFAULT 31.7 cm
*            sbsky     DEFAULT 0.04
*        Functionality
*            function  DEFAULT S_ALL (all output parameters computed)
*
*    Returns:
*        everything defined at the top of this listing.
*----------------------------------------------------------------------------*/
long S_solpos(struct posdata *pdat);

/*============================================================================
*    Void function S_init
*
*    This function initiates all of the input functions to S_Solpos().
*    NOTE: This function is optional if you initialize all input parameters
*          in your calling code.
*
*    Requires: Pointer to a posdata structure, members of which are
*           initialized.
*
*    Returns: Void
*
*----------------------------------------------------------------------------*/
void S_init(struct posdata *pdat);


/*============================================================================
*    Void function S_decode
*
*    This function decodes the error codes from S_solpos return value
*
*    INPUTS: Long integer S_solpos return value, struct posdata*
*
*    OUTPUTS: Descriptive text of errors to stderr
*----------------------------------------------------------------------------*/
void S_decode(long code, struct posdata *pdat);
