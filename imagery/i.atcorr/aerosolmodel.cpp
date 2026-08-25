extern "C" {
#include <grass/gis.h>
#include <grass/glocale.h>
}

#include "common.h"
#include "aerosolmodel.h"
#include "atmosmodel.h"
#ifdef WIN32
#pragma warning(disable : 4305) /* disable warning about initialization of a \
                                   double by a double */
#endif                          /* WIN32 */

/* (background desert model...) */
void AerosolModel::bdm()
{
    sixs_aerbas.ph = &sixs_aerbas.bdm_ph;
}

/* (biomass burning model...) */
void AerosolModel::bbm()
{
    sixs_aerbas.ph = &sixs_aerbas.bbm_ph;
}

/* (stratospherique aerosol model...) */
void AerosolModel::stm()
{
    sixs_aerbas.ph = &sixs_aerbas.stm_ph;
}

/* dust model */
void AerosolModel::dust()
{
    sixs_aerbas.ph = &(sixs_aerbas.dust_ph);
}

/* water model */
void AerosolModel::wate()
{
    sixs_aerbas.ph = &(sixs_aerbas.wate_ph);
}

/* ocean model */
void AerosolModel::ocea()
{
    sixs_aerbas.ph = &sixs_aerbas.ocea_ph;
}

/* soot model */
void AerosolModel::soot()
{
    sixs_aerbas.ph = &sixs_aerbas.soot_ph;
}

/* (user defined model from size distribution) */
/*        To compute, using the scattering of electromagnetic waves by a
  homogeneous isotropic sphere, the physical properties of particles whose sizes
  are comparable to or larger than the wavelength, and to generate mixture of
  dry particles. */
void AerosolModel::mie(double (&ex)[4][10], double (&sc)[4][10],
                       double (&asy)[4][10])
{
    double np[4];
    double ext[10][4];
    double sca2[10][4];
    double p1[10][4][83];

    const double rmul =
        0.99526231496887960135245539673954; /*rlogpas = 0.030;
                                               (10**rlogpas-1.D+00)*/

    int i;
    for (i = 0; i < mie_in.icp; i++) {

        np[i] = 0;
        for (int j = 0; j < 10; j++) {
            ex[i][j] = 0;
            sc[i][j] = 0;
            ext[j][i] = 0;
            sca2[j][i] = 0;
            for (int k = 0; k < 83; k++)
                p1[j][i][k] = 0;
        }
    }

    double r;
    double dr;
    double nr = 0;
    /* LOOPS ON THE NUMBER OF PARTICLE TYPE (4 max) */
    for (i = 0; i < mie_in.icp; i++) {
        r = mie_in.rmin;
        dr = r * rmul;

        /* LOOPS ON THE RADIUS OF THE PARTICLE */
        do {

            /* call of the size distribution nr. For our computation, we need
             * dn/dr for */
            /* all functions except for sun-photometer inputs for which we need
             * dV/dlog(r) */

            switch (iaer - 7) {
            case 1: {
                /* --- Mixture of particles (Log-Normal distribution functions,
                 * up to 5)*/
                const double sqrt2PI = 2.506628274631000502415765284811;
                const double ln10 = 2.3025850929940456840179914546844;
                double log10_x2 = log10(mie_in.x2[i]);
                double sq = log10(r / mie_in.x1[i]) / log10_x2;
                nr = exp(-0.5 * sq * sq);
                nr /= sqrt2PI * log10_x2 * ln10 * r;
                break;
            }
            case 2: {
                /* --- Modified Gamma distribution function */
                const double ldexp = -300.;
                double arg = -mie_in.x2[i] * pow(r, (double)mie_in.x3[i]);
                if (arg > ldexp)
                    nr = pow(r, (double)mie_in.x1[i]) * exp(arg);
                else
                    nr = 0;
                break;
            }
            case 3: {
                /* --- Junge power-law function */
                nr = pow(0.1, -(double)mie_in.x1[i]);
                if (r > 0.1)
                    nr = pow(r, -(double)mie_in.x1[i]);
                break;
            }
            case 4: {
                /* --- from sun photometer */
                nr = 0;
                for (int j = 1; j < mie_in.irsunph; j++)
                    if ((r - mie_in.rsunph[j]) < 0.000001) {
                        nr = (r - mie_in.rsunph[j - 1]) /
                             (mie_in.rsunph[j] - mie_in.rsunph[j - 1]);
                        nr = mie_in.nrsunph[j - 1] +
                             nr * (mie_in.nrsunph[j] - mie_in.nrsunph[j - 1]);
                        break;
                    }
            }
            }

            /* The Mie's calculations have to be called several times (min=2,
               max=10 for each type of particle): at wavelengths bounding the
               range of the selected wavelengths,and at 0.550 microns to
               normalized the extinction coefficient (if it's not in the
               selected range of wavelengths). */

            double xndpr2 = nr * dr * M_PI * (r * r);
            /* relatif number of particle for each type of particle (has to be
             * equal to 1) */
            np[i] += nr * dr;

            for (int j = 0; j < 10; j++) {
                if ((xndpr2 * mie_in.cij[i]) <
                    (1e-8 / sqrt(sixs_disc.wldis[j])))
                    break;

                double alpha = 2. * M_PI * r / sixs_disc.wldis[j];
                double Qext, Qsca;
                double p11[83];
                exscphase(alpha, mie_in.rn[j][i], mie_in.ri[j][i], Qext, Qsca,
                          p11);
                ext[j][i] += xndpr2 * Qext;
                sca2[j][i] += xndpr2 * Qsca;

                /* phase function for each type of particle */
                for (int k = 0; k < 83; k++)
                    p1[j][i][k] += p11[k] * xndpr2;
            }

            r += dr;
            dr = r * rmul;
        } while (r < mie_in.rmax);
    }

    /* NOW WE MIXTE THE DIFFERENT TYPES OF PARTICLE
       computation of the scattering and extinction coefficients. We first start
       at 0.550 micron (the extinction coefficient is normalized at 0.550
       micron) */
    int j;
    for (j = 0; j < 10; j++)
        for (i = 0; i < mie_in.icp; i++) {
            ext[j][i] /= np[i] * 1000;
            sca2[j][i] /= np[i] * 1000;
            ex[0][j] += (double)(mie_in.cij[i] * ext[j][i]);
            sc[0][j] += (double)(mie_in.cij[i] * sca2[j][i]);
        }

    /* computation of the phase function and the asymmetry coefficient
       of the mixture of particles */

    for (j = 0; j < 10; j++) {
        double asy_n = 0;
        double asy_d = 0;

        for (int k = 0; k < 83; k++) {
            sixs_aerbas.usr_ph[j][k] = 0;
            for (i = 0; i < mie_in.icp; i++)
                sixs_aerbas.usr_ph[j][k] +=
                    (double)(mie_in.cij[i] * p1[j][i][k] / np[i] / 1000);

            sixs_aerbas.usr_ph[j][k] += (double)sc[0][j];
            asy_n += sixs_sos.cgaus[k] * sixs_aerbas.usr_ph[j][k] *
                     sixs_sos.pdgs[k] / 10.;
            asy_d += sixs_aerbas.usr_ph[j][k] * sixs_sos.pdgs[k] / 10.;
        }

        asy[0][j] = (double)(asy_n / asy_d);
    }

    sixs_aerbas.ph = &sixs_aerbas.usr_ph;
}

