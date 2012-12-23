/*
   This code is described in "Computational Geometry in C" (Second Edition),
   Chapter 4.  It is not written to be comprehensible without the 
   explanation in that book.

   Input: 3n integer coordinates for the points.
   Output: the 3D convex hull, in postscript with embedded comments
   showing the vertices and faces.

   Compile: gcc -o chull chull.c

   Written by Joseph O'Rourke, with contributions by 
   Kristy Anderson, John Kutcher, Catherine Schevon, Susan Weller.
   Last modified: March 1998
   Questions to orourke@cs.smith.edu.
   --------------------------------------------------------------------
   This code is Copyright 1998 by Joseph O'Rourke.  It may be freely 
   redistributed in its entirety provided that this copyright notice is 
   not removed.
   --------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "globals.h"

/*Define Boolean type */
typedef enum
{ BFALSE, BTRUE } bool;

/* Define vertex indices. */
#define X   0
#define Y   1
#define Z   2

/* Define structures for vertices, edges and faces */
typedef struct tVertexStructure tsVertex;
typedef tsVertex *tVertex;

typedef struct tEdgeStructure tsEdge;
typedef tsEdge *tEdge;

typedef struct tFaceStructure tsFace;
typedef tsFace *tFace;

struct tVertexStructure
{
    double v[3];
    int vnum;
    tEdge duplicate;		/* pointer to incident cone edge (or NULL) */
    bool onhull;		/* T iff point on hull. */
    bool mark;			/* T iff point already processed. */
    tVertex next, prev;
};

struct tEdgeStructure
{
    tFace adjface[2];
    tVertex endpts[2];
    tFace newface;		/* pointer to incident cone face. */
    bool delete;		/* T iff edge should be delete. */
    tEdge next, prev;
};

struct tFaceStructure
{
    tEdge edge[3];
    tVertex vertex[3];
    bool visible;		/* T iff face visible from new point. */
    tFace next, prev;
};

/* Define flags */
#define ONHULL   	BTRUE
#define REMOVED  	BTRUE
#define VISIBLE  	BTRUE
#define PROCESSED	BTRUE

/* Global variable definitions */
tVertex vertices = NULL;
tEdge edges = NULL;
tFace faces = NULL;

/* Function declarations */
tVertex MakeNullVertex(void);
void ReadVertices(double *px, double *py, double *pz, int num_points);
void writeVertices(struct Map_info *Map);
int DoubleTriangle(void);
void ConstructHull(void);
bool AddOne(tVertex p);
int VolumeSign(tFace f, tVertex p);
tFace MakeConeFace(tEdge e, tVertex p);
void MakeCcw(tFace f, tEdge e, tVertex p);
tEdge MakeNullEdge(void);
tFace MakeNullFace(void);
tFace MakeFace(tVertex v0, tVertex v1, tVertex v2, tFace f);
void CleanUp(void);
void CleanEdges(void);
void CleanFaces(void);
void CleanVertices(void);
bool Collinear(tVertex a, tVertex b, tVertex c);

#include "macros.h"


/*

   Release all memory allocated for edges, faces and vertices

 */
void freeMem(void)
{
    tEdge e;			/* Primary index into edge list. */
    tFace f;			/* Primary pointer into face list. */
    tVertex v;
    tEdge te;			/* Temporary edge pointer. */
    tFace tf;			/* Temporary face pointer. */
    tVertex tv;			/* Temporary vertex pointer. */

    e = edges;
    do {
	te = e;
	e = e->next;
	DELETE(edges, te);
    } while (e != edges);

    f = faces;
    do {
	tf = f;
	f = f->next;
	DELETE(faces, tf);
    } while (f != faces);

    v = vertices;
    do {
	tv = v;
	v = v->next;
	DELETE(vertices, tv);
    } while (v != vertices);

    FREE(te);
    FREE(tf);
    FREE(tv);

    DELETE(edges, e);
    DELETE(faces, f);
    DELETE(vertices, v);

    FREE(edges);
    FREE(faces);
    FREE(vertices);

}


