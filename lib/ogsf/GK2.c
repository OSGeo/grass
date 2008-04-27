/*
* $Id$
*/

#include <stdlib.h>
#include <stdio.h>
#include <grass/gstypes.h>
#include <grass/keyframe.h>
#include <grass/kftypes.h>

static int _add_key(Keylist *, int, float);
static void _remove_key(Keylist *);

static Keylist *Keys = NULL;
static Keylist *Keytail = NULL;
static Viewnode *Views = NULL;
static float Keystartpos = 0.0;
static float Keyendpos = 1.0;
static float Tension = 0.8;
static int Viewsteps = 0;
static int Numkeys = 0;
static int Interpmode = KF_SPLINE;
static int Fmode = 0;

/* next & prior already initialized to NULL */
static int _add_key(Keylist * newk, int force_replace, float precis)
{
    Keylist *k, *tempk, *prev;
    int found;

    found = 0;
    prev = NULL;

    /* if(Viewsteps) precis = 0.5/Viewsteps; */
    for (k = Keys; k; k = k->next) {
	if (k->pos >= newk->pos - precis && k->pos <= newk->pos + precis) {
	    if (force_replace) {

		if (k->prior) {
		    k->prior->next = newk;
		    newk->prior = prev;
		}
		else {
		    Keys = newk;
		}

		newk->next = k->next;
		newk->prior = k->prior;
		tempk = k;
		k = newk;
		free(tempk);
	    }
	    else {
		free(newk);
	    }

	    return (-1);
	}
    }

    if (Keys) {
	if (newk->pos < Keys->pos) {
	    /* new will be first */
	    newk->next = Keys;
	    Keys->prior = newk;
	    Keys = newk;
	}
	else {
	    prev = k = Keys;
	    while (k && !found) {
		if (k->pos > newk->pos) {
		    prev->next = newk;
		    newk->next = k;
		    newk->prior = prev;
		    k->prior = newk;
		    found = 1;
		}

		prev = k;
		k = k->next;
	    }
	    if (!found) {
		Keytail = prev->next = newk;
		newk->prior = prev;
	    }
	}
    }
    else {
	Keys = Keytail = newk;
    }

    ++Numkeys;
    return (1);
}

static void _remove_key(Keylist * k)
{
    if (k->prior) {
	k->prior->next = k->next;
	if (k->next) {
	    k->next->prior = k->prior;
	}
	else {
	    Keytail = k->prior;
	}
    }
    else {
	Keys = k->next;
	if (k->next) {
	    k->next->prior = NULL;
	}
    }
    k->next = k->prior = NULL;

    return;
}

int GK_set_interpmode(int mode)
{
    if (KF_LEGAL_MODE(mode)) {
	Interpmode = mode;
	return (1);
    }

    return (-1);
}

void GK_set_tension(float tens)
{
    Tension = tens > 1.0 ? 1.0 : (tens < 0.0 ? 0.0 : tens);

    /* for now */
    if (Views) {
	GK_update_frames();
	GS_set_draw(GSD_BACK);
	GS_ready_draw();
	GS_clear(GS_background_color());
	GS_alldraw_wire();

	gk_draw_path(Views, Viewsteps, Keys);

	GS_done_draw();
    }

    return;
}

void GK_showtension_start(void)
{
    return;
}

void GK_showtension_stop(void)
{
    return;
}

void GK_update_tension(void)
{
    if (Views) {
	GK_update_frames();
    }

    return;
}



void GK_print_keys(char *name)
{
    Keylist *k;
    FILE *fp;
    int cnt = 1;

    if (NULL == (fp = fopen(name, "w"))) {
	fprintf(stderr, "Cannot open file for output\n"), exit(1);
    }
    /* write a default frame rate of 30 at top of file */
    fprintf(fp, "30 \n");

    for (k = Keys; k; k = k->next) {

	fprintf(fp,
		"{%f {{FromX %f} {FromY %f} {FromZ %f} {DirX %f} {DirY %f} {DirZ %f} {FOV %f} {TWIST %f} {cplane-0 {{pos_x 0.000000} {pos_y 0.000000} {pos_z 0.000000} {blend_type OFF} {rot 0.000000} {tilt 0.000000}}}} keyanimtag%d 0} ",
		k->pos, k->fields[KF_FROMX], k->fields[KF_FROMY],
		k->fields[KF_FROMZ], k->fields[KF_DIRX], k->fields[KF_DIRY],
		k->fields[KF_DIRZ], k->fields[KF_FOV] / 10.,
		k->fields[KF_TWIST], cnt);
	cnt++;
    }

    fclose(fp);
    return;

}

void GK_update_frames(void)
{
    Keylist *k;
    int loop = 0;

    if (Keys) {
	if (Numkeys > 1) {
	    k = Keytail;
	    Keyendpos = k->pos;

	    if (k->fields[KF_FROMX] == Keys->fields[KF_FROMX] &&
		k->fields[KF_FROMY] == Keys->fields[KF_FROMY] &&
		k->fields[KF_FROMZ] == Keys->fields[KF_FROMZ]) {
		loop = 1;
	    }
	}

	Keystartpos = Keys->pos;
    }

    if (Interpmode == KF_LINEAR && Numkeys > 1) {
	if (Views) {
	    free(Views);
	    Views = NULL;
	}

	Views = gk_make_linear_framesfromkeys(Keys, Numkeys, Viewsteps, loop);

	if (!Views) {
	    fprintf(stderr,
		    "Check no. of frames requested and keyframes marked\n");
	}
    }
    else if (Numkeys > 2) {
	if (Views) {
	    free(Views);
	    Views = NULL;
	}

	Views = gk_make_framesfromkeys
	    (Keys, Numkeys, Viewsteps, loop, 1.0 - Tension);

	if (!Views) {
	    fprintf(stderr,
		    "Check no. of frames requested and keyframes marked\n");
	}
    }

    return;
}

