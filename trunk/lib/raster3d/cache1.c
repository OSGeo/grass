#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/raster3d.h>
#include "raster3d_intern.h"
#include "cachehash.h"

/*---------------------------------------------------------------------------*/

#define IS_ACTIVE_ELT(elt) (c->locks[elt] != 2)
#define IS_NOT_ACTIVE_ELT(elt) (c->locks[elt] == 2)
#define IS_LOCKED_ELT(elt) (c->locks[elt] == 1)
#define IS_UNLOCKED_ELT(elt) (c->locks[elt] == 0)
#define IS_NOT_IN_QUEUE_ELT(elt) (IS_LOCKED_ELT (elt))
#define IS_IN_QUEUE_ELT(elt) (! IS_NOT_IN_QUEUE_ELT (elt))

#define DEACTIVATE_ELT(elt) ((IS_LOCKED_ELT(elt) ? \
			      (c->nofUnlocked)++ : (0)), \
			     c->locks[elt] = 2)
#define LOCK_ELT(elt) ((IS_LOCKED_ELT(elt) ? \
			(0) : (c->nofUnlocked)--), \
		       (c->locks[elt] = 1))
#define UNLOCK_ELT(elt) ((IS_LOCKED_ELT(elt) ? \
			  (c->nofUnlocked)++ : (0)), \
                         (c->locks[elt] = 0))

#define ONE_UNLOCKED_ELT_ONLY (c->first == c->last)
#define ARE_MIN_UNLOCKED (c->nofUnlocked <= c->minUnlocked)

/*---------------------------------------------------------------------------*/

void Rast3d_cache_reset(RASTER3D_cache * c)
{
    int i;

    for (i = 0; i < c->nofElts; i++) {
	DEACTIVATE_ELT(i);
	c->next[i] = i + 1;
	c->prev[i] = i - 1;
	c->names[i] = -1;
    }

    c->prev[0] = c->next[c->nofElts - 1] = -1;
    c->first = 0;
    c->last = c->nofElts - 1;

    c->autoLock = 0;
    c->nofUnlocked = c->nofElts;
    c->minUnlocked = 1;

    Rast3d_cache_hash_reset(c->hash);
}

/*---------------------------------------------------------------------------*/

static int cache_dummy_fun(int tileIndex, const void *tileBuf, void *map)
{
    return 1;
}

/*---------------------------------------------------------------------------*/

void Rast3d_cache_dispose(RASTER3D_cache * c)
{
    if (c == NULL)
	return;

    Rast3d_cache_hash_dispose(c->hash);

    if (c->elts != NULL)
	Rast3d_free(c->elts);
    if (c->names != NULL)
	Rast3d_free(c->names);
    if (c->locks != NULL)
	Rast3d_free(c->locks);
    if (c->next != NULL)
	Rast3d_free(c->next);
    if (c->prev != NULL)
	Rast3d_free(c->prev);

    Rast3d_free(c);
}

/*---------------------------------------------------------------------------*/

