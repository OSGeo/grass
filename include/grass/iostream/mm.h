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

#ifndef _MM_H
#define _MM_H

#include <sys/types.h>

// GCC with C++98 and -fexceptions requires exception
// specifiers, however with C++11 and newer, using them causes an error.
#if __cplusplus < 201103L
#define GRASS_MM_USE_EXCEPTION_SPECIFIER
#endif /* __cplusplus < 201103L */

#define MM_REGISTER_VERSION 2

// The default amount of memory we will allow to be allocated (40MB).
#define MM_DEFAULT_MM_SIZE  (40 << 20)

// MM accounting modes
typedef enum {
    MM_IGNORE_MEMORY_EXCEEDED = 0,
    MM_ABORT_ON_MEMORY_EXCEEDED,
    MM_WARN_ON_MEMORY_EXCEEDED
} MM_mode;

// MM Error codes
enum MM_err {
    MM_ERROR_NO_ERROR = 0,
    MM_ERROR_INSUFFICIENT_SPACE,
    MM_ERROR_UNDERFLOW,
    MM_ERROR_EXCESSIVE_ALLOCATION
};

// types of memory usage queries we can make on streams
enum MM_stream_usage {
    // Overhead of the object without the buffer
    MM_STREAM_USAGE_OVERHEAD = 1,

    // amount used by a buffer
    MM_STREAM_USAGE_BUFFER,

    // Amount currently in use.
    MM_STREAM_USAGE_CURRENT,

    // Maximum amount possibly in use.
    MM_STREAM_USAGE_MAXIMUM
};

#ifdef GRASS_CMAKE_BUILD
#include <export/grass_iostream_export.h>
#else
#define GRASS_IOSTREAM_EXPORT
#endif

// Declarations of a very simple memory manager designed to work with
// BTEs that rely on the underlying OS to manage physical memory.
class GRASS_IOSTREAM_EXPORT MM_register {
private:
    // The number of instances of this class and descendents that exist.
    static int instances;

    // The amount of space remaining to be allocated.
    size_t remaining;

    // The user-specified limit on memory.
    size_t user_limit;

    // the amount that has been allocated.
    size_t used;

    // flag indicates how we are keeping track of memory
    static MM_mode register_new;

    // protected:
    //   // private methods, only called by operators new and delete.

public: //  Need to be accessible from pqueue constructor
    MM_err register_allocation(size_t sz);
    MM_err register_deallocation(size_t sz);

public:
    MM_register();
    ~MM_register(void);

    MM_err set_memory_limit(size_t sz);
    void enforce_memory_limit();
    void ignore_memory_limit();
    void warn_memory_limit();
    MM_mode get_limit_mode();
    void print_limit_mode();

    size_t memory_available();
    size_t memory_used();
    size_t memory_limit();

    int space_overhead();

    void print();

    // make these members of MM_register
#ifdef GRASS_MM_USE_EXCEPTION_SPECIFIER
    void *operator new(size_t) throw(std::bad_alloc);
    void *operator new[](size_t) throw(std::bad_alloc);
    void operator delete(void *) throw();
    void operator delete[](void *) throw();
#else
    void *operator new(size_t);
    void *operator new[](size_t);
    void operator delete(void *) noexcept;
    void operator delete[](void *) noexcept;
#endif /* GRASS_MM_USE_EXCEPTION_SPECIFIER */

    friend class mm_register_init;
};

// A class to make sure that MM_manager gets set up properly (only one
// instance) .
class mm_register_init {
private:
    // The number of mm_register_init objects that exist.
    static unsigned int count;

public:
    mm_register_init(void);
    ~mm_register_init(void);
};

static mm_register_init source_file_mm_register_init;

// Here is the single memory management object (defined in mm.C).
extern GRASS_IOSTREAM_EXPORT MM_register MM_manager;

#endif // _MM_H
