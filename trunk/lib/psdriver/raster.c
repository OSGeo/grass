
#include <string.h>

#include "psdriver.h"

static int masked;

void PS_begin_raster(int mask, int src[2][2], double dst[2][2])
{
    const char *type = ps.true_color ? (mask ? "RASTERRGBMASK" : "RASTERRGB")
	: (mask ? "RASTERGRAYMASK" : "RASTERGRAY");

    int ssx = src[0][1] - src[0][0];
    int ssy = src[1][1] - src[1][0];
    int sox = src[0][0];
    int soy = src[1][0];

    double dsx = dst[0][1] - dst[0][0];
    double dsy = dst[1][1] - dst[1][0];
    double dox = dst[0][0];
    double doy = dst[1][0];

    masked = mask;

    output("gsave\n");
    output("%f %f translate %f %f scale\n", dox, doy, dsx, dsy);
    output("%d %d [%d 0 0 %d %d %d] %s\n", ssx, ssy, ssx, ssy, sox, soy,
	   type);
}

int PS_raster(int n, int row,
	      const unsigned char *red, const unsigned char *grn,
	      const unsigned char *blu, const unsigned char *nul)
{
    int i;

    for (i = 0; i < n; i++) {
	if (ps.true_color) {
	    if (masked)
		output("%02X%02X%02X%02X", (nul && nul[i]) ? 0xFF : 0x00,
		       red[i], grn[i], blu[i]);
	    else
		output("%02X%02X%02X", red[i], grn[i], blu[i]);
	}
	else {
	    unsigned int gray =
		(unsigned int)(red[i] * 0.299 + grn[i] * 0.587 +
			       blu[i] * 0.114);

	    if (masked)
		output("%02X%02X", (nul && nul[i]) ? 0xFF : 0x00, gray);
	    else
		output("%02X", gray);
	}
    }

    output("\n");

    return row + 1;
}

void PS_end_raster(void)
{
    output("grestore\n");
}
