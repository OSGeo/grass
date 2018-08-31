#include <cstring>

extern "C" {
#include <grass/gis.h>
#include <grass/glocale.h>
}

#include "6s.h"
#include "common.h"
#include "geomcond.h"
#include "atmosmodel.h"
#include "aerosolmodel.h"
#include "aerosolconcentration.h"
#include "altitude.h"
#include "iwave.h"
#include "transform.h"
#include "abstra.h"
#include "interp.h"
#include "output.h"

/* Function prototypes */
extern void discom(const GeomCond &geom, const AtmosModel &atms,
                   const AerosolModel &aero, const AerosolConcentration &aerocon,
                   const Altitude &alt, const IWave &iwave);
extern void specinterp(const double wl, double& tamoy, double& tamoyp, double& pizmoy, double& pizmoyp,
                       const AerosolConcentration &aerocon, const Altitude &alt);
extern void enviro (const double difr, const double difa, const double r, const double palt,
		    const double xmuv, double& fra, double& fae, double& fr);
void printOutput(); // forward declare this function so that it can be used in init_6S


/* Globals */
static GeomCond geom;
static AtmosModel atms;
static AerosolModel aero;
static AerosolConcentration aerocon;
static Altitude alt;
static IWave iwave;


/* The atmospheric model is modified after the first time it is loaded.
   Therefore we need to keep a copy of it just after it is loaded to be
   used in subsequent height changes. */
static AtmosModel original_atms;
int init_6S(char* icnd_name)
{
    /* (atmospheric conditions input text file) */
    ifstream inText;
    inText.open(icnd_name);
    if(!inText.is_open()) {
	G_fatal_error(_("Unable to open file <%s>"), icnd_name);
    }

    /* redirect cin to the input text file */
    cin.rdbuf(inText.rdbuf());

    /* read the input geometrical conditions */
    geom = GeomCond::Parse();

    /* read atmospheric model */
    original_atms = AtmosModel::Parse();
    atms = original_atms; /* making a copy */

    /* read aerosol model */
    aero = AerosolModel::Parse(geom.xmud);

    /* read aerosol concentration */
    aerocon = AerosolConcentration::Parse(aero.iaer, atms);
    
    /* read altitude */
    alt = Altitude::Parse();
    alt.init(atms, aerocon);

    /* read iwave stuff */
    iwave = IWave::Parse();
   
    /**********************************************************************c
	c here, we first compute an equivalent wavelength which is the input   c
	c value for monochromatic conditions or the integrated value for a     c
	c filter function (call equivwl) then, the atmospheric properties are  c
	c computed for that wavelength (call discom then call specinterp)      c
	c molecular optical thickness is computed too (call odrayl). lastly    c
	c the successive order of scattering code is called three times.       c
	c first for a sun at thetas with the scattering properties of aerosols c
	c and molecules, second with a pure molecular atmosphere, then with thec
	c actual atmosphere for a sun at thetav. the iso code allows us to     c
	c compute the scattering transmissions and the spherical albedo. all   c
	c these computations are performed for checking the accuracy of the    c
	c analytical expressions and in addition for computing the averaged    c
	c directional reflectances                                             c
	c**********************************************************************/

    /* NOTE: wlmoy is not affected by a height and/or vis change */
    double wlmoy;
    if(iwave.iwave != -1) wlmoy = iwave.equivwl();
    else wlmoy = iwave.wl;

    iwave.wlmoy = wlmoy;

    discom(geom, atms, aero, aerocon, alt, iwave);
    double tamoy, tamoyp, pizmoy, pizmoyp;
    if(aero.iaer != 0) specinterp(wlmoy, tamoy, tamoyp, pizmoy, pizmoyp, aerocon, alt);

    printOutput();
    fflush(stderr);
    return 0;
}