void *Rast3d_cache_new(int nofElts, int sizeOfElts, int nofNames,
		    int (*eltRemoveFun) (), void *eltRemoveFunData,
		    int (*eltLoadFun) (), void *eltLoadFunData)
{
    RASTER3D_cache *tmp;
    int i;

    tmp = Rast3d_malloc(sizeof(RASTER3D_cache));
    if (tmp == NULL) {
	Rast3d_error("Rast3d_cache_new: error in Rast3d_malloc");
	return (void *)NULL;
    }

    tmp->hash = NULL;

    tmp->nofElts = nofElts;
    tmp->eltSize = sizeOfElts;
    tmp->elts = Rast3d_malloc(tmp->eltSize * tmp->nofElts);
    tmp->names = Rast3d_malloc(sizeof(int) * tmp->nofElts);
    tmp->locks = Rast3d_malloc(tmp->nofElts);
    tmp->next = Rast3d_malloc(sizeof(int) * tmp->nofElts);
    tmp->prev = Rast3d_malloc(sizeof(int) * tmp->nofElts);

    if ((tmp->elts == NULL) || (tmp->names == NULL) || (tmp->locks == NULL) ||
	(tmp->next == NULL) || (tmp->prev == NULL)) {

	Rast3d_cache_dispose(tmp);
	Rast3d_error("Rast3d_cache_new: error in Rast3d_malloc");
	return (void *)NULL;
    }
    /* Init the cache lock */
    for(i = 0; i < tmp->nofElts; i++)
        tmp->locks[i] = 0;
        
    tmp->eltRemoveFun = eltRemoveFun;
    tmp->eltRemoveFunData = eltRemoveFunData;
    tmp->eltLoadFun = eltLoadFun;
    tmp->eltLoadFunData = eltLoadFunData;

    tmp->hash = Rast3d_cache_hash_new(nofNames);
    if (tmp->hash == NULL) {
	Rast3d_cache_dispose(tmp);
	Rast3d_error("Rast3d_cache_new: error in Rast3d_cache_hash_new");
	return (void *)NULL;
    }

    Rast3d_cache_reset(tmp);

    return tmp;
}

/*---------------------------------------------------------------------------*/

void
Rast3d_cache_set_remove_fun(RASTER3D_cache * c, int (*eltRemoveFun) (),
			void *eltRemoveFunData)
{
    c->eltRemoveFun = eltRemoveFun;
    c->eltRemoveFunData = eltRemoveFunData;
}

/*---------------------------------------------------------------------------*/

void
Rast3d_cache_set_load_fun(RASTER3D_cache * c, int (*eltLoadFun) (),
		      void *eltLoadFunData)
{
    c->eltLoadFun = eltLoadFun;
    c->eltLoadFunData = eltLoadFunData;
}

/*---------------------------------------------------------------------------*/

void *Rast3d_cache_new_read(int nofElts, int sizeOfElts, int nofNames,
			 read_fn * eltLoadFun, void *eltLoadFunData)
{
    return Rast3d_cache_new(nofElts, sizeOfElts, nofNames,
			    cache_dummy_fun, NULL, eltLoadFun, eltLoadFunData);
}

/*---------------------------------------------------------------------------*/

static void cache_queue_dequeue(RASTER3D_cache * c, int index)
{
    if (IS_NOT_IN_QUEUE_ELT(index))
	Rast3d_fatal_error("cache_queue_dequeue: index not in queue");

    if (index == c->first)
	c->first = c->next[index];
    if (index == c->last)
	c->last = c->prev[index];

    if (c->next[index] != -1)
	c->prev[c->next[index]] = c->prev[index];
    if (c->prev[index] != -1)
	c->next[c->prev[index]] = c->next[index];

    c->next[index] = c->prev[index] = -1;
}

/*---------------------------------------------------------------------------*/

static void cache_queue_enqueue(RASTER3D_cache * c, int left, int index)
{
    if (IS_IN_QUEUE_ELT(index))
	Rast3d_fatal_error("cache_queue_enqueue: index already in queue");

    if (c->first == -1) {
	if (left != c->last)
	    Rast3d_fatal_error("cache_queue_enqueue: position out of range");

	c->first = c->last = index;
	return;
    }

    if (left >= 0 && IS_NOT_IN_QUEUE_ELT(left))
	Rast3d_fatal_error("cache_queue_enqueue: position not in queue");


    if (left == -1) {
	c->next[index] = c->first;
	c->prev[c->first] = index;
	c->first = index;

	return;
    }

    c->prev[index] = left;

    if (c->next[left] == -1) {
	c->next[left] = index;
	c->last = index;

	return;
    }

    c->prev[c->next[left]] = index;
    c->next[index] = c->next[left];
    c->next[left] = index;
}

/*---------------------------------------------------------------------------*/

static int cache_queue_get_top(RASTER3D_cache * c)
{
    int top;

    top = c->first;

    cache_queue_dequeue(c, c->first);

    return top;
}