/***************************************************************************
  Using the Mie's theory, this subroutine compute the scattering and
  extinction efficiency factors (usually written Qsca and Qext) and it also
  compute the scattering intensity efficiency
***************************************************************************/
void AerosolModel::exscphase(const double X, const double nr, const double ni,
                             double &Qext, double &Qsca, double (&p11)[83])
{
    double Ren = nr / (nr * nr + ni * ni);
    double Imn = ni / (nr * nr + ni * ni);

    /* ---Identification of the greater order of computation (=mu)
       as defined by F.J. Corbato, J. Assoc. Computing Machinery, 1959,
       6, 366-375 */
    int N = int(0.5 * (-1 + sqrt(1 + 4 * X * X))) + 1;
    if (N == 1)
        N = 2;

    int mu2 = 1000000;
    double Up = 2 * X / (2 * N + 1);
    int mu1 = int(N + 30. * (0.1 + 0.35 * Up * (2. - Up * Up) / 2 / (1 - Up)));
    int Np = int(X - 0.5 * sqrt(30. * 0.35 * X));

    if (Np > N) {
        Up = 2 * X / (2 * Np + 1);
        mu2 = int(Np + 30. * (0.1 + 0.35 * Up * (2 - Up * Up) / 2 / (1 - Up)));
    }

    int mu = (mu1 < mu2) ? mu1 : mu2; /* min(mu1, mu2) */

    /* --- Identification of the transition line. Below this line the Bessel
       function j behaves as oscillating functions. Above the behavior
       becomes monotonic. We start at a order greater than this transition
       line (order max=mu) because a downward recursion is called for. */

    double Rn[10001], xj[10001];
    int k = mu + 1;

    Rn[mu] = 0;
    int mub;
    while (true) {
        k--;
        xj[k] = 0;
        Rn[k - 1] = X / (2 * k + 1 - X * Rn[k]);

        if (k == 2) {
            xj[mu + 1] = 0;
            xj[mu] = 1;
            mub = mu;
            break;
        }

        if (Rn[k - 1] > 1) {
            xj[k] = Rn[k - 1];
            xj[k - 1] = 1;
            mub = k - 1;
            break;
        }
    }

    for (k = mub; k >= 1; k--)
        xj[k - 1] = (2 * k + 1) * xj[k] / X - xj[k + 1];
    double coxj = xj[0] - X * xj[1] * cos(X) + X * xj[0] * sin(X);

    /* --- Computation Dn(alpha) and Dn(alpha*m) (cf MIE's theory)
       downward recursion    - real and imaginary parts */

    double RDnY[10001];
    double IDnY[10001];
    double RDnX[10001];
    RDnY[mu] = 0;
    IDnY[mu] = 0;
    RDnX[mu] = 0;

    for (k = mu; k >= 1; k--) {
        RDnX[k - 1] = k / X - 1 / (RDnX[k] + k / X);
        double XnumRDnY = RDnY[k] + Ren * k / X;
        double XnumIDnY = IDnY[k] + Imn * k / X;
        double XdenDnY = XnumRDnY * XnumRDnY + XnumIDnY * XnumIDnY;
        RDnY[k - 1] = k * Ren / X - XnumRDnY / XdenDnY;
        IDnY[k - 1] = k * Imn / X + XnumIDnY / XdenDnY;
    }

    /* --- Initialization of the upward recursions
       macro to help keep indexing correct, can't be to safe */
#define INDEX(X) ((X) + 1)
    double xy[10002];
    xy[INDEX(-1)] = sin(X) / X;
    xy[INDEX(0)] = -cos(X) / X;

    double RGnX[10001];
    double IGnX[10001];
    RGnX[0] = 0;
    IGnX[0] = -1;
    Qsca = 0;
    Qext = 0;

    double RAn[10001];
    double IAn[10001];
    double RBn[10001];
    double IBn[10001];

    for (k = 1; k <= mu; k++) {
        if (k <= mub)
            xj[k] /= coxj;
        else
            xj[k] = Rn[k - 1] * xj[k - 1];

        /* --- Computation of bessel's function y(alpha) */
        xy[INDEX(k)] = (2 * k - 1) * xy[INDEX(k - 1)] / X - xy[INDEX(k - 2)];
        double xJonH = xj[k] / (xj[k] * xj[k] + xy[INDEX(k)] * xy[INDEX(k)]);

        /*  --- Computation of Gn(alpha), Real and Imaginary part */
        double XdenGNX = (RGnX[k - 1] - k / X) * (RGnX[k - 1] - k / X) +
                         IGnX[k - 1] * IGnX[k - 1];
        RGnX[k] = (k / X - RGnX[k - 1]) / XdenGNX - k / X;
        IGnX[k] = IGnX[k - 1] / XdenGNX;

        /* --- Computation of An(alpha) and Bn(alpha), Real and Imaginary part
         */
        double Xnum1An = RDnY[k] - nr * RDnX[k];
        double Xnum2An = IDnY[k] + ni * RDnX[k];
        double Xden1An = RDnY[k] - nr * RGnX[k] - ni * IGnX[k];
        double Xden2An = IDnY[k] + ni * RGnX[k] - nr * IGnX[k];
        double XdenAn = Xden1An * Xden1An + Xden2An * Xden2An;
        double RAnb = (Xnum1An * Xden1An + Xnum2An * Xden2An) / XdenAn;
        double IAnb = (-Xnum1An * Xden2An + Xnum2An * Xden1An) / XdenAn;
        RAn[k] = xJonH * (xj[k] * RAnb - xy[INDEX(k)] * IAnb);
        IAn[k] = xJonH * (xy[INDEX(k)] * RAnb + xj[k] * IAnb);

        double Xnum1Bn = nr * RDnY[k] + ni * IDnY[k] - RDnX[k];
        double Xnum2Bn = nr * IDnY[k] - ni * RDnY[k];
        double Xden1Bn = nr * RDnY[k] + ni * IDnY[k] - RGnX[k];
        double Xden2Bn = nr * IDnY[k] - ni * RDnY[k] - IGnX[k];
        double XdenBn = Xden1Bn * Xden1Bn + Xden2Bn * Xden2Bn;
        double RBnb = (Xnum1Bn * Xden1Bn + Xnum2Bn * Xden2Bn) / XdenBn;
        double IBnb = (-Xnum1Bn * Xden2Bn + Xnum2Bn * Xden1Bn) / XdenBn;
        RBn[k] = xJonH * (xj[k] * RBnb - xy[INDEX(k)] * IBnb);
        IBn[k] = xJonH * (xy[INDEX(k)] * RBnb + xj[k] * IBnb);

        /* ---Criterion on the recursion formulas as defined by D. Deirmendjian
           et al., J. Opt. Soc. Am., 1961, 51, 6, 620-633 */
        double temp = RAn[k] * RAn[k] + IAn[k] * IAn[k] + RBn[k] * RBn[k] +
                      IBn[k] * IBn[k];
        if ((temp / k) < 1e-14) {
            mu = k;
            break;
        }

        /* --- Computation of the scattering and extinction efficiency factor */
        double xpond = 2 / X / X * (2 * k + 1);
        Qsca = Qsca + xpond * temp;
        Qext = Qext + xpond * (RAn[k] + RBn[k]);
    }

    /* --- Computation of the amplitude functions S1 and S2 (cf MIE's theory)
       defined by PIn, TAUn, An and Bn with PIn and TAUn related to the
       Legendre polynomials. */
    for (int j = 0; j < 83; j++) {
        double RS1 = 0;
        double RS2 = 0;
        double IS1 = 0;
        double IS2 = 0;
        double PIn[10001];
        double TAUn[10001];

        PIn[0] = 0;
        PIn[1] = 0;
        TAUn[1] = sixs_sos.cgaus[j];

        for (k = 1; k <= mu; k++) {
            double co_n = (2.0 * k + 1) / k / (k + 1);
            RS1 += co_n * (RAn[k] * PIn[k] + RBn[k] * TAUn[k]);
            RS2 += co_n * (RAn[k] * TAUn[k] + RBn[k] * PIn[k]);
            IS1 += co_n * (IAn[k] * PIn[k] + IBn[k] * TAUn[k]);
            IS2 += co_n * (IAn[k] * TAUn[k] + IBn[k] * PIn[k]);

            PIn[k + 1] = ((2 * k + 1) * sixs_sos.cgaus[j] * PIn[k] -
                          (k + 1) * PIn[k - 1]) /
                         k;
            TAUn[k + 1] =
                (k + 1) * sixs_sos.cgaus[j] * PIn[k + 1] - (k + 2) * PIn[k];
        }

        /* --- Computation of the scattering intensity efficiency */
        p11[j] = 2 * (RS1 * RS1 + IS1 * IS1 + RS2 * RS2 + IS2 * IS2) / X / X;
    }
}