/*-------------------------------------------------------------------*/
int make3DHull(double *px, double *py, double *pz, int num_points,
	       struct Map_info *Map)
{
    int error;

    ReadVertices(px, py, pz, num_points);

    error = DoubleTriangle();
    if (error < 0) {
	G_fatal_error
	    ("All points of 3D input map are in the same plane.\n  Cannot create a 3D hull.");
    }

    ConstructHull();

    writeVertices(Map);

    freeMem();

    return (0);
}

/*---------------------------------------------------------------------
MakeNullVertex: Makes a vertex, nulls out fields.
---------------------------------------------------------------------*/
tVertex MakeNullVertex(void)
{
    tVertex v;

    NEW(v, tsVertex);
    v->duplicate = NULL;
    v->onhull = !ONHULL;
    v->mark = !PROCESSED;
    ADD(vertices, v);

    return v;
}

/*---------------------------------------------------------------------
ReadVertices: Reads in the vertices, and links them into a circular
list with MakeNullVertex.  There is no need for the # of vertices to be
the first line: the function looks for EOF instead.  Sets the global
variable vertices via the ADD macro.
---------------------------------------------------------------------*/
void ReadVertices(double *px, double *py, double *pz, int num_points)
{
    tVertex v;
    int vnum = 0;
    int i;

    G_important_message(_("Reading 3D vertices..."));
    for (i = 0; i < num_points; i++) {
	v = MakeNullVertex();
	v->v[X] = px[i];
	v->v[Y] = py[i];
	v->v[Z] = pz[i];
	v->vnum = vnum++;
	G_percent(i, (num_points - 1), 1);
    }
    fflush(stdout);
}


/*---------------------------------------------------------------------
Outputs the 3D triangles to a GRASS 3d vector map.
---------------------------------------------------------------------*/
void writeVertices(struct Map_info *Map)
{
    /* Pointers to vertices, edges, faces. */
    tFace f;
    double *px, *py, *pz;
    double fx, fy, fz;
    double kx, ky, kz;

    long int cat, num_faces;

    struct line_pnts *Points;
    struct line_cats *Cats;


    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    px = G_malloc(sizeof(double) * 4);
    py = G_malloc(sizeof(double) * 4);
    pz = G_malloc(sizeof(double) * 4);

    f = faces;
    num_faces = 0;
    cat = 0;
    kx = 0.0;
    ky = 0.0;
    kz = 0.0;

    G_message("Writing faces and kernel to output map ...");

    do {

	num_faces++;

	/* write one triangular face */
	px[0] = ((double)(f->vertex[0]->v[X]));
	py[0] = ((double)(f->vertex[0]->v[Y]));
	pz[0] = ((double)(f->vertex[0]->v[Z]));

	px[1] = ((double)(f->vertex[1]->v[X]));
	py[1] = ((double)(f->vertex[1]->v[Y]));
	pz[1] = ((double)(f->vertex[1]->v[Z]));

	px[2] = ((double)(f->vertex[2]->v[X]));
	py[2] = ((double)(f->vertex[2]->v[Y]));
	pz[2] = ((double)(f->vertex[2]->v[Z]));

	px[3] = ((double)(f->vertex[0]->v[X]));
	py[3] = ((double)(f->vertex[0]->v[Y]));
	pz[3] = ((double)(f->vertex[0]->v[Z]));

	/* kernel position: 1st get 3D center of this face */
	fx = (px[0] + px[1] + px[2]) / 3.0;
	fy = (py[0] + py[1] + py[2]) / 3.0;
	fz = (pz[0] + pz[1] + pz[2]) / 3.0;

	/* kernel position: now add this to kernel coordinates */
	kx = kx + fx;
	ky = ky + fy;
	kz = kz + fz;

	/* write out face */
	Vect_copy_xyz_to_pnts(Points, px, py, pz, 4);
	cat++;
	Vect_cat_set(Cats, 1, cat);
	Vect_write_line(Map, GV_FACE, Points, Cats);

	f = f->next;

    } while (f != faces);

    /* write kernel for the center of the whole hull */
    kx = kx / num_faces;
    ky = ky / num_faces;
    kz = kz / num_faces;
    Vect_cat_set(Cats, 1, cat + 1);
    Vect_copy_xyz_to_pnts(Points, &kx, &ky, &kz, 1);
    Vect_write_line(Map, GV_KERNEL, Points, Cats);

    Vect_destroy_line_struct(Points);

    fflush(stdout);

    G_free(px);
    G_free(py);
    G_free(pz);

}


