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

#include "ccforest.h"
#include "sortutils.h"
#include "streamutils.h"





#if(0)
/* ---------------------------------------------------------------------- */
static int valueCmp(const keyvalue<long> &a, const keyvalue<long> &b) {
  if(a.dst() < b.dst()) return -1;
  if(a.dst() > b.dst()) return 1;

  if(a.src() < b.src()) return -1;
  if(a.src() > b.src()) return 1;

  return 0;
}



/* ---------------------------------------------------------------------- */

static int valueCmp(const keyvalue<int> &a, const keyvalue<int> &b) {
  if(a.dst() < b.dst()) return -1;
  if(a.dst() > b.dst()) return 1;

  if(a.src() < b.src()) return -1;
  if(a.src() > b.src()) return 1;

  return 0;
}
#endif





/* ---------------------------------------------------------------------- */

template<class T>
ccforest<T>::ccforest() {
  edgeStream = new AMI_STREAM<ccedge>();
  rootStream = new AMI_STREAM<cckeyvalue>();
  superTree = NULL;
  rootCycles = 0;
  foundAllRoots = 0;
  savedRootValid = 0;
}

/* ---------------------------------------------------------------------- */

template<class T>
ccforest<T>::~ccforest() {
  delete edgeStream;
  delete rootStream;
  if(superTree) delete superTree;
}

/* ---------------------------------------------------------------------- */

template<class T>
int ccforest<T>::size() {
  size_t streamLength = edgeStream->stream_len();
  return streamLength;
}

/* ---------------------------------------------------------------------- */


template<class T> 
void ccforest<T>::removeDuplicates(T src, T parent,
				   EMPQueueAdaptive<cckeyvalue,T> &pq) {
  cckeyvalue kv;
  
  while(pq.min(kv) && (src == kv.getPriority())) {
	pq.extract_min(kv);
	if(kv.getValue() != parent) { /* cycles... */
	  rootCycles++;
	  if(parent < kv.getValue()) {
		superTree->insert(parent, kv.getValue());
	  } else {
		superTree->insert(kv.getValue(), parent);
	  }
	  DEBUG_CCFOREST cerr << "ROOT CYCLE DETECTED! " << src << " (" << 
		parent << "," << kv.getValue() << ")" << endl;
	}
  }
}

/* ---------------------------------------------------------------------- */

/* needs to be re-entrant */
template<class T> 
void ccforest<T>::findAllRoots(int depth) {
  if(foundAllRoots) return;
  foundAllRoots = 1;
  Rtimer rt;
  rt_start(rt);

  if(depth > 5) {
	cerr << "WARNING: excessive recursion in ccforest (ignored)" << endl;
  }

  int explicitRootCount = 0;
  assert(!superTree);
  superTree = new ccforest<T>();

  if (stats)
    DEBUG_CCFOREST *stats << "sort edgeStream (by cclabel)): ";
  keyCmpKeyvalueType<T> fo;
  sort(&edgeStream, fo); /* XXX -changed this to use a cmp obj  */

  /* time forward processing */
  EMPQueueAdaptive<cckeyvalue,T> *pq =
	new EMPQueueAdaptive<cckeyvalue,T>();	/* parent queue */

  size_t streamLength = edgeStream->stream_len();
  T prevSrc = T(-1);
  T parent = T(-1);
  ccedge prevEdge;
  for(unsigned int i=0; i<streamLength; i++) {
	ccedge *e;
	AMI_err ae = edgeStream->read_item(&e);
	assert(ae == AMI_ERROR_NO_ERROR);

#if(0)
	if (stats) {
	    DEBUG_CCFOREST *stats << "------------------------------" << endl;
	    DEBUG_CCFOREST *stats << "processing edge " << *e << endl;
	}
	DEBUG_CCFOREST pq->print();
#endif

	if(*e == prevEdge) {
	  if (stats)
	    DEBUG_CCFOREST *stats << "\tduplicate " << *e << " removed\n";
	  continue; /* already have this done */
	}
	prevEdge = *e;

	if (stats)
	  DEBUG_CCFOREST *stats << "processing edge " << *e << endl;

	/* find root (assign parent) */
	if(e->src() != prevSrc) {
	  prevSrc = e->src();
	  cckeyvalue kv;
	  /* check if we have a key we don't use. */
	  while(pq->min(kv) && (kv.getPriority() < e->src())) {
		pq->extract_min(kv);
		assert(kv.src() >= kv.dst());
		removeDuplicates(kv.src(), kv.dst(), *pq);
		ae = rootStream->write_item(kv); /* save root */
		assert(ae == AMI_ERROR_NO_ERROR);	
	  }
	  /* try to find our root */
	  if(pq->min(kv) && ((e->src() == kv.getPriority()))) {
		pq->extract_min(kv);
		parent = kv.getValue();
		removeDuplicates(e->src(), parent, *pq);
	  } else {
		parent = e->src();		/* we are root */
		explicitRootCount++;
		/* technically, we could skip this part. the lookup function
           automatically assumes that values without parents are roots */
	  }

	  /* save result */
	  cckeyvalue kroot(e->src(), parent);
	  assert(kroot.src() >= kroot.dst());
	  ae = rootStream->write_item(kroot);
	  assert(ae == AMI_ERROR_NO_ERROR);
	}
#ifndef NDEBUG
	cckeyvalue kv2;
	assert(pq->is_empty() || (pq->min(kv2) && kv2.getPriority() > e->src()));
#endif

	/* insert */
	cckeyvalue kv(e->dst(), parent);
	assert(kv.src() >= kv.dst());
	pq->insert(kv);

	/* cout << "identified: " << kroot << endl; */
  }

  /* drain the priority queue */
  if (stats)
    DEBUG_CCFOREST *stats << "draining priority queue" << endl;
  while (!pq->is_empty()) {
	cckeyvalue kv;
	pq->extract_min(kv);
	assert(kv.src() >= kv.dst());
	if (stats)
	  DEBUG_CCFOREST *stats << "processing edge " << kv << endl;

	removeDuplicates(kv.src(), kv.dst(), *pq);
	AMI_err ae = rootStream->write_item(kv);
	assert(ae == AMI_ERROR_NO_ERROR);
  }
  delete pq;

  /* note that rootStream is naturally ordered by src */

  if(superTree->size()) {
        if (stats) {
	    DEBUG_CCFOREST *stats << "resolving cycles..." << endl;
	    /* printStream(rootStream); */
	    DEBUG_CCFOREST *stats << "sort rootStream: ";
        }

	AMI_STREAM<cckeyvalue> *sortedRootStream; 
	dstCmpKeyvalueType<T> dstfo;
	sortedRootStream = sort(rootStream, dstfo); 
	/* XXX replaced this to use a cmp object -- laura
	   AMI_STREAM<cckeyvalue>*sortedRootStream=new AMI_STREAM<cckeyvalue>();
	   AMI_err ae = AMI_sort(rootStream, sortedRootStream, valueCmp); 
	   assert(ae == AMI_ERROR_NO_ERROR);
	*/
	delete rootStream;

	cckeyvalue *kv;
	T parent;
	AMI_err ae;
	
	AMI_STREAM<cckeyvalue>* relabeledRootStream
	  = new AMI_STREAM<cckeyvalue>();
	ae = sortedRootStream->seek(0);
	superTree->findAllRoots(depth+1);
	while((ae = sortedRootStream->read_item(&kv)) == AMI_ERROR_NO_ERROR) {
	  parent = superTree->findNextRoot(kv->dst());
	  ae = relabeledRootStream->write_item(cckeyvalue(kv->src(), parent));
	  assert(ae == AMI_ERROR_NO_ERROR);
	}
	delete sortedRootStream;

        if (stats)
	    DEBUG_CCFOREST *stats << "sort relabeledRootStream: ";
	rootStream = sort(relabeledRootStream, fo);
	/* laura: changed  this
	   rootStream = new AMI_STREAM<cckeyvalue>();
	   ae = AMI_sort(relabeledRootStream, rootStream);
	   assert(ae == AMI_ERROR_NO_ERROR);
	*/
	delete relabeledRootStream;

        if (stats)
	    DEBUG_CCFOREST *stats << "resolving cycles... done." << endl;
  }
  rootStream->seek(0);

  if (stats){
    DEBUG_CCFOREST *stats << "Rootstream length="
					  << rootStream->stream_len() << endl;
    DEBUG_CCFOREST printStream(*stats, rootStream);
    DEBUG_CCFOREST *stats << "Explicit root count=" << explicitRootCount << endl;
  }

  rt_stop(rt);
  if (stats)
    stats->recordTime("ccforest::findAllRoots",  (long int)rt_seconds(rt));
}