/* load parameters from .mie file */
void AerosolModel::load()
{
    int i;
    ifstream in(filename.c_str());
    cin.ignore(numeric_limits<int>::max(), '\n'); /* ignore this line */

    in.ignore(8);
    for (i = 0; i < 10; i++) {
        in.ignore(3);
        in >> sixs_aer.ext[i];
        in.ignore(6);
        in >> sca[i];
        in.ignore(6);
        in >> sixs_aer.ome[i];
        in.ignore(6);
        in >> sixs_aer.gasym[i];
        in.ignore(3);
        cin.ignore(numeric_limits<int>::max(), '\n'); /* ignore the rest */
    }

    /* ignore 3 lines */
    cin.ignore(numeric_limits<int>::max(), '\n'); /* ignore this line */
    cin.ignore(numeric_limits<int>::max(), '\n'); /* ignore this line */
    cin.ignore(numeric_limits<int>::max(), '\n'); /* ignore this line */

    for (i = 0; i < 83; i++) {
        in.ignore(8);
        for (int j = 0; j < 10; j++) {
            in.ignore(1);
            in >> sixs_sos.phasel[j][i];
        }
        cin.ignore(numeric_limits<int>::max(), '\n'); /* ignore the rest */
    }
}

/* do we wish to save this? */
void AerosolModel::save()
{
    ofstream out(filename.c_str());
    /* output header */
    out << "   Wlgth  Nor_Ext_Co  Nor_Sca_Co  Sg_Sca_Alb  Asymm_Para  "
           "Extinct_Co  Scatter_Co"
        << endl;
    int i;
    for (i = 0; i < 10; i++) {
        out << setprecision(4); /* set the required precision */
        out << "  " << setw(10) << sixs_disc.wldis[0] << "   " << setw(10)
            << sixs_aer.ext[i] << "      " << setw(10) << sca[i] << "      "
            << setw(10) << sixs_aer.ome[i] << "      " << setw(10)
            << sixs_aer.gasym[i] << "      " << setw(10)
            << sixs_aer.ext[i] / nis << "      " << setw(10) << sca[i] / nis
            << endl;
    }

    out << endl
        << endl
        << setw(20) << " "
        << " Phase Function " << endl;
    out << "   TETA ";
    for (i = 0; i < 10; i++)
        out << "   " << setw(10) << sixs_disc.wldis[i] << "  ";
    out << endl;

    for (i = 0; i < 83; i++) {
        out << setprecision(2);
        out << "  " << setw(8) << (180. * acos(sixs_sos.cgaus[i]) / M_PI);

        out << setprecision(4);
        out.setf(ios::scientific, ios::floatfield);
        for (int j = 0; j < 10; j++)
            out << " " << setw(14) << sixs_sos.phasel[j][i];
        out.setf(ios::fixed, ios::floatfield);
        out << endl;
    }
}

