#include <cstring>

extern "C" {
#include <grass/gis.h>
#include <grass/glocale.h>
}

#include "6s.h"
#include "common.h"
#include "GeomCond.h"
#include "AtmosModel.h"
#include "AerosolModel.h"
#include "AerosolConcentration.h"
#include "Altitude.h"
#include "Iwave.h"
#include "Transform.h"
#include "Abstra.h"
#include "Interp.h"
#include "Output.h"

/* Function prototypes */
extern void discom(const GeomCond &geom, const AtmosModel &atms,
                   const AerosolModel &aero, const AerosolConcentration &aerocon,
                   const Altitude &alt, const IWave &iwave);
extern void specinterp(const float wl, float& tamoy, float& tamoyp, float& pizmoy, float& pizmoyp,
                       const AerosolConcentration &aerocon, const Altitude &alt);
extern void enviro (const float difr, const float difa, const float r, const float palt,
		    const float xmuv, float& fra, float& fae, float& fr);
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
	c here, we first compute an equivalent wavelenght which is the input   c
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
    float wlmoy;
    if(iwave.iwave != -1) wlmoy = iwave.equivwl();
    else wlmoy = iwave.wl;

    iwave.wlmoy = wlmoy;

    discom(geom, atms, aero, aerocon, alt, iwave);
    float tamoy, tamoyp, pizmoy, pizmoyp;
    if(aero.iaer != 0) specinterp(wlmoy, tamoy, tamoyp, pizmoy, pizmoyp, aerocon, alt);

    printOutput();
    fflush(stderr);
    return 0;
}


/* Only update those objects that are affected by a height and vis change */
void pre_compute_hv(const float height, const float vis)
{
    atms = original_atms;
    aerocon.set_visibility(vis, atms);
    alt.set_height(height);
    alt.init(atms, aerocon);
   
    float wlmoy = iwave.wlmoy;

    discom(geom, atms, aero, aerocon, alt, iwave);
    float tamoy, tamoyp, pizmoy, pizmoyp;
    if(aero.iaer != 0) specinterp(wlmoy, tamoy, tamoyp, pizmoy, pizmoyp, aerocon, alt);
}

/* Only update those objects that are affected by a visibility change */
void pre_compute_v(const float vis)
{
    atms = original_atms;
    aerocon.set_visibility(vis, atms);
    alt.init(atms, aerocon);

    float wlmoy = iwave.wlmoy;

    discom(geom, atms, aero, aerocon, alt, iwave);
    float tamoy, tamoyp, pizmoy, pizmoyp;
    if(aero.iaer != 0) specinterp(wlmoy, tamoy, tamoyp, pizmoy, pizmoyp, aerocon, alt);
}

/* Only update those objects that are affected by a height change */
void pre_compute_h(const float height)
{
    atms = original_atms;
    alt.set_height(height);
    alt.init(atms, aerocon);

    float wlmoy = iwave.wlmoy;

    discom(geom, atms, aero, aerocon, alt, iwave);
    float tamoy, tamoyp, pizmoy, pizmoyp;
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

    float rocave = 0;       /* block of code in Fortran will always compute 0 */
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

	ostringstream s;
	s.setf(ios::fixed, ios::floatfield);
	s << setprecision(3);
	s << " uh2o=" << setw(9) << atms.uw << " g/cm2      "
	  << "  uo3=" << setw(9) << atms.uo3 << " cm-atm" << ends;
	Output::WriteLn(15,s.str());
    }

    alt.print();

    /* ---- atmospheric correction  ---- */
    Output::Ln();
    Output::WriteLn(23," atmospheric correction activated ");
    Output::WriteLn(23," -------------------------------- ");
}