/* ---------------------------------------------------------------------- */

template<class T>
void 
ccforest<T>::insert(const T& i, const T& j) {
  ccedge e(i,j);
  /* assert(i<j);  not required, as long as it's consistent. */
  assert(i!=j);					/* meaningless */
  AMI_err ae = edgeStream->write_item(e);
  assert(ae == AMI_ERROR_NO_ERROR);
  /* cout << "INST " << i << ", " << j << endl; */
}

/* ---------------------------------------------------------------------- */

template<class T>
T 
ccforest<T>::findNextRoot(const T& i) {
  AMI_err ae;
  cckeyvalue *kroot;
  T retRoot;
  
  findAllRoots();   /* find all the roots if not done */
  
  if (stats)
    DEBUG_CCFOREST *stats << "looking for " << i << endl;
  if(!savedRootValid || savedRoot.src() < i) { /* need to read more */
    ae = rootStream->read_item(&kroot);
	while(ae == AMI_ERROR_NO_ERROR && kroot->src() < i) {
	  ae = rootStream->read_item(&kroot);
	}
	if(ae == AMI_ERROR_NO_ERROR) {
	  savedRoot = *kroot;
	  savedRootValid = 1;
	} else {
	  savedRootValid = -1;		/* to avoid reading uselessly */
	}
  }
  
  if(savedRootValid==1 && savedRoot.src() == i) { /* check savedRoot */
    retRoot = savedRoot.dst();
    if (stats)
      DEBUG_CCFOREST *stats << "using saved/found value" << endl;
  } else {
    if (stats)
      DEBUG_CCFOREST *stats << "not found" << endl;
    retRoot = i;
  }
  if (stats)
    DEBUG_CCFOREST *stats << "lookup for " << i << " gives " << retRoot
		      << "; saved = " << savedRoot << endl;
  return retRoot;
}

/* ---------------------------------------------------------------------- */

template<class T>
void
ccforest<T>::printRootStream() {
  findAllRoots();  /* find all the roots if not done */
  printStream(cout, rootStream);
}


template<class T> void
ccforest<T>::printEdgeStream() {
  printStream(cout, edgeStream);
}

/* ---------------------------------------------------------------------- */

template class keyvalue<cclabel_type>;
template class  keyCmpKeyvalueType<cclabel_type>;
template class ccforest<cclabel_type>;