/*
  To compute the optical scattering parameters (extinction and scattering
  coefficients, single scattering albedo, phase function, asymmetry factor)
  at the ten discrete wavelengths for the selected model (or created model)
  from:
  (1) the characteristics of the basic components of the International
  Radiation Commission. (1983).
  dust-like component (D.L., SUBROUTINE DUST)
  oceanic component (O.C., SUBROUTINE OCEA)
  water-soluble component (W.S., SUBROUTINE WATE)
  soot component (S.O., SUBROUTINE SOOT)
  (2) pre-computed characteristics, now available are the desertic aerosol model
  corresponding to background conditions, as described in Shettle(1984), a
  stratospheric aerosol model as measured Mona Loa (Hawaii) during El Chichon
  eruption and as described by King et al. (1984), and a biomass burning aerosol
  model as deduced from measurements taken by sunphotometers in Amazonia.
  (SUBROUTINES BDM, STM and BBM)
  (3) computed using the MIE theory with inputs (size distribution, refractive
  indexes...) given by the user (see SUBROUTINES MIE and EXSCPHASE).
  These models don't correspond to a mixture of the four basic components.
*/
void AerosolModel::aeroso(const double xmud)
{
    /* sra basic components for aerosol model, extinction coefficients are */
    /* in km-1. */
    /*     dust-like = 1 */
    /*     water-soluble = 2 */
    /*     oceanique = 3 */
    /*     soot = 4 */
    static const double vi[4] = {113.983516, 1.13983516e-4, 5.1444150196,
                                 5.977353425e-5};
    static const double ni[4] = {54.734, 1868550., 276.05, 1805820.};

    /* i: 1=dust-like 2=water-soluble 3=oceanic 4=soot */
    static const double s_ex[4][10] = {
        {0.1796674e-01, 0.1815135e-01, 0.1820247e-01, 0.1827016e-01,
         0.1842182e-01, 0.1853081e-01, 0.1881427e-01, 0.1974608e-01,
         0.1910712e-01, 0.1876025e-01},
        {0.7653460e-06, 0.6158538e-06, 0.5793444e-06, 0.5351736e-06,
         0.4480091e-06, 0.3971033e-06, 0.2900993e-06, 0.1161433e-06,
         0.3975192e-07, 0.1338443e-07},
        {0.3499458e-02, 0.3574996e-02, 0.3596592e-02, 0.3622467e-02,
         0.3676341e-02, 0.3708866e-02, 0.3770822e-02, 0.3692255e-02,
         0.3267943e-02, 0.2801670e-02},
        {0.8609083e-06, 0.6590103e-06, 0.6145787e-06, 0.5537643e-06,
         0.4503008e-06, 0.3966041e-06, 0.2965532e-06, 0.1493927e-06,
         0.1017134e-06, 0.6065031e-07}};

    static const double s_sc[4][10] = {
        {0.1126647e-01, 0.1168918e-01, 0.1180978e-01, 0.1196792e-01,
         0.1232056e-01, 0.1256952e-01, 0.1319347e-01, 0.1520712e-01,
         0.1531952e-01, 0.1546761e-01},
        {0.7377123e-06, 0.5939413e-06, 0.5587120e-06, 0.5125148e-06,
         0.4289210e-06, 0.3772760e-06, 0.2648252e-06, 0.9331806e-07,
         0.3345499e-07, 0.1201109e-07},
        {0.3499455e-02, 0.3574993e-02, 0.3596591e-02, 0.3622465e-02,
         0.3676338e-02, 0.3708858e-02, 0.3770696e-02, 0.3677038e-02,
         0.3233194e-02, 0.2728013e-02},
        {0.2299196e-06, 0.1519321e-06, 0.1350890e-06, 0.1155423e-06,
         0.8200095e-07, 0.6469735e-07, 0.3610638e-07, 0.6227224e-08,
         0.1779378e-08, 0.3050002e-09}};

    static const double ex2[10] = {43.83631f, 42.12415f, 41.57425f, 40.85399f,
                                   39.1404f,  37.89763f, 34.67506f, 24.59f,
                                   17.96726f, 10.57569f};

    static const double sc2[10] = {40.28625f, 39.04473f, 38.6147f,  38.03645f,
                                   36.61054f, 35.54456f, 32.69951f, 23.41019f,
                                   17.15375f, 10.09731f};

    static const double ex3[10] = {95397.86f, 75303.6f,  70210.64f, 64218.28f,
                                   52430.56f, 45577.68f, 31937.77f, 9637.68f,
                                   3610.691f, 810.5614f};

    static const double sc3[10] = {92977.9f,  73397.17f, 68425.49f, 62571.8f,
                                   51049.87f, 44348.77f, 31006.21f, 9202.678f,
                                   3344.476f, 664.1915f};

    static const double ex4[10] = {
        54273040.f, 61981440.f, 63024320.f, 63489470.f, 61467600.f,
        58179720.f, 46689090.f, 15190620.f, 5133055.f,  899859.4f};

    static const double sc4[10] = {
        54273040.f, 61981440.f, 63024320.f, 63489470.f, 61467600.f,
        58179720.f, 46689090.f, 15190620.f, 5133055.f,  899859.4f};

    static const double s_asy[4][10] = {
        {0.896, 0.885, 0.880, 0.877, 0.867, 0.860, 0.845, 0.836, 0.905, 0.871},
        {0.642, 0.633, 0.631, 0.628, 0.621, 0.616, 0.610, 0.572, 0.562, 0.495},
        {0.795, 0.790, 0.788, 0.781, 0.783, 0.782, 0.778, 0.783, 0.797, 0.750},
        {0.397, 0.359, 0.348, 0.337, 0.311, 0.294, 0.253, 0.154, 0.103, 0.055}};

    static const double asy2[10] = {.718f, .712f, .71f, .708f, .704f,
                                    .702f, .696f, .68f, .668f, .649f};

    static const double asy3[10] = {.704f, .69f,  .686f, .68f,  .667f,
                                    .659f, .637f, .541f, .437f, .241f};
    static const double asy4[10] = {.705f, .744f, .751f, .757f, .762f,
                                    .759f, .737f, .586f, .372f, .139f};

    /* local */
    double coef;
    double sigm;
    double sumni;
    double dd[4][10];
    double pha[5][10][83];

    double ex[4][10];
    double sc[4][10];
    double asy[4][10];

    int i; /* crappy VS6 */
    /* initialize ex, sc & asy */
    for (i = 0; i < 4; i++) {
        int j;
        for (j = 0; j < 10; j++)
            ex[i][j] = s_ex[i][j];
        for (j = 0; j < 10; j++)
            sc[i][j] = s_sc[i][j];
        for (j = 0; j < 10; j++)
            asy[i][j] = s_asy[i][j];
    }

    /* optical properties of aerosol model computed from sra basic comp */
    for (i = 0; i < 10; ++i) {
        if (i == 4 && iaer == 0)
            sixs_aer.ext[i] = 1.f;
        else
            sixs_aer.ext[i] = 0.f;
        sca[i] = 0.f;
        sixs_aer.ome[i] = 0.f;
        sixs_aer.gasym[i] = 0.f;
        sixs_aer.phase[i] = 0.f;

        for (int k = 0; k < 83; ++k)
            sixs_sos.phasel[i][k] = 0.f;
    }

    /* return if iear = 0 */
    if (iaer == 0)
        return;

    /* look for an interval in cgaus */
    long int j1 = -1;
    for (i = 0; i < 82; ++i)
        if (xmud >= sixs_sos.cgaus[i] && xmud < sixs_sos.cgaus[i + 1]) {
            j1 = i;
            break;
        }
    if (j1 == -1)
        return; /* unable to find interval */

    coef = -(xmud - sixs_sos.cgaus[j1]) /
           (sixs_sos.cgaus[j1 + 1] - sixs_sos.cgaus[j1]);

    switch (iaer) {
    case 12: /* read from file */
    {
        load();
        for (i = 0; i < 10; i++)
            sixs_aer.phase[i] = (double)(sixs_sos.phasel[i][j1] +
                                         coef * (sixs_sos.phasel[i][j1] -
                                                 sixs_sos.phasel[i][j1 + 1]));
        return;
    }
    case 5: {
        for (i = 0; i < 10; i++) {
            asy[0][i] = asy2[i];
            ex[0][i] = ex2[i];
            sc[0][i] = sc2[i];
        }
        break;
    }
    case 6: {
        for (i = 0; i < 10; i++) {
            asy[0][i] = asy3[i];
            ex[0][i] = ex3[i];
            sc[0][i] = sc3[i];
        }
        break;
    }
    case 7: {
        for (i = 0; i < 10; i++) {
            asy[0][i] = asy4[i];
            ex[0][i] = ex4[i];
            sc[0][i] = sc4[i];
        }
        break;
    }
    default:;
    }

    if (iaer >= 5 && iaer <= 11) {
        /* calling a special aerosol model */

        switch (iaer) {
            /* (background desert model...) */
        case 5:
            bdm();
            break;
            /* (biomass burning model...) */
        case 6:
            bbm();
            break;
            /* (stratospherique aerosol model...) */
        case 7:
            stm();
            break;

            /* (user defined model from size distribution) */
        case 8:
        case 9:
        case 10:
        case 11:
            mie(ex, sc, asy);
            break;
        }

        for (i = 0; i < 10; i++) {
            dd[0][i] = (*sixs_aerbas.ph)[i][j1] +
                       coef * ((*sixs_aerbas.ph)[i][j1] -
                               (*sixs_aerbas.ph)[i][j1 + 1]);
            for (int k = 0; k < 83; k++)
                pha[0][i][k] = (*sixs_aerbas.ph)[i][k];
        }

        mie_in.icp = 1;
        mie_in.cij[0] = 1.f;
        /* for normalization of the extinction coefficient */
        nis = 1. / ex[0][3];
    }
    else {
        /* calling each sra components */
        mie_in.icp = 4;
        /*  -dust */
        dust();
        for (i = 0; i < 10; i++) {
            dd[0][i] = (*sixs_aerbas.ph)[i][j1] +
                       coef * ((*sixs_aerbas.ph)[i][j1] -
                               (*sixs_aerbas.ph)[i][j1 + 1]);
            for (int k = 0; k < 83; k++)
                pha[0][i][k] = ((*sixs_aerbas.ph))[i][k];
        }

        /* -water soluble */
        wate();
        for (i = 0; i < 10; i++) {
            dd[1][i] = (*sixs_aerbas.ph)[i][j1] +
                       coef * ((*sixs_aerbas.ph)[i][j1] -
                               (*sixs_aerbas.ph)[i][j1 + 1]);
            for (int k = 0; k < 83; k++)
                pha[1][i][k] = (*sixs_aerbas.ph)[i][k];
        }

        /* -oceanic type */
        ocea();
        for (i = 0; i < 10; i++) {
            dd[2][i] = (*sixs_aerbas.ph)[i][j1] +
                       coef * ((*sixs_aerbas.ph)[i][j1] -
                               (*sixs_aerbas.ph)[i][j1 + 1]);
            for (int k = 0; k < 83; k++)
                pha[2][i][k] = (*sixs_aerbas.ph)[i][k];
        }

        /* -soot */
        soot();
        for (i = 0; i < 10; i++) {
            dd[3][i] = (*sixs_aerbas.ph)[i][j1] +
                       coef * ((*sixs_aerbas.ph)[i][j1] -
                               (*sixs_aerbas.ph)[i][j1 + 1]);
            for (int k = 0; k < 83; k++)
                pha[3][i][k] = (*sixs_aerbas.ph)[i][k];
        }

        /* summ of the c/vi calculation */
        sumni = 0.f;
        sigm = 0.f;

        for (i = 0; i < 4; i++)
            sigm += (double)(c[i] / vi[i]);

        /* cij coefficients calculation */
        for (i = 0; i < 4; i++) {
            mie_in.cij[i] = (double)(c[i] / vi[i] / sigm);
            sumni += mie_in.cij[i] / ni[i];
        }

        nis = 1. / sumni;
    }

    /*     mixing parameters calculation */
    for (i = 0; i < 10; i++) {
        for (int j = 0; j < mie_in.icp; j++) {
            sixs_aer.ext[i] += (double)(ex[j][i] * mie_in.cij[j]);
            sca[i] += (double)(sc[j][i] * mie_in.cij[j]);
            sixs_aer.gasym[i] += (double)(sc[j][i] * mie_in.cij[j] * asy[j][i]);
            sixs_aer.phase[i] += (double)(sc[j][i] * mie_in.cij[j] * dd[j][i]);

            for (int k = 0; k < 83; k++)
                sixs_sos.phasel[i][k] +=
                    (double)(sc[j][i] * mie_in.cij[j] * pha[j][i][k]);
        }

        sixs_aer.ome[i] = sca[i] / sixs_aer.ext[i];
        sixs_aer.gasym[i] /= sca[i];
        sixs_aer.phase[i] /= sca[i];

        for (int k = 0; k < 83; k++)
            sixs_sos.phasel[i][k] /= sca[i];

        sixs_aer.ext[i] *= (double)nis;
        sca[i] *= (double)nis;
    }

    if (filename.size() != 0 && iaer >= 8 && iaer <= 11)
        save();
}