void GK_set_numsteps(int newsteps)
{
    Viewsteps = newsteps;
    GK_update_frames();

    return;
}


void GK_clear_keys(void)
{
    gk_free_key(Keys);
    Keys = NULL;
    Numkeys = 0;
    free(Views);
    Views = NULL;

    Keystartpos = 0.0;
    Keyendpos = 1.0;

    return;
}

int GK_move_key(float oldpos, float precis, float newpos)
{
    Keylist *k;

    for (k = Keys; k; k = k->next) {
	if (k->pos >= oldpos - precis && k->pos <= oldpos + precis) {
	    _remove_key(k);
	    k->pos = newpos;
	    _add_key(k, 1, precis);
	    GK_update_frames();
	    return (1);
	}
    }

    return (0);
}

/* returns number of keys deleted */
int GK_delete_key(float pos, float precis, int justone)
{
    Keylist *k, *next;
    int cnt;

    for (cnt = 0, k = Keys; k;) {
	next = k->next;

	if (k->pos >= pos - precis && k->pos <= pos + precis) {
	    cnt++;
	    _remove_key(k);
	    free(k);
	    if (justone) {
		break;
	    }
	}

	k = next;
    }

    GK_update_frames();
    return (cnt);
}

/* returns 1 if key added, otherwise -1 */
int GK_add_key(float pos, unsigned long fmask, int force_replace,
	       float precis)
{
    Keylist *newk;
    float tmp[3];

    if (NULL == (newk = (Keylist *) malloc(sizeof(Keylist)))) {
	fprintf(stderr, "Out of memory\n");
	return (-1);
    }

    /* All fields set, don't use mask until making Views */

    GS_get_from(tmp);
    newk->fields[KF_FROMX] = tmp[X];
    newk->fields[KF_FROMY] = tmp[Y];
    newk->fields[KF_FROMZ] = tmp[Z];

#ifdef KDEBUG
    {
	fprintf(stderr, "KEY FROM: %f %f %f\n", tmp[X], tmp[Y], tmp[Z]);
    }
#endif

/* Instead of View Dir try get_focus (view center) */
/* View Dir is implied from eye and center position */
/*    GS_get_viewdir(tmp); */

/* ACS 1 line: was 	GS_get_focus(tmp);
 	with this kanimator works also for flythrough navigation
	also changed in gk.c
*/
    GS_get_viewdir(tmp);
    newk->fields[KF_DIRX] = tmp[X];
    newk->fields[KF_DIRY] = tmp[Y];
    newk->fields[KF_DIRZ] = tmp[Z];

    newk->fields[KF_FOV] = GS_get_fov();
    newk->fields[KF_TWIST] = GS_get_twist();
    newk->pos = pos;
    newk->fieldmask = fmask;
    newk->next = NULL;
    newk->prior = NULL;

    if (0 < _add_key(newk, force_replace, precis)) {
	GK_update_frames();
	return (1);
    }

    return (-1);
}

void GK_do_framestep(int step, int render)
{
    if (Views) {
	if (step > 0 && step <= Viewsteps) {
	    gk_follow_frames(Views, Viewsteps, Keys, step, 1, render, Fmode);
	}
    }

    return;
}


void GK_show_path(int flag)
{
    if (flag) {
	Fmode |= FM_PATH;

	if (Views) {
	    GS_set_draw(GSD_FRONT);
	    GS_ready_draw();

	    gk_draw_path(Views, Viewsteps, Keys);

	    GS_done_draw();

	}
    }
    else {
	Fmode &= ~FM_PATH;
    }

    return;
}

void GK_show_vect(int flag)
{
    if (flag) {
	Fmode |= FM_VECT;
	if (Views) {

	    GS_set_draw(GSD_FRONT);
	    GS_ready_draw();

	    GV_alldraw_vect();

	    GS_done_draw();
	}
    }
    else {
	Fmode &= ~FM_VECT;
    }

    return;
}

void GK_show_site(int flag)
{
    if (flag) {
	Fmode |= FM_SITE;

	if (Views) {

	    GS_set_draw(GSD_FRONT);
	    GS_ready_draw();

	    GP_alldraw_site();

	    GS_done_draw();

	}
    }
    else {
	Fmode &= ~FM_SITE;
    }

    return;
}

void GK_show_vol(int flag)
{
    if (flag) {
	Fmode |= FM_VOL;

	if (Views) {

	    GS_set_draw(GSD_FRONT);
	    GS_ready_draw();

	    GVL_alldraw_vol();

	    GS_done_draw();

	}
    }
    else {
	Fmode &= ~FM_VOL;
    }

    return;
}


void GK_show_list( int flag)
{

        if (flag) {

                Fmode |= FM_LABEL;

                if (Views) {

                GS_draw_all_list();

                }
        }
        else {
                Fmode &= ~FM_LABEL;
        }

        return;
}

