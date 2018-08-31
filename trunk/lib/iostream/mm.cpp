/****************************************************************************
 * 
 *  MODULE:     iostream
 *

 *  COPYRIGHT (C) 2007 Laura Toma
 *   
 * 

 *  Iostream is a library that implements streams, external memory
 *  sorting on streams, and an external memory priority queue on
 *  streams. These are the fundamental components used in external
 *  memory algorithms.  

 * Credits: The library was developed by Laura Toma.  The kernel of
 * class STREAM is based on the similar class existent in the GPL TPIE
 * project developed at Duke University. The sorting and priority
 * queue have been developed by Laura Toma based on communications
 * with Rajiv Wickremesinghe. The library was developed as part of
 * porting Terraflow to GRASS in 2001.  PEARL upgrades in 2003 by
 * Rajiv Wickremesinghe as part of the Terracost project.

 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.  *
 *  **************************************************************************/



// A simple registration based memory manager.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

//#include <mm.h>
#include <grass/iostream/mm.h>

#define MM_DEBUG if(0)



/* ************************************************************ */
MM_register::MM_register() {
 
   instances++;
    if (instances > 1) {
      cerr << "MM_register(): Only 1 instance of MM_register should exist.\n";
      assert(0); //core dump if debugging
      exit(1);
    }
    assert(instances == 1);
 
    // by default, we ignore if memory limit is exceeded   
    register_new = MM_IGNORE_MEMORY_EXCEEDED;
}



/* ************************************************************ */
MM_register::~MM_register(void) {
 
  if (instances > 1) {
    cerr << "MM_register(): Only 1 instance of MM_register should exist.\n";
    assert(0); //core dump if debugging    
    exit(1);
  }
  assert(instances == 1);
  instances--;
}


/* ************************************************************ */
void MM_register::print() {
  
  size_t availMB = (remaining >> 20);
  if (remaining) {
    cout << "available memory: " << availMB << "MB "
	 << "(" << remaining << "B)"
	 << endl; 
  } else {
    cout << "available memory: " << remaining << "B, exceeding: " 
	 << used - user_limit << "B"
	 << endl; 
  }
}


/* ************************************************************ */
// User-callable method to set allowable memory size
MM_err MM_register::set_memory_limit (size_t new_limit) {

  assert( new_limit > 0); 
  if (used > new_limit) {
    //    return MM_ERROR_EXCESSIVE_ALLOCATION;    
    switch (register_new) {
    case MM_ABORT_ON_MEMORY_EXCEEDED:
      cerr << " MM_register::set_memory_limit to " << new_limit 
      	   << ", used " << used << ". allocation exceeds new limit.\n";
      cerr.flush();
      assert(0); //core dump if debugging
      exit(1);
      break;
      
    case MM_WARN_ON_MEMORY_EXCEEDED:
      cerr << " MM_register::set_memory_limit to " << new_limit 
	   << ", used " << used << ". allocation exceeds new limit.\n";
      break;
      
    case MM_IGNORE_MEMORY_EXCEEDED:
      break;
    }   
    user_limit = new_limit;
    remaining = 0;
    return MM_ERROR_NO_ERROR;
  }
  
  assert(used <= new_limit);
  // These are unsigned, so be careful.
  if (new_limit < user_limit) {
    remaining -= user_limit - new_limit;
  } else {
    remaining += new_limit - user_limit;
  }
  user_limit = new_limit;
  return MM_ERROR_NO_ERROR;
}  



/* ************************************************************ */
//only warn if memory limit exceeded
void MM_register::warn_memory_limit() {
  register_new = MM_WARN_ON_MEMORY_EXCEEDED;
}


/* ************************************************************ */
//abort if memory limit exceeded
void MM_register::enforce_memory_limit() {
  register_new = MM_ABORT_ON_MEMORY_EXCEEDED;

  if (used > user_limit) {
    cerr << " MM_register::enforce_memory_limit: limit=" << user_limit 
	 << ", used=" << used << ". allocation exceeds limit.\n";
    assert(0); //core dump if debugging
    exit(1);
  }
}


/* ************************************************************ */
//ignore memory limit accounting
void MM_register::ignore_memory_limit() {
  register_new = MM_IGNORE_MEMORY_EXCEEDED;
}


/* ************************************************************ */
// provide accounting state
MM_mode MM_register::get_limit_mode() {
  return register_new;
}

/* ************************************************************ */
// provide print ccounting state
void MM_register::print_limit_mode() {
  cout << "Memory manager registering memory in ";  
  switch (register_new)  {
  case MM_ABORT_ON_MEMORY_EXCEEDED:
    cout << "MM_ABORT_ON_MEMORY_EXCEEDED";
    break;
  case MM_WARN_ON_MEMORY_EXCEEDED:
    cout << "MM_WARN_ON_MEMORY_EXCEEDED";
    break;
  case  MM_IGNORE_MEMORY_EXCEEDED:
    cout << "MM_IGNORE_MEMORY_EXCEEDED";
    break;
  }
  cout << " mode." << endl;
}