/*---------------------------------------------------------------------
 DoubleTriangle builds the initial double triangle.  It first finds 3 
 noncollinear points and makes two faces out of them, in opposite order.
 It then finds a fourth point that is not coplanar with that face.  The  
 vertices are stored in the face structure in counterclockwise order so 
 that the volume between the face and the point is negative. Lastly, the
 3 newfaces to the fourth point are constructed and the data structures
 are cleaned up. 
---------------------------------------------------------------------*/

/* RETURN:      0 if OK */
/*              -1 if all points collinear */
/*              -2 if all points coplanar */

int DoubleTriangle(void)
{
    tVertex v0, v1, v2, v3;
    tFace f0, f1 = NULL;
    long int vol;

    /* Find 3 noncollinear points. */
    v0 = vertices;
    while (Collinear(v0, v0->next, v0->next->next)) {
	if ((v0 = v0->next) == vertices) {
	    G_warning("DoubleTriangle:  All points are collinear!\n");
	    return (-1);
	}
    }
    v1 = v0->next;
    v2 = v1->next;

    /* Mark the vertices as processed. */
    v0->mark = PROCESSED;
    v1->mark = PROCESSED;
    v2->mark = PROCESSED;

    /* Create the two "twin" faces. */
    f0 = MakeFace(v0, v1, v2, f1);
    f1 = MakeFace(v2, v1, v0, f0);

    /* Link adjacent face fields. */
    f0->edge[0]->adjface[1] = f1;
    f0->edge[1]->adjface[1] = f1;
    f0->edge[2]->adjface[1] = f1;
    f1->edge[0]->adjface[1] = f0;
    f1->edge[1]->adjface[1] = f0;
    f1->edge[2]->adjface[1] = f0;

    /* Find a fourth, noncoplanar point to form tetrahedron. */
    v3 = v2->next;
    vol = VolumeSign(f0, v3);
    while (!vol) {
	if ((v3 = v3->next) == v0) {
	    G_warning("DoubleTriangle:  All points are coplanar!\n");
	    return (-2);
	}
	vol = VolumeSign(f0, v3);
    }

    /* Insure that v3 will be the first added. */
    vertices = v3;

    return (0);
}


/*---------------------------------------------------------------------
ConstructHull adds the vertices to the hull one at a time.  The hull
vertices are those in the list marked as onhull.
---------------------------------------------------------------------*/
void ConstructHull(void)
{
    tVertex v, vnext;
    bool changed;		/* T if addition changes hull; not used. */
    int i;
    int numVertices;


    G_important_message(_("Constructing 3D hull..."));

    v = vertices;
    i = 0;
    do {
	vnext = v->next;
	v = vnext;
	i++;
    } while (v != vertices);
    numVertices = i;

    v = vertices;
    i = 0;
    do {
	vnext = v->next;
	if (!v->mark) {
	    v->mark = PROCESSED;
	    changed = AddOne(v);
	    CleanUp();
	}
	v = vnext;
	i++;

	G_percent(i, numVertices, 1);

    } while (v != vertices);

    fflush(stdout);

}

