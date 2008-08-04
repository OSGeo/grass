#include "driver.h"
#include "driverlib.h"

void COM_Bitmap(int ncols, int nrows, int threshold, const unsigned char *buf)
{
    DRV_draw_bitmap(ncols, nrows, threshold, buf);
}