void AerosolModel::parse(const double xmud)
{
    cin >> iaer;
    cin.ignore(numeric_limits<int>::max(), '\n');

    /* initialize vars; */
    mie_in.rmin = 0.f;
    mie_in.rmax = 0.f;
    mie_in.icp = 1;

    int i;
    for (i = 0; i < 4; i++) {
        mie_in.cij[i] = 0.f;

        mie_in.x1[i] = 0.f;
        mie_in.x2[i] = 0.f;
        mie_in.x3[i] = 0.f;

        for (int j = 0; j < 10; j++) {
            mie_in.rn[j][i] = 0.f;
            mie_in.ri[j][i] = 0.f;
        }
    }

    for (i = 0; i < 50; i++) {
        mie_in.rsunph[i] = 0.f;
        mie_in.nrsunph[i] = 0.f;
    }
    mie_in.cij[0] = 1.00f;

    switch (iaer) {
    case 0:
    case 5:
    case 6:
    case 7:
        break; /* do nothing */

    case 1: {
        c[0] = 0.70f;
        c[1] = 0.29f;
        c[2] = 0.00f;
        c[3] = 0.01f;
        break;
    }
    case 2: {
        c[0] = 0.00f;
        c[1] = 0.05f;
        c[2] = 0.95f;
        c[3] = 0.00f;
        break;
    }
    case 3: {
        c[0] = 0.17f;
        c[1] = 0.61f;
        c[2] = 0.00f;
        c[3] = 0.22f;
        break;
    }
    case 4: {
        for (i = 0; i < 4; i++)
            cin >> c[i];
        cin.ignore(numeric_limits<int>::max(), '\n');
        break;
    }
    case 8: {
        cin >> mie_in.rmin;
        cin >> mie_in.rmax;
        cin >> mie_in.icp;
        cin.ignore(numeric_limits<int>::max(), '\n');

        if (mie_in.icp >= 4) {
            G_fatal_error(
                _("mie_in.icp: %ld > 4, will cause internal buffer overflow"),
                mie_in.icp);
        }

        for (i = 0; i < mie_in.icp; i++) {
            cin >> mie_in.x1[i];
            cin >> mie_in.x2[i];
            cin >> mie_in.cij[i];
            cin.ignore(numeric_limits<int>::max(), '\n');

            int j;
            for (j = 0; j < 10; j++)
                cin >> mie_in.rn[j][i];
            cin.ignore(numeric_limits<int>::max(), '\n');

            for (j = 0; j < 10; j++)
                cin >> mie_in.ri[j][i];
            cin.ignore(numeric_limits<int>::max(), '\n');
        }
        break;
    }
    case 9: {
        cin >> mie_in.rmin;
        cin >> mie_in.rmax;
        cin.ignore(numeric_limits<int>::max(), '\n');

        cin >> mie_in.x1[0];
        cin >> mie_in.x2[0];
        cin >> mie_in.x3[0];
        cin.ignore(numeric_limits<int>::max(), '\n');

        int j;
        for (j = 0; j < 10; j++)
            cin >> mie_in.rn[j][0];
        cin.ignore(numeric_limits<int>::max(), '\n');

        for (j = 0; j < 10; j++)
            cin >> mie_in.ri[j][0];
        cin.ignore(numeric_limits<int>::max(), '\n');

        break;
    }
    case 10: {
        cin >> mie_in.rmin;
        cin >> mie_in.rmax;
        cin.ignore(numeric_limits<int>::max(), '\n');

        cin >> mie_in.x1[0];
        cin.ignore(numeric_limits<int>::max(), '\n');

        int j;
        for (j = 0; j < 10; j++)
            cin >> mie_in.rn[j][0];
        cin.ignore(numeric_limits<int>::max(), '\n');

        for (j = 0; j < 10; j++)
            cin >> mie_in.ri[j][0];
        cin.ignore(numeric_limits<int>::max(), '\n');

        break;
    }
    case 11: {
        cin >> mie_in.irsunph;
        cin.ignore(numeric_limits<int>::max(), '\n');

        if (mie_in.irsunph >= 50) {
            G_fatal_error(_("mie_in.irsunph: %ld > 50, will cause internal "
                            "buffer overflow"),
                          mie_in.irsunph);
        }

        for (i = 0; i < mie_in.irsunph; i++) {
            cin >> mie_in.rsunph[i];
            cin >> mie_in.nrsunph[i];
            cin.ignore(numeric_limits<int>::max(), '\n');

            double sq = mie_in.rsunph[i] * mie_in.rsunph[i];
            const double ln10 = 2.3025850929940456840179914546844;
            mie_in.nrsunph[i] = (double)(mie_in.nrsunph[i] / (sq * sq) / ln10);
        }
        mie_in.rmin = mie_in.rsunph[0];
        mie_in.rmax = mie_in.rsunph[mie_in.irsunph - 1] + 1e-07f;

        for (i = 0; i < 10; i++)
            cin >> mie_in.rn[i][0];
        cin.ignore(numeric_limits<int>::max(), '\n');

        for (i = 0; i < 10; i++)
            cin >> mie_in.ri[i][0];
        cin.ignore(numeric_limits<int>::max(), '\n');
        break;
    }
    case 12: { /* read file name */
        getline(cin, filename);
        filename = filename.substr(0, filename.find(" "));
        break;
    }
    default:
        G_warning(_("Unknown aerosol model!"));
    }

    if (iaer >= 8 && iaer <= 11) {
        cin >> iaerp;
        if (iaerp == 1) /* read file name */
        {
            getline(cin, filename);
            filename = filename.substr(0, filename.find(" "));
            filename += ".mie";
        }
    }

    aeroso(xmud);
}