/*---------------------------------------------------------------------
AddOne is passed a vertex.  It first determines all faces visible from 
that point.  If none are visible then the point is marked as not 
onhull.  Next is a loop over edges.  If both faces adjacent to an edge
are visible, then the edge is marked for deletion.  If just one of the
adjacent faces is visible then a new face is constructed.
---------------------------------------------------------------------*/
bool AddOne(tVertex p)
{
    tFace f;
    tEdge e, temp;
    long int vol;
    bool vis = BFALSE;


    /* Mark faces visible from p. */
    f = faces;
    do {
	vol = VolumeSign(f, p);

	if (vol < 0) {
	    f->visible = VISIBLE;
	    vis = BTRUE;
	}
	f = f->next;
    } while (f != faces);

    /* If no faces are visible from p, then p is inside the hull. */
    if (!vis) {
	p->onhull = !ONHULL;
	return BFALSE;
    }

    /* Mark edges in interior of visible region for deletion.
       Erect a newface based on each border edge. */
    e = edges;
    do {
	temp = e->next;
	if (e->adjface[0]->visible && e->adjface[1]->visible)
	    /* e interior: mark for deletion. */
	    e->delete = REMOVED;
	else if (e->adjface[0]->visible || e->adjface[1]->visible)
	    /* e border: make a new face. */
	    e->newface = MakeConeFace(e, p);
	e = temp;
    } while (e != edges);
    return BTRUE;
}

/*---------------------------------------------------------------------
VolumeSign returns the sign of the volume of the tetrahedron determined by f
and p.  VolumeSign is +1 iff p is on the negative side of f,
where the positive side is determined by the rh-rule.  So the volume 
is positive if the ccw normal to f points outside the tetrahedron.
The final fewer-multiplications form is due to Bob Williamson.
---------------------------------------------------------------------*/
int VolumeSign(tFace f, tVertex p)
{
    double vol;
    double ax, ay, az, bx, by, bz, cx, cy, cz;

    ax = f->vertex[0]->v[X] - p->v[X];
    ay = f->vertex[0]->v[Y] - p->v[Y];
    az = f->vertex[0]->v[Z] - p->v[Z];
    bx = f->vertex[1]->v[X] - p->v[X];
    by = f->vertex[1]->v[Y] - p->v[Y];
    bz = f->vertex[1]->v[Z] - p->v[Z];
    cx = f->vertex[2]->v[X] - p->v[X];
    cy = f->vertex[2]->v[Y] - p->v[Y];
    cz = f->vertex[2]->v[Z] - p->v[Z];

    vol = ax * (by * cz - bz * cy)
	+ ay * (bz * cx - bx * cz)
	+ az * (bx * cy - by * cx);

    /* The volume should be an integer. */
    if (vol > 0.0)
	return 1;
    else if (vol < -0.0)
	return -1;
    else
	return 0;
}


/*---------------------------------------------------------------------
MakeConeFace makes a new face and two new edges between the 
edge and the point that are passed to it. It returns a pointer to
the new face.
---------------------------------------------------------------------*/
tFace MakeConeFace(tEdge e, tVertex p)
{
    tEdge new_edge[2];
    tFace new_face;
    int i, j;

    /* Make two new edges (if don't already exist). */
    for (i = 0; i < 2; ++i)
	/* If the edge exists, copy it into new_edge. */
	if (!(new_edge[i] = e->endpts[i]->duplicate)) {
	    /* Otherwise (duplicate is NULL), MakeNullEdge. */
	    new_edge[i] = MakeNullEdge();
	    new_edge[i]->endpts[0] = e->endpts[i];
	    new_edge[i]->endpts[1] = p;
	    e->endpts[i]->duplicate = new_edge[i];
	}

    /* Make the new face. */
    new_face = MakeNullFace();
    new_face->edge[0] = e;
    new_face->edge[1] = new_edge[0];
    new_face->edge[2] = new_edge[1];
    MakeCcw(new_face, e, p);

    /* Set the adjacent face pointers. */
    for (i = 0; i < 2; ++i)
	for (j = 0; j < 2; ++j)
	    /* Only one NULL link should be set to new_face. */
	    if (!new_edge[i]->adjface[j]) {
		new_edge[i]->adjface[j] = new_face;
		break;
	    }

    return new_face;
}

