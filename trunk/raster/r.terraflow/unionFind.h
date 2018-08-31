/****************************************************************************
 * 
 *  MODULE:	r.terraflow
 *
 *  COPYRIGHT (C) 2007 Laura Toma
 *   
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/

#ifndef __UNION_FIND
#define __UNION_FIND

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

/* initial range guesstimate */
#define UNION_INITIAL_SIZE 2000


/* maintains a collection of disjoint dynamic sets; elements are in
the range 1..maxsize-1 and range is increased dynamically; if element x
is not in the set then parent[x] = 0;
*/

template<class T>
class unionFind { 
private: 
  T* parent; 
  T*  rank; 
  T maxsize;
  
public:
  unionFind();
  ~unionFind();
  
  /* return true if element x is in the structure */
  inline bool inSet(T x);

  void print();

  /* create a new set whose only member (and representative) is x;
	 since x is disjoint we require x not be already in the set; */
  inline void makeSet(T x);
  
  /* unites the dynamic sets that contain x and y; */
  inline void makeUnion(T x, T y);
  
  /* returns the representative of the set containing x */
  inline T findSet(T x);
  
  /* returns main memory usage if the max nb of makeset calls is n */
  inline size_t mmusage(T n);
};


/************************************************************/
template<class T>
unionFind<T>::unionFind() {
  maxsize = UNION_INITIAL_SIZE;
  /*  parent = new (long)[maxsize]; */
  parent = (T*)calloc(maxsize, sizeof(T));
  assert(parent);
  /*  rank = new (long)[maxsize]; */
  rank = (T*)calloc(maxsize, sizeof(T));
  assert(rank);
}

/************************************************************/
template<class T>
unionFind<T>::~unionFind() {
  /* if (parent) delete [] parent; 
	 if (rank) delete [] rank; 
  */
  if (parent) free(parent);
  if (rank) free(rank);
}

/************************************************************/
/* return true if element x is in the structure */
template<class T>
inline bool 
unionFind<T>::inSet(T x) {
  if (x > 0 && x < maxsize && parent[x] > 0) {
    return true;
  } else {
    return false;
  }
}



/************************************************************/
template<class T>
void 
unionFind<T>::print() {
  for (T i=0; i< maxsize; i++) {
    if (parent[i] == 0) cout << "x ";
    else cout << parent[i] << " ";
  }
  cout << "\n";
}

/************************************************************/
/* 
create a new set whose only member (and representative) is x; since x
is disjoint we require x not be already in the set; */
template<class T>
inline void
unionFind<T>::makeSet(T x) {
  /* cout << "makeSet " << x << "\n"; print(); */
  assert(x > 0);
  if (x >= maxsize) {
    /* reallocate parent */
    cout << "UnionFind::makeSet: reallocate double " << maxsize << "\n";
    parent = (T*)realloc(parent, 2*maxsize*sizeof(T));
    assert(parent);
    memset(parent + maxsize, 0, maxsize*sizeof(T));
    /*reallocate rank */
    rank = (T*)realloc(rank, 2*maxsize*sizeof(T));
    assert(rank);
    memset(rank + maxsize, 0, maxsize*sizeof(T));
    /*update maxsize */
    maxsize *= 2;
  }  
  /*since x is disjoint we require x not be already in the set; should
    relax this..*/
  assert(!inSet(x));
  parent[x] = x;
  rank[x] = 0;
}

/************************************************************/
/* returns the representative of the set containing x */
template<class T>
inline T
unionFind<T>::findSet(T x) {
  /* valid entry */
  assert(inSet(x));
  if (parent[x] != x) {
    /* path compression heuristic */
    parent[x] = findSet(parent[x]);
  }
  /* parent[x] must be a root */
  assert(parent[parent[x]] == parent[x]);
  return parent[x];
}


/************************************************************/
/* unites the dynamic sets that contain x and y; */
template<class T>
inline void 
unionFind<T>::makeUnion(T x, T y) {
  assert(inSet(x) && inSet(y));
  T setx = findSet(x);
  T sety = findSet(y);
  if (setx == sety) return;

  /* union by rank heuristic */
  assert(inSet(x) && inSet(y));
  if (rank[setx] > rank[sety]) {
    /* hook sety onto setx */
    parent[sety] = setx;
  } else {
    /* hook setx onto sety */
    parent[setx] = sety;
    /* if equal increase rank */
    if (sety == sety) {
      rank[sety]++;
    }
  }
  /* this does not have side effects.. */
  assert(findSet(x) == findSet(y));
}


/************************************************************/
/* returns main memory usage if the max nb of makeset calls is n */
template<class T>
inline size_t 
unionFind<T>::mmusage(T n) {
  if (n < UNION_INITIAL_SIZE) n = UNION_INITIAL_SIZE;
  return (n * sizeof(T));
}


#endif

