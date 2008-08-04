/* camera_ref.h */

#define BACKGROUND GREY
#define NLINES 20

static int ok;

struct box
{
    int top, bottom, left, right;
};

#ifndef GLOBALCAM
#  define GLOBALCAM extern
#endif

GLOBALCAM int which;
GLOBALCAM struct box more, less, report;
GLOBALCAM int height, size, edge, nlines;
GLOBALCAM int curp, cury, first_point;
GLOBALCAM double rms;
GLOBALCAM char cam_name[30], cam_id[30];
GLOBALCAM double Xp, Yp, cfl;
GLOBALCAM int num_fid;
GLOBALCAM char fid_id[5];
GLOBALCAM double *Xf, *Yf;
GLOBALCAM int pager;
GLOBALCAM int xmax, ymax, gmax;
GLOBALCAM int color;
GLOBALCAM int tsize;
GLOBALCAM int cury;
GLOBALCAM int len;
GLOBALCAM int line;
GLOBALCAM int top, bottom, left, right, width, middle, nums;

#define FMT0(buf,n) \
	sprintf (buf, "%3d ", n)
#define FMT0f(buf,n) \
	sprintf (buf, "%3f ", n)
#define FMT1(buf,fid_id,Xf,Yf) \
	sprintf (buf, " %10s     %10.4f     %10.4f ", fid_id,Xf,Yf)
#define FMT2(buf,cam_name) \
	sprintf (buf, "CAMERA NAME   %10s", cam_name)
#define FMT3(buf,cam_id) \
	sprintf (buf, "CAMERA ID     %10s", cam_id)
#define FMT4(buf,cfl) \
	sprintf (buf, "CAMERA CFL    %10.4f", cfl)
#define FMT5(buf, Xp) \
	sprintf (buf, "CAMERA XP     %10.4f", Xp)
#define FMT6(buf, Yp) \
	sprintf (buf, "CAMERA YP     %10.4f", Yp)
#define FMT7(buf, num_fid) \
	sprintf (buf, "number of fid.  %5d", num_fid)
#define LHEAD1 "          CAMERA REFERENCE FILE               "
#define LHEAD3 "                                              "
#define LHEAD4 "        ID         Photo X         Photo Y    "
#define LHEAD2 "----------------------------------------------"
