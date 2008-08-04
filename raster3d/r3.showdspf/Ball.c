
/***** Ball.c *****/
/* This file contains code required to implement a trackball user
 * interface for manipulating a 3D scene.  This code was taken from 
 * Graphics Gems 4, editted by Andrew Glassner
 */
#include "Ball.h"
#include "BallMath.h"
#include <stdio.h>
#define TRUE 1
#define FALSE 0

HMatrix mId = { {1, 0, 0, 0}
, {0, 1, 0, 0}
, {0, 0, 1, 0}
, {0, 0, 0, 1}
};
float otherAxis[][4] = { {-0.48, 0.80, 0.36, 1} };

/* Establish reasonable initial values for controller. */
void Ball_Init(BallData * ball)
{
    int i;

    ball->center = qOne;
    ball->radius = 1.0;
    ball->vDown = ball->vNow = qOne;
    ball->qDown = ball->qNow = qOne;
    for (i = 15; i >= 0; i--)
	((float *)ball->mNow)[i] = ((float *)ball->mDown)[i] =
	    ((float *)mId)[i];
    ball->dragging = 0;
}

/* Set the center and size of the controller. */
void Ball_Place(BallData * ball, HVect center, double radius)
{
    ball->center = center;
    ball->radius = radius;
}

/* Incorporate new mouse position. */
void Ball_Mouse(BallData * ball, HVect vNow)
{
    ball->vNow = vNow;
}


/* Using vDown, vNow, dragging, and axisSet, compute rotation etc. */
void Ball_Update(BallData * ball)
{
    Quat qout, qout2;

    ball->vFrom = MouseOnSphere(ball->vDown, ball->center, ball->radius);
    ball->vTo = MouseOnSphere(ball->vNow, ball->center, ball->radius);
    if (ball->dragging) {
	ball->qDrag = Qt_FromBallPoints(ball->vFrom, ball->vTo);
	ball->qNow = Qt_Mul(ball->qDrag, ball->qDown);
    }
    Qt_ToBallPoints(ball->qDown, &ball->vrFrom, &ball->vrTo);


    Qt_ToMatrix(Qt_Conj(ball->qNow), ball->mNow);	/* Gives transpose for GL. */
    qout2 = Matrix_to_Qt(ball->mNow);
    qout = Qt_Conj(qout2);

}

void Ball_SetMatrix(BallData * ball, HMatrix mat)
{
    int i, j;
    Quat qt;

    Ball_Init(ball);
    qt = Matrix_to_Qt(mat);
    ball->qNow = Qt_Conj(qt);
    ball->qDown = ball->qNow;
    for (i = 0; i < QuatLen; i++)
	for (j = 0; j < QuatLen; j++)
	    ball->mNow[i][j] = mat[i][j];
}

/* Return rotation matrix defined by controller use. */
void Ball_Value(BallData * ball, HMatrix mNow)
{
    int i;

    for (i = 15; i >= 0; i--)
	((float *)mNow)[i] = ((float *)ball->mNow)[i];
}


/* Begin drag sequence. */
void Ball_BeginDrag(BallData * ball)
{
    ball->dragging = 1;
    ball->vDown = ball->vNow;
}

/* Stop drag sequence. */
void Ball_EndDrag(BallData * ball)
{
    int i;

    ball->dragging = 0;
    ball->qDown = ball->qNow;
    for (i = 15; i >= 0; i--)
	((float *)ball->mDown)[i] = ((float *)ball->mNow)[i];
}