/*---------------------------------------------------------------------
MakeCcw puts the vertices in the face structure in counterclock wise 
order.  We want to store the vertices in the same 
order as in the visible face.  The third vertex is always p.
---------------------------------------------------------------------*/
void MakeCcw(tFace f, tEdge e, tVertex p)
{
    tFace fv;			/* The visible face adjacent to e */
    int i;			/* Index of e->endpoint[0] in fv. */
    tEdge s;			/* Temporary, for swapping */

    if (e->adjface[0]->visible)
	fv = e->adjface[0];
    else
	fv = e->adjface[1];

    /* Set vertex[0] & [1] of f to have the same orientation
       as do the corresponding vertices of fv. */
    for (i = 0; fv->vertex[i] != e->endpts[0]; ++i) ;
    /* Orient f the same as fv. */
    if (fv->vertex[(i + 1) % 3] != e->endpts[1]) {
	f->vertex[0] = e->endpts[1];
	f->vertex[1] = e->endpts[0];
    }
    else {
	f->vertex[0] = e->endpts[0];
	f->vertex[1] = e->endpts[1];
	SWAP(s, f->edge[1], f->edge[2]);
    }
    /* This swap is tricky. e is edge[0]. edge[1] is based on endpt[0],
       edge[2] on endpt[1].  So if e is oriented "forwards," we
       need to move edge[1] to follow [0], because it precedes. */

    f->vertex[2] = p;
}

/*---------------------------------------------------------------------
MakeNullEdge creates a new cell and initializes all pointers to NULL
and sets all flags to off.  It returns a pointer to the empty cell.
---------------------------------------------------------------------*/
tEdge MakeNullEdge(void)
{
    tEdge e;

    NEW(e, tsEdge);
    e->adjface[0] = e->adjface[1] = e->newface = NULL;
    e->endpts[0] = e->endpts[1] = NULL;
    e->delete = !REMOVED;
    ADD(edges, e);
    return e;
}

/*--------------------------------------------------------------------
MakeNullFace creates a new face structure and initializes all of its
flags to NULL and sets all the flags to off.  It returns a pointer
to the empty cell.
---------------------------------------------------------------------*/
tFace MakeNullFace(void)
{
    tFace f;
    int i;

    NEW(f, tsFace);
    for (i = 0; i < 3; ++i) {
	f->edge[i] = NULL;
	f->vertex[i] = NULL;
    }
    f->visible = !VISIBLE;
    ADD(faces, f);
    return f;
}

/*---------------------------------------------------------------------
MakeFace creates a new face structure from three vertices (in ccw
order).  It returns a pointer to the face.
---------------------------------------------------------------------*/
tFace MakeFace(tVertex v0, tVertex v1, tVertex v2, tFace fold)
{
    tFace f;
    tEdge e0, e1, e2;

    /* Create edges of the initial triangle. */
    if (!fold) {
	e0 = MakeNullEdge();
	e1 = MakeNullEdge();
	e2 = MakeNullEdge();
    }
    else {			/* Copy from fold, in reverse order. */
	e0 = fold->edge[2];
	e1 = fold->edge[1];
	e2 = fold->edge[0];
    }
    e0->endpts[0] = v0;
    e0->endpts[1] = v1;
    e1->endpts[0] = v1;
    e1->endpts[1] = v2;
    e2->endpts[0] = v2;
    e2->endpts[1] = v0;

    /* Create face for triangle. */
    f = MakeNullFace();
    f->edge[0] = e0;
    f->edge[1] = e1;
    f->edge[2] = e2;
    f->vertex[0] = v0;
    f->vertex[1] = v1;
    f->vertex[2] = v2;

    /* Link edges to face. */
    e0->adjface[0] = e1->adjface[0] = e2->adjface[0] = f;

    return f;
}

