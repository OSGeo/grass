#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/vask.h>
#include "orthophoto.h"
#include "globals.h"

#define NLINES 10

int mod_cam_info(int have_old, struct Ortho_Camera_File_Ref *cam_info)
{
    long fidnum[NLINES];
    char next[20];
    char next_line[20];
    int i;
    int atnum;
    int line;
    int startfid;
    int endfid;

    if (!have_old)
	strcpy(cam_info->cam_name, "DBA SYSTEMS CAMERA");

    V_clear();
    V_line(1, "                   Please provide the following information:");
    V_line(2,
	   "+------------------------------------------------------------------------------+");
    V_line(4, "        Camera Name");
    V_line(5, "        Camera Identification");
    V_line(6, "        Calibrated Focal Length mm.");
    V_line(7, "        Point of Symmetry: X-coordinate mm.");
    V_line(8, "        Point of Symmetry: Y-coordinate mm.");
    V_line(9, "        Maximum number of fiducial or reseau marks");
    V_line(11,
	   "+-----------------------------------------------------------------------------+");
    V_ques(cam_info->cam_name, 's', 4, 55, 20 - 1);
    V_ques(cam_info->cam_id, 's', 5, 55, 20 - 1);
    V_ques(&(cam_info->CFL), 'd', 6, 55, 20 - 1);
    V_ques(&(cam_info->Xp), 'd', 7, 55, 20 - 1);
    V_ques(&(cam_info->Yp), 'd', 8, 55, 20 - 1);
    V_ques(&(cam_info->num_fid), 'i', 9, 55, 20 - 1);
    V_intrpt_ok();
    if (!V_call())
	exit(0);

    /* get fiducail or reseau info NUMLINES at a time */
    startfid = 0;
    while (startfid >= 0 && startfid < cam_info->num_fid) {
	V_clear();
	{
	    V_line(1,
		   "               Please provide the following information:");
	    V_line(2,
		   "+--------------------------------------------------------------------------+");
	    V_line(4,
		   "            Fid#     Fid Id          Xf              Yf");
	}
	endfid =
	    startfid + NLINES <=
	    cam_info->num_fid + 1 ? startfid + NLINES : cam_info->num_fid;

	atnum = 0;
	line = 6;
	for (i = startfid; i < endfid; i++) {

	    fidnum[atnum] = i + 1;

	    V_const(&fidnum[atnum], 'i', line, 13, 5);
	    V_ques(cam_info->fiducials[i].fid_id, 's', line, 21, 6);
	    V_ques(&(cam_info->fiducials[i].Xf), 'd', line, 33, 10);
	    V_ques(&(cam_info->fiducials[i].Yf), 'd', line, 49, 10);

	    atnum++;
	    line++;
	}

	line += 2;
	*next = 0;

	if (endfid >= cam_info->num_fid)
	    strcpy(next, "end");
	else
	    sprintf(next, "%d", endfid);
	sprintf(next_line, next);
	V_line(line, "                            Next:");
	V_ques(next, 's', line, 34, 5);
	V_line(line + 2,
	       "+--------------------------------------------------------------------------+");
	V_intrpt_ok();
	if (!V_call())
	    exit(0);

	if (*next == 0)
	    break;
	if (strcmp(next, "end") == 0)
	    break;
	if (sscanf(next, "%d", &endfid) != 1)
	    continue;
	if (endfid < 0)
	    endfid = 0;
	if (endfid > cam_info->num_fid) {
	    endfid = cam_info->num_fid - NLINES + 1;
	    if (endfid < 0)
		endfid = 0;
	}

	startfid = endfid;
    }

    return 0;
}