/* format 132 */
void AerosolModel::print132(string s)
{
    Output::Begin();
    Output::Repeat(15, ' ');
    Output::Print(s);
    Output::Print(" aerosols model");
    Output::End();
}

/* --- aerosols model ---- */
void AerosolModel::print()
{
    /* --- aerosols model (type) ---- */
    Output::Begin();
    Output::Repeat(10, ' ');
    Output::Print(" aerosols type identity :");
    Output::End();

    if (iaer == 4 || (iaer >= 8 && iaer != 11)) {
        Output::Begin();
        Output::Repeat(15, ' ');
        Output::Print(" user defined aerosols model ");
        Output::End();
    }

    switch (iaer) {
    case 0: {
        Output::Begin();
        Output::Repeat(15, ' ');
        Output::Print(" no aerosols computed   ");
        Output::End();
        break;
    }
    case 1:
        print132("    Continental");
        break;
    case 2:
        print132("       Maritime");
        break;
    case 3:
        print132("          Urban");
        break;
    case 4: {
        static const string desc[4] = {
            string(" % of dust-like"), string(" % of water-soluble"),
            string(" % of oceanic"), string(" % of soot")};

        for (int i = 0; i < 4; i++) {
            Output::Begin();
            Output::Repeat(26, ' ');
            ostringstream s;
            s.setf(ios::fixed, ios::floatfield);
            s << setprecision(3);
            s << c[i] << desc[i] << ends;
            Output::Print(s.str());
            Output::End();
        }
        break;
    }
    case 5:
        print132("       Desertic");
        break;
    case 6:
        print132("          Smoke");
        break;
    case 7:
        print132("  Stratospheric");
        break;
    case 8: {
        Output::Begin();
        Output::Repeat(15, ' ');
        ostringstream s;
        s << "using " << mie_in.icp << " Log-normal size-distribution(s)"
          << ends;
        Output::Print(s.str());
        Output::End();

        Output::Begin();
        Output::Repeat(15, ' ');
        Output::Print("Mean radius  Stand. Dev.  Percent. dencity");
        Output::End();

        for (int i = 0; i < mie_in.icp; i++) {
            Output::Begin();
            Output::Position(41);
            ostringstream s1;
            s1.setf(ios::fixed, ios::floatfield);
            s1 << setprecision(4);
            s1 << setw(10) << mie_in.x1[i] << ends;
            Output::Print(s1.str());

            Output::Position(55);
            ostringstream s2;
            s2.setf(ios::fixed, ios::floatfield);
            s2 << setprecision(3);
            s2 << setw(8) << mie_in.x2[i] << ends;
            Output::Print(s2.str());

            Output::Position(69);
            ostringstream s3;
            s3.setf(ios::fixed, ios::floatfield);
            s3 << setprecision(3);
            s3 << setw(11) << mie_in.cij[i] << ends;
            Output::Print(s3.str());

            Output::End();
        }
        break;
    }
    case 9: {
        Output::Begin();
        Output::Repeat(15, ' ');
        Output::Print("using a Modified Gamma size-distribution");
        Output::End();

        Output::Begin();
        Output::Repeat(19, ' ');
        Output::Print("Alpha         b             Gamma");
        Output::End();

        Output::Begin();
        Output::Position(20);
        ostringstream s1;
        s1.setf(ios::fixed, ios::floatfield);
        s1 << setprecision(3);
        s1 << setw(9) << mie_in.x1[0] << ends;
        Output::Print(s1.str());

        Output::Position(31);
        ostringstream s2;
        s2.setf(ios::fixed, ios::floatfield);
        s2 << setprecision(3);
        s2 << setw(9) << mie_in.x2[0] << ends;
        Output::Print(s2.str());

        Output::Position(47);
        ostringstream s3;
        s3.setf(ios::fixed, ios::floatfield);
        s3 << setprecision(3);
        s3 << setw(9) << mie_in.x3[0] << ends;
        Output::Print(s3.str());

        Output::End();
        break;
    }
    case 10: {
        Output::Begin();
        Output::Repeat(15, ' ');
        Output::Print("using a Power law size-distribution with alpha=");
        ostringstream s;
        s.setf(ios::fixed, ios::floatfield);
        s << setprecision(1);
        s << setw(4) << mie_in.x1[0] << ends;
        Output::Print(s.str());
        Output::End();

        break;
    }
    case 11:
        print132(" Sun Photometer");
        break;
    case 12: {
        Output::Begin();
        Output::Repeat(15, ' ');
        Output::Print("using data from the file:");
        Output::End();

        Output::Begin();
        Output::Position(25);
        Output::Print(filename);
        Output::End();
    }
    }

    if (iaer > 7 && iaerp == 1) {
        Output::Begin();
        Output::Repeat(15, ' ');
        Output::Print(" results saved into the file:");
        Output::End();

        Output::Begin();
        Output::Position(25);
        Output::Print(filename);
        Output::End();
    }
}

AerosolModel AerosolModel::Parse(const double xmud)
{
    AerosolModel aero;
    aero.parse(xmud);
    return aero;
}
