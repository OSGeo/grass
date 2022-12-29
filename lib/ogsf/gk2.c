/*!
   \file lib/ogsf/gk2.c

   \brief OGSF library - setting and manipulating keyframes animation

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL, GMSL/University of Illinois
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/ogsf.h>

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

/*!
   \brief Set interpolation mode 

   \param mode interpolation mode (KF_LINEAR or KF_SPLINE)

   \return 1 on success
   \return -1 on error (invalid interpolation mode)
 */
int GK_set_interpmode(int mode)
{
    if (KF_LEGAL_MODE(mode)) {
	Interpmode = mode;
	return (1);
    }

    return (-1);
}

/*!
   \brief Set value for tension when interpmode is KF_SPLINE. 

   \param tens value tens should be between 0.0; 1.0.
 */
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

/*!
   \brief Show tension stop ?

   Use GK_showtension_start/GK_update_tension/GK_showtension_stop to
   initialize and stop multi-view display of path when changing
   tension.
 */
void GK_showtension_stop(void)
{
    return;
}

/*!
   \brief Update tension
 */
void GK_update_tension(void)
{
    if (Views) {
	GK_update_frames();
    }

    return;
}

/*!
   \brief Print keyframe info

   \param name filename
 */
void GK_print_keys(const char *name)
{
    Keylist *k;
    FILE *fp;
    int cnt = 1;

    if (NULL == (fp = fopen(name, "w"))) {
	G_fatal_error(_("Unable to open file <%s> for writing"), name);
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

/*!
   \brief Recalculate path using the current number of frames requested.

   Call after changing number of frames or when
   Keyframes change.
 */
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
	    G_warning(_("Check no. of frames requested and keyframes marked"));
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
	    G_warning(_("Check no. of frames requested and keyframes marked"));
	}
    }

    return;
}

/*!
   \brief Set the number of frames to be interpolated from keyframes

   \param newsteps number of frames
 */
void GK_set_numsteps(int newsteps)
{
    Viewsteps = newsteps;
    GK_update_frames();

    return;
}

/*!
   \brief Deletes all keyframes, resets field masks.

   Doesn't change number of frames requested.
 */
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

/*!
   \brief Move keyframe

   Precis works as in other functions - to identify keyframe to move.
   Only the first keyframe in the precis range will be moved.

   \param oldpos old position
   \param precis precision value
   \param newpos new position

   \return number of keys moved (1 or 0)
 */
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

/*!
   Delete keyframe

   The values pos and precis are used to determine which keyframes to
   delete.  Any keyframes with their position within precis of pos will
   be deleted if justone is zero.  If justone is non-zero, only the first
   (lowest pos) keyframe in the range will be deleted.

   \param pos position
   \param precis precision
   \param justone delete only one keyframe

   \return number of keys deleted.
 */
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

/*!
   \brief Add keyframe

   The pos value is the relative position in the animation for this
   particular keyframe - used to compare relative distance to neighboring
   keyframes, it can be any floating point value.

   The fmask value can be any of the following or'd together:    
   - KF_FROMX_MASK    
   - KF_FROMY_MASK    
   - KF_FROMZ_MASK    
   - KF_FROM_MASK (KF_FROMX_MASK | KF_FROMY_MASK | KF_FROMZ_MASK) 

   - KF_DIRX_MASK    
   - KF_DIRY_MASK    
   - KF_DIRZ_MASK    
   - KF_DIR_MASK (KF_DIRX_MASK | KF_DIRY_MASK | KF_DIRZ_MASK) 

   - KF_FOV_MASK    
   - KF_TWIST_MASK    

   - KF_ALL_MASK (KF_FROM_MASK | KF_DIR_MASK | KF_FOV_MASK | KF_TWIST_MASK) 

   Other fields will be added later.

   The value precis and the boolean force_replace are used to determine
   if a keyframe should be considered to be at the same position as a
   pre-existing keyframe. e.g., if anykey.pos - newkey.pos &lt;= precis,
   GK_add_key() will fail unless force_replace is TRUE.

   \param pos position
   \param fmaks
   \param force_replace
   \param precis precision value

   \return 1 if key is added
   \return -1 key not added
 */
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

    G_debug(3, "KEY FROM: %f %f %f", tmp[X], tmp[Y], tmp[Z]);

    /* Instead of View Dir try get_focus (view center) */
    /* View Dir is implied from eye and center position */
    /*    GS_get_viewdir(tmp); */

    /* ACS 1 line: was      GS_get_focus(tmp);
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

/*!
   \brief Moves the animation to frame number "step".

   Step should be a value between 1 and the number of frames.  If
   render is non-zero, calls draw_all.

   \param step step value
   \param render
 */
void GK_do_framestep(int step, int render)
{
    if (Views) {
	if (step > 0 && step <= Viewsteps) {
	    gk_follow_frames(Views, Viewsteps, Keys, step, 1, render, Fmode);
	}
    }

    return;
}

/*!
   \brief Draw the current path

   \param flag
 */
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

/*!
   \brief Show vector sets

   \param flag
 */
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

/*!
   \brief Show point sets

   \param flag
 */
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

/*!
   \brief Show volumes

   \param flag
 */
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

/*!
   \brief Show list

   \param flag
 */
void GK_show_list(int flag)
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