/* ************************************************************ */
//return the amount of memory available before user-specified memory
//limit will be exceeded
size_t MM_register::memory_available() {
  return remaining;    
}

/* ************************************************************ */
size_t MM_register::memory_used() {
  return used;    
}


/* ************************************************************ */
size_t MM_register::memory_limit() {
  return user_limit;    
}


/* ---------------------------------------------------------------------- */
// return the overhead on each memory allocation request 


// SIZE_SPACE is to ensure alignment on quad word boundaries.  It may be
// possible to check whether a machine needs this at configuration
// time or if dword alignment is ok.  On the HP 9000, bus errors occur
// when loading doubles that are not qword aligned.
static const size_t SIZE_SPACE=(sizeof(size_t) > 8 ? sizeof(size_t) : 8);



int   MM_register::space_overhead ()  {
  return SIZE_SPACE;
}
  



/* ************************************************************ */
// check that new allocation request is below user-defined limit.
// This should be a private method, only called by operator new.
MM_err MM_register::register_allocation(size_t request) {

  if (request > remaining) {
    remaining = 0;
    used += request;
    return MM_ERROR_INSUFFICIENT_SPACE;
    
  } else {
    used      += request;     
    remaining -= request;
    return MM_ERROR_NO_ERROR;
  }
}



/* ************************************************************ */
// do the accounting for a memory deallocation request.
// This should be a private method, only called by operators 
// delete and delete [].
MM_err MM_register::register_deallocation(size_t sz) {
  
  if (sz > used) {
    used = 0;
    remaining = user_limit;
    return MM_ERROR_UNDERFLOW;
  } else {

    used      -= sz;        
    if (used < user_limit) {
      remaining = user_limit - used;
    } else {
      assert(remaining == 0);
    }
    return MM_ERROR_NO_ERROR;
  }
}


 
/* ************************************************************ */
/* these overloaded operators must only be used by this memory manager
 * risk of invalid free if these operators are defined outside the MM_register class
 * e.g. GDAL allocating memory with something else than new as defined here
 * but then using delete as defined here 
 */
