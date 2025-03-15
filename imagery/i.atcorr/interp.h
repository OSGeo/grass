#ifndef INTERP_H
#define INTERP_H

struct InterpStruct {
    double romix;
    double rorayl;
    double roaero;
    double phaa;
    double phar;
    double tsca;
    double tray;
    double trayp;
    double taer;
    double taerp;
    double dtott;
    double utott;
    double astot;
    double asray;
    double asaer;
    double utotr;
    double utota;
    double dtotr;
    double dtota;
};

/*
   To estimate the different atmospheric functions r(mS,mv,fS,fv), T(q) and S at
   any wavelength from the 10 discrete computations (subroutine DISCOM).
 */
void interp(const int iaer, const int idatmp, const double wl,
            const double taer55, const double taer55p, const double xmud,
            InterpStruct &is);

#endif /* INTERP_H */