/*---------------------------------------------------------------------------*/

static void cache_queue_append(RASTER3D_cache * c, int index)
{
    cache_queue_enqueue(c, c->last, index);
}

/*---------------------------------------------------------------------------*/

static void cache_queue_preppend(RASTER3D_cache * c, int index)
{
    cache_queue_enqueue(c, -1, index);
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

			/* EXPORTED FUNCTIONS */

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

int Rast3d_cache_lock(RASTER3D_cache * c, int name)
{
    int index;

    index = Rast3d_cache_hash_name2index(c->hash, name);
    if (index == -1) {
	Rast3d_error("Rast3d_cache_lock: name not in cache");
	return 0;
    }

    if (IS_LOCKED_ELT(index))
	return 1;
    if (ONE_UNLOCKED_ELT_ONLY)
	return -1;
    if (ARE_MIN_UNLOCKED)
	return -1;

    cache_queue_dequeue(c, index);
    LOCK_ELT(index);

    return 1;
}

/*---------------------------------------------------------------------------*/

void Rast3d_cache_lock_intern(RASTER3D_cache * c, int index)
{
    if (IS_LOCKED_ELT(index))
	return;

    cache_queue_dequeue(c, index);
    LOCK_ELT(index);
}

/*---------------------------------------------------------------------------*/

int Rast3d_cache_unlock(RASTER3D_cache * c, int name)
{
    int index;

    index = Rast3d_cache_hash_name2index(c->hash, name);
    if (index == -1) {
	Rast3d_error("Rast3d_cache_unlock: name not in cache");
	return 0;
    }

    if (IS_UNLOCKED_ELT(index))
	return 1;

    cache_queue_append(c, index);
    UNLOCK_ELT(index);

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_cache_unlock_all(RASTER3D_cache * c)
{
    int index;

    for (index = 0; index < c->nofElts; index++)
	if (IS_LOCKED_ELT(index))
	    if (!Rast3d_cache_unlock(c, c->names[index])) {
		Rast3d_error("Rast3d_cache_unlock_all: error in Rast3d_cache_unlock");
		return 0;
	    }

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_cache_lock_all(RASTER3D_cache * c)
{
    int index;

    for (index = 0; index < c->nofElts; index++)
	if (IS_UNLOCKED_ELT(index))
	    Rast3d_cache_lock_intern(c, index);

    return 1;
}

/*---------------------------------------------------------------------------*/

void Rast3d_cache_autolock_on(RASTER3D_cache * c)
{
    c->autoLock = 1;
}

/*---------------------------------------------------------------------------*/

void Rast3d_cache_autolock_off(RASTER3D_cache * c)
{
    c->autoLock = 0;
}

/*---------------------------------------------------------------------------*/

void Rast3d_cache_set_min_unlock(RASTER3D_cache * c, int nofMinUnLocked)
{
    c->minUnlocked = nofMinUnLocked;
}

/*---------------------------------------------------------------------------*/

static int cache_remove_elt(RASTER3D_cache * c, int name, int doFlush)
{
    int index;

    index = Rast3d_cache_hash_name2index(c->hash, name);
    if (index == -1) {
	Rast3d_error("Rast3d_cache_deactivate_elt : name not in cache");
	return 0;
    }

    if (IS_NOT_ACTIVE_ELT(index))
	return 1;

    if (IS_IN_QUEUE_ELT(index)) {
	cache_queue_dequeue(c, index);
	LOCK_ELT(index);
    }

    if (doFlush)
	if (!c->eltRemoveFun(name, c->elts + c->eltSize * index,
			     c->eltRemoveFunData)) {
	    Rast3d_error("cache_remove_elt: error in c->eltRemoveFun");
	    return 0;
	}

    cache_queue_preppend(c, index);
    DEACTIVATE_ELT(index);

    Rast3d_cache_hash_remove_name(c->hash, name);

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_cache_remove_elt(RASTER3D_cache * c, int name)
{
    if (!cache_remove_elt(c, name, 0)) {
	Rast3d_error("Rast3d_cache_remove_elt: error in cache_remove_elt");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_cache_flush(RASTER3D_cache * c, int name)
{
    if (!cache_remove_elt(c, name, 1)) {
	Rast3d_error("Rast3d_cache_flush: error in cache_remove_elt");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_cache_remove_all(RASTER3D_cache * c)
{
    int index;

    for (index = 0; index < c->nofElts; index++)
	if (IS_ACTIVE_ELT(index))
	    if (!Rast3d_cache_remove_elt(c, c->names[index])) {
		Rast3d_error
		    ("Rast3d_cache_remove_all: error in Rast3d_cache_remove_elt");
		return 0;
	    }

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_cache_flush_all(RASTER3D_cache * c)
{
    int index;

    for (index = 0; index < c->nofElts; index++)
	if (IS_ACTIVE_ELT(index))
	    if (!Rast3d_cache_flush(c, c->names[index])) {
		Rast3d_error("Rast3d_cache_flush_all: error in Rast3d_cache_flush");
		return 0;
	    }

    return 1;
}

/*---------------------------------------------------------------------------*/

void *Rast3d_cache_elt_ptr(RASTER3D_cache * c, int name)
{
    int index, oldName, doUnlock;

    index = Rast3d_cache_hash_name2index(c->hash, name);

    if (index != -1) {
	if (c->autoLock)
	    if (IS_UNLOCKED_ELT(index) && (!ONE_UNLOCKED_ELT_ONLY) &&
		(!ARE_MIN_UNLOCKED))
		Rast3d_cache_lock_intern(c, index);

	return c->elts + c->eltSize * index;
    }

    index = c->first;
    if (IS_ACTIVE_ELT(index)) {
	oldName = c->names[index];
	Rast3d_cache_hash_remove_name(c->hash, oldName);
	if (!c->eltRemoveFun(oldName, c->elts + c->eltSize * index,
			     c->eltRemoveFunData)) {
	    Rast3d_error("Rast3d_cache_elt_ptr: error in c->eltRemoveFun");
	    return NULL;
	}
    }

    Rast3d_cache_hash_load_name(c->hash, name, index);

    doUnlock = ((!c->autoLock) || ONE_UNLOCKED_ELT_ONLY || ARE_MIN_UNLOCKED);

    UNLOCK_ELT(index);
    c->names[index] = name;
    Rast3d_cache_lock_intern(c, index);

    if (doUnlock)
	if (!Rast3d_cache_unlock(c, name)) {
	    Rast3d_error("Rast3d_cache_elt_ptr: error in Rast3d_cache_unlock");
	    return NULL;
	}

    if (!c->eltLoadFun(name, c->elts + c->eltSize * index, c->eltLoadFunData)) {
	Rast3d_error("Rast3d_cache_elt_ptr: error in c->eltLoadFun");
	return NULL;
    }

    return c->elts + c->eltSize * index;
}

/*---------------------------------------------------------------------------*/

int Rast3d_cache_load(RASTER3D_cache * c, int name)
{
    if (Rast3d_cache_elt_ptr(c, name) == NULL) {
	Rast3d_error("Rast3d_cache_load: error in Rast3d_cache_elt_ptr");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_cache_get_elt(RASTER3D_cache * c, int name, void *dst)
{
    const void *elt;

    elt = Rast3d_cache_elt_ptr(c, name);
    if (elt == NULL) {
	Rast3d_error("Rast3d_cache_get_elt: error in Rast3d_cache_elt_ptr");
	return 0;
    }

    memcpy(dst, elt, c->eltSize);

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_cache_put_elt(RASTER3D_cache * c, int name, const void *src)
{
    void *elt;

    elt = Rast3d_cache_elt_ptr(c, name);
    if (elt == NULL) {
	Rast3d_error("Rast3d_cache_put_elt: error in Rast3d_cache_elt_ptr");
	return 0;
    }

    memcpy(elt, src, c->eltSize);

    return 1;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

			/* TEST FUNCTIONS */

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

static void cache_test_print(RASTER3D_cache * c)
{
    int i, al;
    int *a;

    al = c->autoLock;
    Rast3d_cache_autolock_off(c);

    printf("\n--------------------------------\n");
    for (i = 0; i < c->nofElts; i++) {
	printf("elt %d: ", i);
	if (IS_NOT_ACTIVE_ELT(i)) {
	    printf("na\n");
	    continue;
	}

	a = (int *)Rast3d_cache_elt_ptr(c, c->names[i]);
	/*Rast3d_cache_get_elt (c, c->names[i], a); */
	printf("name %d val %d %s\n", c->names[i], a[17],
	       (IS_LOCKED_ELT(i) ? "locked" :
		IS_UNLOCKED_ELT(i) ? "unlocked" : ""));
    }
    printf("\n--------------------------------\n");

    if (al)
	Rast3d_cache_autolock_on(c);
}

/*---------------------------------------------------------------------------*/

static int cache_test_flush_fun(int name, const void *eltPtr, void *data)
{
    printf("flushing name %d value %d\n", name, ((const int *)eltPtr)[17]);
    return 0;
}

/*---------------------------------------------------------------------------*/

typedef struct
{

    int *value;
    int size;

} cache_test_data_type;

static int cache_test_load_fun(int name, void *eltPtr, void *data)
{
    const void *src;

    printf("loading name %d value %d\n", name,
	   ((cache_test_data_type *) data)->value[17]);

    src = ((cache_test_data_type *) data)->value;
    memcpy(eltPtr, src, ((cache_test_data_type *) data)->size);

    return 0;
}

/*---------------------------------------------------------------------------*/

static cache_test_data_type ctd;

static void cache_test_add(void *c, int name, int val)
{
    static int firstTime = 1;

    if (firstTime) {
	ctd.value = Rast3d_malloc(((RASTER3D_cache *) c)->eltSize * sizeof(int));
	firstTime = 0;
    }

    ctd.value[17] = val;
    ctd.size = ((RASTER3D_cache *) c)->eltSize;

    Rast3d_cache_load(c, name);
}

/*---------------------------------------------------------------------------*/

int MAIN()
{
    void *c;

    c = Rast3d_cache_new(3, 76 * sizeof(int), 100000,
		      cache_test_flush_fun, NULL, cache_test_load_fun, &ctd);

    Rast3d_cache_autolock_on(c);
    cache_test_print(c);
    cache_test_add(c, 1111, -11);
    cache_test_print(c);
    cache_test_add(c, 2222, -22);
    cache_test_print(c);
    cache_test_add(c, 3333, -33);
    cache_test_print(c);
    cache_test_add(c, 4444, -44);
    cache_test_print(c);
    Rast3d_cache_unlock_all(c);
    cache_test_print(c);
    Rast3d_cache_load(c, 2222);
    cache_test_print(c);
    cache_test_add(c, 5555, -55);
    cache_test_print(c);
    cache_test_add(c, 6666, -66);
    cache_test_print(c);
    cache_test_add(c, 7777, -77);
    cache_test_print(c);
    cache_test_add(c, 8888, -88);
    cache_test_print(c);
    cache_test_add(c, 9999, -99);
    cache_test_print(c);
    Rast3d_cache_flush(c, 9999);
    cache_test_print(c);
    Rast3d_cache_flush_all(c);
    cache_test_print(c);
    cache_test_add(c, 1111, -11);
    cache_test_print(c);
    cache_test_add(c, 2222, -22);
    cache_test_print(c);
    cache_test_add(c, 3333, -33);
    cache_test_print(c);
    Rast3d_cache_reset(c);
    cache_test_print(c);
    cache_test_add(c, 1111, -11);
    cache_test_print(c);
    cache_test_add(c, 2222, -22);
    cache_test_print(c);
    cache_test_add(c, 3333, -33);
    cache_test_print(c);

    return 0;
}