/* Only update those objects that are affected by a height and vis change */
void pre_compute_hv(const double height, const double vis)
{
    atms = original_atms;
    aerocon.set_visibility(vis, atms);
    alt.set_height(height);
    alt.init(atms, aerocon);
   
    double wlmoy = iwave.wlmoy;

    discom(geom, atms, aero, aerocon, alt, iwave);
    double tamoy, tamoyp, pizmoy, pizmoyp;
    if(aero.iaer != 0) specinterp(wlmoy, tamoy, tamoyp, pizmoy, pizmoyp, aerocon, alt);
}

/* Only update those objects that are affected by a visibility change */
void pre_compute_v(const double vis)
{
    atms = original_atms;
    aerocon.set_visibility(vis, atms);
    alt.init(atms, aerocon);

    double wlmoy = iwave.wlmoy;

    discom(geom, atms, aero, aerocon, alt, iwave);
    double tamoy, tamoyp, pizmoy, pizmoyp;
    if(aero.iaer != 0) specinterp(wlmoy, tamoy, tamoyp, pizmoy, pizmoyp, aerocon, alt);
}

/* Only update those objects that are affected by a height change */
void pre_compute_h(const double height)
{
    atms = original_atms;
    alt.set_height(height);
    alt.init(atms, aerocon);

    double wlmoy = iwave.wlmoy;

    discom(geom, atms, aero, aerocon, alt, iwave);
    double tamoy, tamoyp, pizmoy, pizmoyp;
    if(aero.iaer != 0) specinterp(wlmoy, tamoy, tamoyp, pizmoy, pizmoyp, aerocon, alt);
}


void printOutput()
{
    static const string head(" 6s version 4.2b ");

    if (G_verbose() <= G_verbose_std())
	return;
    
    cout << endl << endl << endl;
    Output::Begin(); Output::Repeat(30,'*'); Output::Print(head); Output::Repeat(30,'*'); Output::End();

    /* ---- geometrical conditions ---- */
    geom.print();

    /* --- atmospheric model ---- */
    atms.print();

    /* --- aerosols model (type) ---- */
    aero.print();

    /* --- aerosols model (concentration) ---- */
    aerocon.print();

    /* --- spectral condition ---- */
    iwave.print();

    /* --- ground reflectance (type and spectral variation) ---- */

    Output::Ln();
    Output::WriteLn(22," target type  ");
    Output::WriteLn(22," -----------  ");
    Output::WriteLn(10," homogeneous ground ");

    /* 12x a39 f6.3 */
    static const string reflec[8] = {
	string(" user defined spectral reflectance     "),
	string(" monochromatic reflectance "),
	string(" constant reflectance over the spectra "),
	string(" spectral vegetation ground reflectance"),
	string(" spectral clear water reflectance      "),
	string(" spectral dry sand ground reflectance  "),
	string(" spectral lake water reflectance       "),
	string(" spectral volcanic debris reflectance  ")
    };

    double rocave = 0;       /* block of code in Fortran will always compute 0 */
    ostringstream s;
    s.setf(ios::fixed, ios::floatfield);
    s << setprecision(3);
    s << reflec[2] << setw(9) << rocave << ends;
    Output::WriteLn(12, s.str());


    /* --- pressure at ground level (174) and altitude (175) ---- */
    Output::Ln();
    Output::WriteLn(22," target elevation description ");
    Output::WriteLn(22," ---------------------------- ");

    ostringstream s1;
    s1.setf(ios::fixed, ios::floatfield);
    s1 << setprecision(2);
    s1 << " ground pressure  [mb]     " << setw(9) << atms.p[0] << ends;
    Output::WriteLn(10,s1.str());

    ostringstream s2;
    s2.setf(ios::fixed, ios::floatfield);
    s2 << setprecision(3);
    s2 << " ground altitude  [km]    " << setw(9) << alt.xps << ends;
    Output::WriteLn(10,s2.str());

    if( alt.xps > 0 )
    {
	Output::WriteLn(15," gaseous content at target level: ");

	ostringstream s3;
	s3.setf(ios::fixed, ios::floatfield);
	s3 << setprecision(3);
	s3 << " uh2o=" << setw(9) << atms.uw << " g/cm2      "
	   << "  uo3=" << setw(9) << atms.uo3 << " cm-atm" << ends;
	Output::WriteLn(15,s3.str());
    }

    alt.print();

    /* ---- atmospheric correction  ---- */
    Output::Ln();
    Output::WriteLn(23," atmospheric correction activated ");
    Output::WriteLn(23," -------------------------------- ");
}

