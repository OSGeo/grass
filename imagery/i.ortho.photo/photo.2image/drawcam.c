#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "globals.h"

static char buf[300];
static int done(void);
static int debug(char *);
static int to_file(void);
static int askfile(void);
static int to_printer(void);
static int do_report(FILE *);

#define FMT0(buf,n) \
	sprintf (buf, " %3d ", n)
#define FMT1(buf,fid_id,Xf,Yf) \
	sprintf (buf, " %10s     %10.4f     %10.4f ", fid_id,Xf,Yf)
#define FMT2(buf,cam_name) \
	sprintf (buf, "CAMERA NAME   %30s", cam_name)
#define FMT3(buf,cam_id) \
	sprintf (buf, "CAMERA ID     %30s", cam_id)
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


int drawcam(void)
{
    static int use = 1;
    static Objects objects[] = {
	MENU("CANCEL", done, &use),
	MENU("PRINT", to_printer, &use),
	MENU("FILE", to_file, &use),
	/*INFO("do something", &use), */
	/*OTHER(pick,&use), */
	{0}
    };

    while (1) {
	if (Input_pointer(objects) < 0)
	    break;
    }
    return 0;			/* return but don't QUIT */
}

static int done(void)
{
    return -1;
}

static int debug(char *msg)
{
    R_stabilize();
    Curses_write_window(PROMPT_WINDOW, 1, 1, msg);
    /* Curses_getch(0); */

    return 0;
}

static int to_file(void)
{
    FILE *fd;
    char msg[1024];

    if (Input_other(askfile, "Keyboard") < 0) {
	return 0;
    }

    fd = fopen(buf, "w");
    if (fd == NULL) {
	sprintf(msg, "** Unable to create file %s\n", buf);
	Beep();
	Curses_write_window(PROMPT_WINDOW, 2, 1, msg);
    }
    else {
	do_report(fd);
	fclose(fd);
	sprintf(msg, "Report saved in file %s\n", buf);
	Curses_write_window(PROMPT_WINDOW, 2, 1, msg);
    }
    return -1;
}

static int askfile(void)
{
    char file[100];

    while (1) {
	Curses_prompt_gets("Enter file to hold report: ", file);
	G_strip(file);
	if (*file == 0)
	    return -1;
	if (G_index(file, '/'))
	    strcpy(buf, file);
	else
	    sprintf(buf, "%s/%s", G_home(), file);
	if (access(buf, 0) != 0)
	    return 1;
	sprintf(buf, "** %s already exists. choose another file", file);
	Beep();
	Curses_write_window(PROMPT_WINDOW, 2, 1, buf);
    }
}

static int to_printer(void)
{
    FILE *fd;

    Menu_msg("sending camera file to printer ...");

    fd = popen("lpr", "w");
    do_report(fd);
    pclose(fd);
    return 0;
}

static int do_report(FILE * fd)
{
    char buf[100];
    int n;
    int width;

    fprintf(fd, "LOCATION: %-20s GROUP: %-20s MAPSET: %s\n\n",
	    G_location(), group.name, G_mapset());
    fprintf(fd, "CAMERA REFERENCE FILE\n\n");
    fprintf(fd, "%s\n", LHEAD1);
    fprintf(fd, "%s\n", LHEAD2);

    FMT1(buf, "     ", 0.0, 0.0);
    width = strlen(buf);

    for (n = 0; n < group.camera_ref.num_fid; n++) {
	FMT0(buf, n + 1);
	fprintf(fd, "%s", buf);
	FMT1(buf, group.camera_ref.fiducials[n].fid_id,
	     group.camera_ref.fiducials[n].Xf,
	     group.camera_ref.fiducials[n].Yf);
	fprintf(fd, "%s", buf);
	fprintf(fd, "   %s\n", buf);
    }
    fprintf(fd, "\n");

    return 0;
}