#ifdef GRASS_MM_USE_EXCEPTION_SPECIFIER
void* MM_register::operator new[] (size_t sz) throw (std::bad_alloc) {
#else
void* MM_register::operator new[] (size_t sz) {
#endif /* GRASS_MM_USE_EXCEPTION_SPECIFIER */
  void *p;

  MM_DEBUG cout << "new: sz=" << sz << ", register " 
		<< sz+SIZE_SPACE << "B ,"; 

  if (MM_manager.register_allocation (sz + SIZE_SPACE) != MM_ERROR_NO_ERROR){
    //must be MM_ERROR_INSUF_SPACE
    switch(MM_manager.register_new) {
      
    case MM_ABORT_ON_MEMORY_EXCEEDED:
      cerr << "MM error: limit ="<< MM_manager.memory_limit() <<"B. " 
	   << "allocating " << sz << "B. " 
	   << "limit exceeded by " 
	   <<  MM_manager.memory_used() -  MM_manager.memory_limit()<<"B."
	   << endl;
      assert (0);		// core dump if debugging
      exit (1);
      break;
      
    case MM_WARN_ON_MEMORY_EXCEEDED:
      cerr << "MM warning: limit="<<MM_manager.memory_limit() <<"B. " 
	   << "allocating " << sz << "B. " 
	   << " limit exceeded by " 
	   <<  MM_manager.memory_used() -  MM_manager.memory_limit()<<"B."
	   << endl;
      break;
      
    case MM_IGNORE_MEMORY_EXCEEDED:
      break;
    }
  }
  
  p = malloc(sz + SIZE_SPACE);
  
  if (!p) {
    cerr << "new: out of memory while allocating " << sz << "B" << endl;
    assert(0);
    exit (1);
  }

  *((size_t *) p) = sz;
  
  MM_DEBUG cout << "ptr=" << (void*) (((char *) p) + SIZE_SPACE) << endl;
  
  return ((char *) p) + SIZE_SPACE;
}


 
/* ************************************************************ */
#ifdef GRASS_MM_USE_EXCEPTION_SPECIFIER
void* MM_register::operator new (size_t sz) throw (std::bad_alloc) {
#else
void* MM_register::operator new (size_t sz) {
#endif /* GRASS_MM_USE_EXCEPTION_SPECIFIER */
  void *p;

  MM_DEBUG cout << "new: sz=" << sz << ", register " 
		<< sz+SIZE_SPACE << "B ,"; 

  if (MM_manager.register_allocation (sz + SIZE_SPACE) != MM_ERROR_NO_ERROR){
    //must be MM_ERROR_INSUF_SPACE
    switch(MM_manager.register_new) {
      
    case MM_ABORT_ON_MEMORY_EXCEEDED:
      cerr << "MM error: limit ="<< MM_manager.memory_limit() <<"B. " 
	   << "allocating " << sz << "B. " 
	   << "limit exceeded by " 
	   <<  MM_manager.memory_used() -  MM_manager.memory_limit()<<"B."
	   << endl;
      assert (0);		// core dump if debugging
      exit (1);
      break;
      
    case MM_WARN_ON_MEMORY_EXCEEDED:
      cerr << "MM warning: limit="<<MM_manager.memory_limit() <<"B. " 
	   << "allocating " << sz << "B. " 
	   << " limit exceeded by " 
	   <<  MM_manager.memory_used() -  MM_manager.memory_limit()<<"B."
	   << endl;
      break;
      
    case MM_IGNORE_MEMORY_EXCEEDED:
      break;
    }
  }
  
  p = malloc(sz + SIZE_SPACE);
  
  if (!p) {
    cerr << "new: out of memory while allocating " << sz << "B" << endl;
    assert(0);
    exit (1);
  }

  *((size_t *) p) = sz;
  
  MM_DEBUG cout << "ptr=" << (void*) (((char *) p) + SIZE_SPACE) << endl;
  
  return ((char *) p) + SIZE_SPACE;
}




/* ---------------------------------------------------------------------- */
#ifdef GRASS_MM_USE_EXCEPTION_SPECIFIER
void MM_register::operator delete (void *ptr) throw() {
#else
void MM_register::operator delete (void *ptr) noexcept {
#endif /* GRASS_MM_USE_EXCEPTION_SPECIFIER */
  size_t sz;
  void *p;

  MM_DEBUG cout << "delete: ptr=" << ptr << ","; 

  if (!ptr) {
    cerr << "MM warning: operator delete was given a NULL pointer\n";
    cerr.flush();
    //this may actually happen: for instance when calling a default
    //destructor for something that was not allocated with new
    //e.g. ofstream str(name) ---- ~ofstream() called ==> ptr=NULL
    
	//who wrote the above comment? -RW
    assert(0); 
    //exit(1);
    return;
  }
  
  assert(ptr);

  /* causes invalid free if ptr has not been allocated with new as 
   * defined above */
  p = ((char *)ptr) - SIZE_SPACE; // the base of memory
  sz = *((size_t *)p);
  
  MM_DEBUG cout << "size=" << sz <<", free " << p << "B and deallocate " 
		<< sz + SIZE_SPACE << endl;
  
  if(MM_manager.register_deallocation (sz + SIZE_SPACE) != MM_ERROR_NO_ERROR){
    //must be MM_ERROR_UNDERFLOW
    cerr << "delete: MM_manager.register_deallocation failed\n";
    assert(0);
    exit(1);
  }

  free(p);
}




/* ---------------------------------------------------------------------- */
#ifdef GRASS_MM_USE_EXCEPTION_SPECIFIER
void MM_register::operator delete[] (void *ptr) throw() {
#else
void MM_register::operator delete[] (void *ptr) noexcept {
#endif /* GRASS_MM_USE_EXCEPTION_SPECIFIER */
  size_t sz;
  void *p;
  
  MM_DEBUG cout << "delete[]: ptr=" << ptr << ","; 

  if (!ptr) {
    //can this happen? -- it does: see delete above
    cerr << "MM warning: operator delete [] was given a NULL pointer\n";
    cerr.flush();
    //assert(0);
    //exit(1);
    return;
  }
   assert(ptr);

  /* causes invalid free if ptr has not been allocated with new as 
   * defined above */
   p = ((char *)ptr) - SIZE_SPACE; // the base of memory
   sz = *((size_t *)p);

   MM_DEBUG cout << "size=" << sz <<", free " << p << "B and deallocate " 
		 << sz + SIZE_SPACE << endl;
   
   if(MM_manager.register_deallocation (sz + SIZE_SPACE)!= MM_ERROR_NO_ERROR){
     //must be MM_ERROR_UNDERFLOW
     cerr << "delete[]: MM_manager.register_deallocation failed\n";
     assert(0);
     exit(1);
   }
   
   free(p);
}





/* ************************************************************ */
// Instantiate the actual memory manager, and allocate the 
// its static data members
MM_register MM_manager;
int MM_register::instances = 0; // Number of instances. (init)
// TPIE's "register memory requests" flag
MM_mode MM_register::register_new = MM_IGNORE_MEMORY_EXCEEDED;
//This causes constructors for static variables to fail
//MM_mode MM_register::register_new = MM_ABORT_ON_MEMORY_EXCEEDED; 






/* ************************************************************ */
// The counter of mm_register_init instances.  It is implicitly set to 0.
unsigned int mm_register_init::count;

// The constructor and destructor that ensure that the memory manager is
// created exactly once, and destroyed when appropriate.
mm_register_init::mm_register_init(void) {
  if (count++ == 0) {
    MM_manager.set_memory_limit(MM_DEFAULT_MM_SIZE);
  }
}

mm_register_init::~mm_register_init(void) {
  --count;
}
