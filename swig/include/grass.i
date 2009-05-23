
%include "common.i"

%include "grass/gis.h"
%include "grass/gisdefs.h"

%pythoncode %{
import sys

def G_gisinit(pgm):
	G__gisinit(GIS_H_VERSION, pgm)

def G__get_trace():
	f = sys._getframe(1)
	ln = f.f_lineno
	fi = f.f_code.co_filename
	return fi, ln

def G_malloc(n):
	fi, ln = G__get_trace()
	return G__malloc(fi, ln, n)

def G_calloc(m, n):
	fi, ln = G__get_trace()
	return G__calloc(fi, ln, m, n)

def G_realloc(p, n):
	fi, ln = G__get_trace()
	return G__realloc(fi, ln, p, n)
%}

