/***************************************************************************
                          AerosolConcentration.h  -  description
                             -------------------
    begin                : Mon Jan 13 2003
    copyright            : (C) 2003 by Christo Zietsman
    email                : 13422863@sun.ac.za
 ***************************************************************************/

#ifndef AEROSOLCONCENTRATION_H
#define AEROSOLCONCENTRATION_H

/**********************************************************************c
c              aerosol model (concentration)                           c
c              ----------------------------                            c
c                                                                      c
c                                                                      c
c  v             if you have an estimate of the meteorological         c
c                parameter: the visibility v, enter directly the       c
c                value of v in km (the aerosol optical depth will      c
c                be computed from a standard aerosol profile)          c

c                                                                      c
c  v=0, taer55   if you have an estimate of aerosol optical depth ,    c
c                enter v=0 for the visibility and enter the aerosol    c
c                optical depth at 550                                  c
c                                                                      c
c  v=-1          warning:  if iaer=0, enter v=-1                       c
c                                                                      c
c**********************************************************************/
struct AtmosModel;

struct AerosolConcentration
{
	/* aerosol concentration parameters */
    double taer55;

private:
    long int iaer;
    double v;
    void parse(const long int iaer, const AtmosModel &atms);
    void oda550(const double v, const AtmosModel &atms);

public:
    /* Set the visibility, this will overide any previous estimates of taer55 */
    void set_visibility (const double vis, const AtmosModel &atms) { if(vis > 0) oda550(vis, atms); }
    void print();
    static AerosolConcentration Parse(const long int iaer, const AtmosModel &atms);
};

#endif /* AEROSOLCONCENTRATION_H */