/*---------------------------------------------------------------------
CleanUp goes through each data structure list and clears all
flags and NULLs out some pointers.  The order of processing
(edges, faces, vertices) is important.
---------------------------------------------------------------------*/
void CleanUp(void)
{
    CleanEdges();
    CleanFaces();
    CleanVertices();
}

/*---------------------------------------------------------------------
CleanEdges runs through the edge list and cleans up the structure.
If there is a newface then it will put that face in place of the 
visible face and NULL out newface. It also deletes so marked edges.
---------------------------------------------------------------------*/
void CleanEdges(void)
{
    tEdge e;			/* Primary index into edge list. */
    tEdge t;			/* Temporary edge pointer. */

    /* Integrate the newface's into the data structure. */
    /* Check every edge. */
    e = edges;
    do {
	if (e->newface) {
	    if (e->adjface[0]->visible)
		e->adjface[0] = e->newface;
	    else
		e->adjface[1] = e->newface;
	    e->newface = NULL;
	}
	e = e->next;
    } while (e != edges);

    /* Delete any edges marked for deletion. */
    while (edges && edges->delete) {
	e = edges;
	DELETE(edges, e);
    }
    e = edges->next;
    do {
	if (e->delete) {
	    t = e;
	    e = e->next;
	    DELETE(edges, t);
	}
	else
	    e = e->next;
    } while (e != edges);
}

/*---------------------------------------------------------------------
CleanFaces runs through the face list and deletes any face marked visible.
---------------------------------------------------------------------*/
void CleanFaces(void)
{
    tFace f;			/* Primary pointer into face list. */
    tFace t;			/* Temporary pointer, for deleting. */


    while (faces && faces->visible) {
	f = faces;
	DELETE(faces, f);
    }
    f = faces->next;
    do {
	if (f->visible) {
	    t = f;
	    f = f->next;
	    DELETE(faces, t);
	}
	else
	    f = f->next;
    } while (f != faces);
}

/*---------------------------------------------------------------------
CleanVertices runs through the vertex list and deletes the 
vertices that are marked as processed but are not incident to any 
undeleted edges. 
---------------------------------------------------------------------*/
void CleanVertices(void)
{
    tEdge e;
    tVertex v, t;

    /* Mark all vertices incident to some undeleted edge as on the hull. */
    e = edges;
    do {
	e->endpts[0]->onhull = e->endpts[1]->onhull = ONHULL;
	e = e->next;
    } while (e != edges);

    /* Delete all vertices that have been processed but
       are not on the hull. */
    while (vertices && vertices->mark && !vertices->onhull) {
	v = vertices;
	DELETE(vertices, v);
    }
    v = vertices->next;
    do {
	if (v->mark && !v->onhull) {
	    t = v;
	    v = v->next;
	    DELETE(vertices, t)
	}
	else
	    v = v->next;
    } while (v != vertices);

    /* Reset flags. */
    v = vertices;
    do {
	v->duplicate = NULL;
	v->onhull = !ONHULL;
	v = v->next;
    } while (v != vertices);
}

/*---------------------------------------------------------------------
Collinear checks to see if the three points given are collinear,
by checking to see if each element of the cross product is zero.
---------------------------------------------------------------------*/
bool Collinear(tVertex a, tVertex b, tVertex c)
{
    return
	(c->v[Z] - a->v[Z]) * (b->v[Y] - a->v[Y]) -
	(b->v[Z] - a->v[Z]) * (c->v[Y] - a->v[Y]) == 0
	&& (b->v[Z] - a->v[Z]) * (c->v[X] - a->v[X]) -
	(b->v[X] - a->v[X]) * (c->v[Z] - a->v[Z]) == 0
	&& (b->v[X] - a->v[X]) * (c->v[Y] - a->v[Y]) -
	(b->v[Y] - a->v[Y]) * (c->v[X] - a->v[X]) == 0;
}
