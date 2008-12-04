#include "driver.h"
#include "driverlib.h"

void COM_Bitmap(int ncols, int nrows, int threshold,
		const unsigned char *buf)
{
    if (driver->Bitmap)
	(*driver->Bitmap) (ncols, nrows, threshold, buf);
}

void COM_Line_abs(double x0, double y0, double x1, double y1)
{
    COM_Begin();
    COM_Move(x0, y0);
    COM_Cont(x1, y1);
    COM_Stroke();
}

void COM_Begin(void)
{
    if (driver->Begin)
	(*driver->Begin)();
}

void COM_Move(double x, double y)
{
    if (driver->Move)
	(*driver->Move)(x, y);
}

void COM_Cont(double x, double y)
{
    if (driver->Cont)
	(*driver->Cont)(x, y);
}

void COM_Close(void)
{
    if (driver->Close)
	(*driver->Close)();
}

void COM_Stroke(void)
{
    if (driver->Stroke)
	(*driver->Stroke)();
}

void COM_Fill(void)
{
    if (driver->Fill)
	(*driver->Fill)();
}

void COM_Point(double x, double y)
{
    if (driver->Point)
	(*driver->Point)(x, y);
}