TransformInput compute()
{
    const float accu3 = 1e-07;
/* ---- initilialization	 very liberal :) */
    int i, j;

    float fr = 0;
    float rad = 0;
    float sb = 0;
    float seb = 0;
    float refet = 0;
    float refet1 = 0;
    float refet2 = 0;
    float refet3 = 0;
    float alumet = 0;
    float tgasm = 0;
    float rog = 0;
    float dgasm = 0;
    float ugasm = 0;
    float sdwava = 0;
    float sdozon = 0;
    float sddica = 0;
    float sdoxyg = 0;
    float sdniox = 0;
    float sdmoca = 0;
    float sdmeth = 0;

    float suwava = 0;
    float suozon = 0;
    float sudica = 0;
    float suoxyg = 0;
    float suniox = 0;
    float sumoca = 0;
    float sumeth = 0;
    float stwava = 0;
    float stozon = 0;
    float stdica = 0;
    float stoxyg = 0;
    float stniox = 0;
    float stmoca = 0;
    float stmeth = 0;
    float sodray = 0;
    float sodrayp = 0;
    float sodaer = 0;
    float sodaerp = 0;
    float sodtot = 0;
    float sodtotp = 0;
    float fophsr = 0;
    float fophsa = 0;
    float sroray = 0;
    float sroaer = 0;
    float srotot = 0;
    float ssdaer = 0;
    float sdtotr = 0;
    float sdtota = 0;
    float sdtott = 0;
    float sutotr = 0;
    float sutota = 0;
    float sutott = 0;
    float sasr = 0;
    float sasa = 0;
    float sast = 0;

    float ani[2][3];
    float aini[2][3];
    float anr[2][3];
    float ainr[2][3];

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
        float sbor = iwave.ffu.s[l];

        if(l == iwave.iinf || l == iwave.isup) sbor *= 0.5f;
        if(iwave.iwave == -1) sbor = 1.0f / step;

        float roc = 0; /* rocl[l]; */
        float roe = 0; /* roel[l]; */
        float wl = 0.25f + l * step;

	AbstraStruct as;
	float uwus, uo3us;		/* initialized in abstra */

	abstra(atms, alt, wl, (float)geom.xmus, (float)geom.xmuv, atms.uw / 2.0f, atms.uo3,
	       uwus, uo3us, alt.puw / 2.0f, alt.puo3, alt.puwus, alt.puo3us, as);

	float attwava = as.ttwava;

	abstra(atms, alt, wl, (float)geom.xmus, (float)geom.xmuv, atms.uw, atms.uo3,
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

        float swl = iwave.solirr(wl);
        swl = swl * geom.dsol;
        float coef = sbor * step * swl;

	InterpStruct is;
	memset(&is, 0, sizeof(is));
	interp(aero.iaer, alt.idatmp, wl, aerocon.taer55, alt.taer55p, (float)geom.xmud, is);


        float dgtot = as.dtwava * as.dtozon * as.dtdica * as.dtoxyg * as.dtniox * as.dtmeth * as.dtmoca;
        float tgtot = as.ttwava * as.ttozon * as.ttdica * as.ttoxyg * as.ttniox * as.ttmeth * as.ttmoca;
        float ugtot = as.utwava * as.utozon * as.utdica * as.utoxyg * as.utniox * as.utmeth * as.utmoca;
        float tgp1 = as.ttozon * as.ttdica * as.ttoxyg * as.ttniox * as.ttmeth * as.ttmoca;
        float tgp2 = attwava * as.ttozon * as.ttdica * as.ttoxyg * as.ttniox * as.ttmeth * as.ttmoca;
        float edifr = (float)(is.utotr - exp(-is.trayp / geom.xmuv));
        float edifa = (float)(is.utota - exp(-is.taerp / geom.xmuv));


	float fra, fae;
	enviro(edifr, edifa, rad, alt.palt, (float)geom.xmuv, fra, fae, fr);

	float avr = roc * fr + (1 - fr) * roe;
	float rsurf = (float)(roc * is.dtott * exp(-(is.trayp + is.taerp) / geom.xmuv) / (1 - avr * is.astot)
			      + avr * is.dtott * (is.utott - exp(-(is.trayp + is.taerp) / geom.xmuv)) / (1 - avr * is.astot));
        float ratm1 = (is.romix - is.rorayl) * tgtot + is.rorayl * tgp1;
        float ratm3 = is.romix * tgp1;
        float ratm2 = (is.romix - is.rorayl) * tgp2 + is.rorayl * tgp1;
        float romeas1 = ratm1 + rsurf * tgtot;
        float romeas2 = ratm2 + rsurf * tgtot;
        float romeas3 = ratm3 + rsurf * tgtot;

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

        
	float alumeas = (float)(geom.xmus * swl * romeas2 / M_PI);
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
        float tdir = (float)exp(-(is.tray + is.taer) / geom.xmus);
        float tdif = is.dtott - tdir;
        float etn = is.dtott * dgtot / (1 - avr * is.astot);
        float esn = tdir * dgtot;
        float es = (float)(tdir * dgtot * geom.xmus * swl);
        float ea0n = tdif * dgtot;
        float ea0 = (float)(tdif * dgtot * geom.xmus * swl);
        float ee0n = dgtot * avr * is.astot * is.dtott / (1 - avr * is.astot);
        float ee0 = (float)(geom.xmus * swl * dgtot * avr * is.astot * is.dtott / (1 - avr * is.astot));

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
        float tmdir = (float)exp(-(is.tray + is.taer) / geom.xmuv);
        float tmdif = is.utott - tmdir;
        float xla0n = ratm2;
        float xla0 = (float)(xla0n * geom.xmus * swl / M_PI);
        float xltn = roc * is.dtott * tmdir * tgtot / (1 - avr * is.astot);
        float xlt = (float)(xltn * geom.xmus * swl / M_PI);
        float xlen = avr * is.dtott * tmdif * tgtot / (1 - avr * is.astot);
        float xle = (float)(xlen * geom.xmus * swl / M_PI);
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
    float pizera = 0.0f;
    if(aero.iaer != 0) pizera = ssdaer / sodaer;
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