TransformInput compute()
{
    const double accu3 = 1e-07;
/* ---- initilialization	 very liberal :) */
    int i, j;

    double fr = 0;
    double rad = 0;
    double sb = 0;
    double seb = 0;
    double refet = 0;
    double refet1 = 0;
    double refet2 = 0;
    double refet3 = 0;
    double alumet = 0;
    double tgasm = 0;
    double rog = 0;
    double dgasm = 0;
    double ugasm = 0;
    double sdwava = 0;
    double sdozon = 0;
    double sddica = 0;
    double sdoxyg = 0;
    double sdniox = 0;
    double sdmoca = 0;
    double sdmeth = 0;

    double suwava = 0;
    double suozon = 0;
    double sudica = 0;
    double suoxyg = 0;
    double suniox = 0;
    double sumoca = 0;
    double sumeth = 0;
    double stwava = 0;
    double stozon = 0;
    double stdica = 0;
    double stoxyg = 0;
    double stniox = 0;
    double stmoca = 0;
    double stmeth = 0;
    double sodray = 0;
    double sodrayp = 0;
    double sodaer = 0;
    double sodaerp = 0;
    double sodtot = 0;
    double sodtotp = 0;
    double fophsr = 0;
    double fophsa = 0;
    double sroray = 0;
    double sroaer = 0;
    double srotot = 0;
    double ssdaer = 0;
    double sdtotr = 0;
    double sdtota = 0;
    double sdtott = 0;
    double sutotr = 0;
    double sutota = 0;
    double sutott = 0;
    double sasr = 0;
    double sasa = 0;
    double sast = 0;

    double ani[2][3];
    double aini[2][3];
    double anr[2][3];
    double ainr[2][3];

    for(i = 0; i < 2; i++)
	for(j = 0; j < 3; j++)
	{
	    ani[i][j] = 0;
	    aini[i][j] = 0;
	    anr[i][j] = 0;
	    ainr[i][j] = 0;
	}

    /* ---- spectral loop ---- */
    if (iwave.iwave == -2)
    {
	Output::WriteLn(1,"wave   total  total  total  total  atm.   swl    step   sbor   dsol   toar ");
	Output::WriteLn(1,"       gas    scat   scat   spheri intr   ");
	Output::WriteLn(1,"       trans  down   up     albedo refl   ");
    }

    int l;
    for(l = iwave.iinf; l <= iwave.isup; l++)
    {
        double sbor = iwave.ffu.s[l];

        if(l == iwave.iinf || l == iwave.isup) sbor *= 0.5f;
        if(iwave.iwave == -1) sbor = 1.0f / step;

        double roc = 0; /* rocl[l]; */
        double roe = 0; /* roel[l]; */
        double wl = 0.25f + l * step;

	AbstraStruct as;
	double uwus, uo3us;		/* initialized in abstra */

	abstra(atms, alt, wl, (double)geom.xmus, (double)geom.xmuv, atms.uw / 2.0f, atms.uo3,
	       uwus, uo3us, alt.puw / 2.0f, alt.puo3, alt.puwus, alt.puo3us, as);

	double attwava = as.ttwava;

	abstra(atms, alt, wl, (double)geom.xmus, (double)geom.xmuv, atms.uw, atms.uo3,
	       uwus, uo3us, alt.puw, alt.puo3, alt.puwus, alt.puo3us, as);

        if (as.dtwava < accu3) as.dtwava = 0;
        if (as.dtozon < accu3) as.dtozon = 0;
        if (as.dtdica < accu3) as.dtdica = 0;
        if (as.dtniox < accu3) as.dtniox = 0;
        if (as.dtmeth < accu3) as.dtmeth = 0;
        if (as.dtmoca < accu3) as.dtmeth = 0;
        if (as.utwava < accu3) as.utwava = 0;
        if (as.utozon < accu3) as.utozon = 0;
        if (as.utdica < accu3) as.utdica = 0;
        if (as.utniox < accu3) as.utniox = 0;
        if (as.utmeth < accu3) as.utmeth = 0;
        if (as.utmoca < accu3) as.utmeth = 0;
        if (as.ttwava < accu3) as.ttwava = 0;
        if (as.ttozon < accu3) as.ttozon = 0;
        if (as.ttdica < accu3) as.ttdica = 0;
        if (as.ttniox < accu3) as.ttniox = 0;
        if (as.ttmeth < accu3) as.ttmeth = 0;
        if (as.ttmoca < accu3) as.ttmeth = 0;

        double swl = iwave.solirr(wl);
        swl = swl * geom.dsol;
        double coef = sbor * step * swl;

	InterpStruct is;
	memset(&is, 0, sizeof(is));
	interp(aero.iaer, alt.idatmp, wl, aerocon.taer55, alt.taer55p, (double)geom.xmud, is);


        double dgtot = as.dtwava * as.dtozon * as.dtdica * as.dtoxyg * as.dtniox * as.dtmeth * as.dtmoca;
        double tgtot = as.ttwava * as.ttozon * as.ttdica * as.ttoxyg * as.ttniox * as.ttmeth * as.ttmoca;
        double ugtot = as.utwava * as.utozon * as.utdica * as.utoxyg * as.utniox * as.utmeth * as.utmoca;
        double tgp1 = as.ttozon * as.ttdica * as.ttoxyg * as.ttniox * as.ttmeth * as.ttmoca;
        double tgp2 = attwava * as.ttozon * as.ttdica * as.ttoxyg * as.ttniox * as.ttmeth * as.ttmoca;
        double edifr = (double)(is.utotr - exp(-is.trayp / geom.xmuv));
        double edifa = (double)(is.utota - exp(-is.taerp / geom.xmuv));


	double fra, fae;
	enviro(edifr, edifa, rad, alt.palt, (double)geom.xmuv, fra, fae, fr);

	double avr = roc * fr + (1 - fr) * roe;
	double rsurf = (double)(roc * is.dtott * exp(-(is.trayp + is.taerp) / geom.xmuv) / (1 - avr * is.astot)
			      + avr * is.dtott * (is.utott - exp(-(is.trayp + is.taerp) / geom.xmuv)) / (1 - avr * is.astot));
        double ratm1 = (is.romix - is.rorayl) * tgtot + is.rorayl * tgp1;
        double ratm3 = is.romix * tgp1;
        double ratm2 = (is.romix - is.rorayl) * tgp2 + is.rorayl * tgp1;
        double romeas1 = ratm1 + rsurf * tgtot;
        double romeas2 = ratm2 + rsurf * tgtot;
        double romeas3 = ratm3 + rsurf * tgtot;

	/* computing integrated values over the spectral band */
        if (iwave.iwave == -2)
	{
	    Output::Begin();
	    ostringstream s;
	    s.setf(ios::fixed, ios::floatfield);
	    s.precision(4);
	    s	<< setw(10) << wl << " "
		<< setw(10) << tgtot << " "
		<< setw(10) << is.dtott << " "
		<< setw(10) << is.utott << " "
		<< setw(10) << is.astot << " "
		<< setw(10) << ratm2 << " "
		<< setprecision(1) << setw(7) << swl << " "
		<< setprecision(4) << setw(10) << step << " "
		<< setw(10) << sbor << " "
		<< setw(10) << geom.dsol << " "
		<< setw(10) << romeas2;
        }

        
	double alumeas = (double)(geom.xmus * swl * romeas2 / M_PI);
        fophsa = fophsa + is.phaa * coef;
        fophsr = fophsr + is.phar * coef;
        sasr = sasr + is.asray * coef;
        sasa = sasa + is.asaer * coef;
        sast = sast + is.astot * coef;
        sroray = sroray + is.rorayl * coef;
        sroaer = sroaer + is.roaero * coef;
        sodray = sodray + is.tray * coef;
        sodaer = sodaer + is.taer * coef;
        sodrayp = sodrayp + is.trayp * coef;
        sodaerp = sodaerp + is.taerp * coef;
        ssdaer = ssdaer + is.tsca * coef;
        sodtot = sodtot + (is.taer + is.tray) * coef;
        sodtotp = sodtotp + (is.taerp + is.trayp) * coef;
        srotot = srotot + is.romix * coef;
        rog = rog + roc * coef;
        refet = refet + romeas2 * coef;
        refet1 = refet1 + romeas1 * coef;
        refet2 = refet2 + romeas2 * coef;
        refet3 = refet3 + romeas3 * coef;
        alumet = alumet + alumeas * sbor * step;
        tgasm = tgasm + tgtot * coef;
        dgasm = dgasm + dgtot * coef;
        ugasm = ugasm + ugtot * coef;
        sdwava = sdwava + as.dtwava * coef;
        sdozon = sdozon + as.dtozon * coef;
        sddica = sddica + as.dtdica * coef;
        sdoxyg = sdoxyg + as.dtoxyg * coef;
        sdniox = sdniox + as.dtniox * coef;
        sdmeth = sdmeth + as.dtmeth * coef;
        sdmoca = sdmoca + as.dtmoca * coef;
        suwava = suwava + as.utwava * coef;
        suozon = suozon + as.utozon * coef;
        sudica = sudica + as.utdica * coef;
        suoxyg = suoxyg + as.utoxyg * coef;
        suniox = suniox + as.utniox * coef;
        sumeth = sumeth + as.utmeth * coef;
        sumoca = sumoca + as.utmoca * coef;
        stwava = stwava + as.ttwava * coef;
        stozon = stozon + as.ttozon * coef;
        stdica = stdica + as.ttdica * coef;
        stoxyg = stoxyg + as.ttoxyg * coef;
        stniox = stniox + as.ttniox * coef;
        stmeth = stmeth + as.ttmeth * coef;
        stmoca = stmoca + as.ttmoca * coef;
        sdtotr = sdtotr + is.dtotr * coef;
        sdtota = sdtota + is.dtota * coef;
        sdtott = sdtott + is.dtott * coef;
        sutotr = sutotr + is.utotr * coef;
        sutota = sutota + is.utota * coef;
        sutott = sutott + is.utott * coef;
        sb = sb + sbor * step;
        seb = seb + coef;

	/* output at the ground level. */
        double tdir = (double)exp(-(is.tray + is.taer) / geom.xmus);
        double tdif = is.dtott - tdir;
        double etn = is.dtott * dgtot / (1 - avr * is.astot);
        double esn = tdir * dgtot;
        double es = (double)(tdir * dgtot * geom.xmus * swl);
        double ea0n = tdif * dgtot;
        double ea0 = (double)(tdif * dgtot * geom.xmus * swl);
        double ee0n = dgtot * avr * is.astot * is.dtott / (1 - avr * is.astot);
        double ee0 = (double)(geom.xmus * swl * dgtot * avr * is.astot * is.dtott / (1 - avr * is.astot));

        if (etn > accu3)
	{
	    ani[0][0] = esn / etn;
	    ani[0][1] = ea0n / etn;
	    ani[0][2] = ee0n / etn;
	}
        else
	{
	    ani[0][0] = 0;
	    ani[0][1] = 0;
	    ani[0][2] = 0;
        }

        ani[1][0] = es;
        ani[1][1] = ea0;
        ani[1][2] = ee0;


	for(j = 0; j < 3; j++)
	{
	    aini[0][j] = aini[0][j] + ani[0][j] * coef;
	    aini[1][j] = aini[1][j] + ani[1][j] * sbor * step;
	}

	/* output at satellite level */
        double tmdir = (double)exp(-(is.tray + is.taer) / geom.xmuv);
        double tmdif = is.utott - tmdir;
        double xla0n = ratm2;
        double xla0 = (double)(xla0n * geom.xmus * swl / M_PI);
        double xltn = roc * is.dtott * tmdir * tgtot / (1 - avr * is.astot);
        double xlt = (double)(xltn * geom.xmus * swl / M_PI);
        double xlen = avr * is.dtott * tmdif * tgtot / (1 - avr * is.astot);
        double xle = (double)(xlen * geom.xmus * swl / M_PI);
        anr[0][0] = xla0n;
        anr[0][1] = xlen;
        anr[0][2] = xltn;
        anr[1][0] = xla0;
        anr[1][1] = xle;
        anr[1][2] = xlt;

	for(j = 0; j < 3; j++)
	{
	    ainr[0][j] = ainr[0][j] + anr[0][j] * coef;
	    ainr[1][j] = ainr[1][j] + anr[1][j] * sbor * step;
	}
    }

    if (seb < accu3)
	G_warning("compute(): variable seb is too small: %g", seb);
    if (sb < accu3)
	G_warning("compute(): variable sb is too small: %g", sb);

    /* ---- integrated values of apparent reflectance, radiance          ----*/
    /* ---- and gaseous transmittances (total,downward,separately gases) ----*/
    refet = refet / seb;
    refet1 = refet1 / seb;
    refet2 = refet2 / seb;
    refet3 = refet3 / seb;
    tgasm = tgasm / seb;
    dgasm = dgasm / seb;
    ugasm = ugasm / seb;
    sasa = sasa / seb;
    sasr = sasr / seb;
    sast = sast / seb;
    sdniox = sdniox / seb;
    sdmoca = sdmoca / seb;
    sdmeth = sdmeth / seb;
    sdwava = sdwava / seb;
    sdozon = sdozon / seb;
    sddica = sddica / seb;
    suniox = suniox / seb;
    sumoca = sumoca / seb;
    sumeth = sumeth / seb;
    suwava = suwava / seb;
    suozon = suozon / seb;
    sudica = sudica / seb;
    suoxyg = suoxyg / seb;
    sdoxyg = sdoxyg / seb;
    stniox = stniox / seb;
    stmoca = stmoca / seb;
    stmeth = stmeth / seb;
    stwava = stwava / seb;
    stozon = stozon / seb;
    stdica = stdica / seb;
    stoxyg = stoxyg / seb;
    sdtotr = sdtotr / seb;
    sdtota = sdtota / seb;
    sdtott = sdtott / seb;

    sutotr = sutotr / seb;
    sutota = sutota / seb;
    sutott = sutott / seb;
    rog = rog / seb;
    sroray = sroray / seb;
    sroaer = sroaer / seb;
    srotot = srotot / seb;
    alumet = alumet / sb;
    /*
    double pizera = 0.0f;
    if(aero.iaer != 0) pizera = ssdaer / sodaer;
    */
    sodray = sodray / seb;
    sodaer = sodaer / seb;
    sodtot = sodtot / seb;
    sodrayp = sodrayp / seb;
    sodaerp = sodaerp / seb;
    sodtotp = sodtotp / seb;
    fophsa = fophsa / seb;
    fophsr = fophsr / seb;

    for(j = 0; j < 3; j++)

    {
        aini[0][j] = aini[0][j] / seb;
        ainr[0][j] = ainr[0][j] / seb;
        aini[1][j] = aini[1][j] / sb;
        ainr[1][j] = ainr[1][j] / sb;
    }

    /* Prepare data for final dn transformation */
    TransformInput ti;
    ti.iwave = iwave.iwave;
    ti.asol = geom.asol;
    memcpy(ti.ainr, ainr, sizeof(ainr));
    ti.sb = sb;
    ti.seb = seb;
    ti.tgasm = tgasm;
    ti.sutott = sutott;
    ti.sdtott = sdtott;
    ti.sast = sast;
    ti.srotot = srotot;
    ti.xmus = geom.xmus;

 
    return ti;
}

