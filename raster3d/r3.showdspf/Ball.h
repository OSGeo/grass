
/***** Ball.h *****/
#ifndef _H_Ball
#define _H_Ball
#include "BallAux.h"

typedef enum AxisSet
{ NoAxes, CameraAxes, BodyAxes, OtherAxes, NSets } AxisSet;
typedef float *ConstraintSet;
typedef struct
{
    HVect center;
    double radius;
    Quat qNow, qDown, qDrag;
    HVect vNow, vDown, vFrom, vTo, vrFrom, vrTo;
    HMatrix mNow, mDown;
    int dragging;
} BallData;

/* Public routines */
void Ball_Init(BallData * ball);
void Ball_Place(BallData * ball, HVect center, double radius);
void Ball_Mouse(BallData * ball, HVect vNow);
void Ball_Update(BallData * ball);
void Ball_Value(BallData * ball, HMatrix mNow);
void Ball_BeginDrag(BallData * ball);
void Ball_EndDrag(BallData * ball);
void Ball_SetMatrix(BallData * ball, HMatrix mat);
#endif
