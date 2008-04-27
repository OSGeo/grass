/***************************************************************************
                          Transform.h  -  do atmospheric correction on an input value
                             -------------------
    begin                : Fri Jan 10 2003
    copyright            : (C) 2003 by Christo Zietsman
    email                : 13422863@sun.ac.za
 ***************************************************************************/

#ifndef TRANSFORM_H
#define TRANSFORM_H

/* Inputs needed to do transformation */
struct TransformInput
{
    int iwave;
    float asol;
    
    float ainr[2][3];
    float sb;
    float seb;
    float tgasm;
    float sutott;
    float sdtott;
    float sast;
    float srotot;
    float xmus;
};

/* The following combinations of input values types exist */
enum InputMask
{
    REFLECTANCE     = 0,  
    RADIANCE        = 1,  /* the default */
    ETM_BEFORE      = 2,  /* etm+ taken before July 1, 2000 */
    REF_ETM_BEFORE  = 2,
    RAD_ETM_BEFORE  = 3,
    ETM_AFTER       = 4,  /* etm+ taken after July 1, 2000 */
    REF_ETM_AFTER   = 4,
    RAD_ETM_AFTER   = 5
};

/* Assuming input value between 0 and 1
if rad is true, idn should first be converted to a reflectance value
returns adjusted value also between 0 and 1 */
extern float transform(const TransformInput ti, InputMask imask, float idn);

#endif /* TRANSFORM_H */
