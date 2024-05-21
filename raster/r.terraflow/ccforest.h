/****************************************************************************
 *
 *  MODULE:        r.terraflow
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

#ifndef CCFOREST_H
#define CCFOREST_H

#include <assert.h>
#include <iostream>

#include <grass/iostream/ami.h>

#define DEBUG_CCFOREST if (0)

template <class T>
class keyvalue {
private:
    T key, value;

public:
    keyvalue() : key(-1), value(-1){};
    keyvalue(T vk, T vv) : key(vk), value(vv){};

    T getPriority() const { return key; };
    T getValue() const { return value; };
    T src() const { return key; };
    T dst() const { return value; };

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    keyvalue operator=(const keyvalue &that)
    {
        key = that.key;
        value = that.value;
        return *this;
    };
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 021dfb5d52 (r.terrafow: explicit use of default constructors (#2660))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    int operator!=(const keyvalue &e2) const
    {
        return (key != e2.key) || (value != e2.value);
    }
    int operator==(const keyvalue &e2) const
    {
        return (value == e2.value) && (key == e2.key);
    }
    int operator>(const keyvalue &e2) const
    {
        return (key > e2.key) || (key == e2.key && value > e2.value);
    }
    int operator>=(const keyvalue &e2) const
    {
        return (key > e2.key) || (key == e2.key && value >= e2.value);
    }
    int operator<(const keyvalue &e2) const
    {
        return (key < e2.key) || (key == e2.key && value < e2.value);
    }
    int operator<=(const keyvalue &e2) const
    {
        return (key <= e2.key) || (key == e2.key && value <= e2.value);
    }

    friend ostream &operator<<(ostream &s, const keyvalue &p)
    {
        return s << "(" << p.key << "," << p.value << ")";
    }

    static int qscompare(const void *a, const void *b)
    {
        keyvalue<T> *x = (keyvalue<T> *)a;
        keyvalue<T> *y = (keyvalue<T> *)b;
        return compare(*x, *y);
    }

    static int compare(const keyvalue<T> &x, const keyvalue<T> &y)
    {
        return (x < y ? -1 : (x > y ? 1 : 0));
    }
};

/* laura: used in sort (instead of <); checkit  */
template <class T>
class keyCmpKeyvalueType {
public:
    static int compare(const keyvalue<T> &a, const keyvalue<T> &b)
    {
        if (a.getPriority() < b.getPriority())
            return -1;
        if (a.getPriority() > b.getPriority())
            return 1;
        return 0;
    }
};

/* ---------------------------------------------------------------------- */
/* laura: used in sort instead of valueCmp; checkit */
template <class T>
class dstCmpKeyvalueType {
public:
    static int compare(const keyvalue<T> &a, const keyvalue<T> &b)
    {
        if (a.dst() < b.dst())
            return -1;
        if (a.dst() > b.dst())
            return 1;

        if (a.src() < b.src())
            return -1;
        if (a.src() > b.src())
            return 1;

        return 0;
    }
};

template <class T>
class ccforest;

template <class T>
class ccforest {
    /* class cckeyvalue : public keyvalue<T> {}; */
    typedef keyvalue<T> ccedge;
    typedef keyvalue<T> cckeyvalue;

private:
    AMI_STREAM<ccedge> *edgeStream;
    AMI_STREAM<cckeyvalue> *rootStream;

    void findAllRoots(int depth = 0);
    int rootCycles;
    ccforest<T> *superTree;
    void removeDuplicates(T src, T parent, EMPQueueAdaptive<cckeyvalue, T> &pq);
    int foundAllRoots;
    cckeyvalue savedRoot;
    int savedRootValid;

public:
    ccforest();
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 021dfb5d52 (r.terrafow: explicit use of default constructors (#2660))
    ccforest(const ccforest &) = delete;
    ccforest &operator=(const ccforest &) = delete;
    ccforest(ccforest &&) = delete;
    ccforest &operator=(ccforest &&) = delete;
    ~ccforest();

<<<<<<< HEAD
=======
    ~ccforest();
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    ~ccforest();
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 021dfb5d52 (r.terrafow: explicit use of default constructors (#2660))
=======
    ~ccforest();
=======
    ccforest(const ccforest &) = delete;
    ccforest &operator=(const ccforest &) = delete;
    ccforest(ccforest &&) = delete;
    ccforest &operator=(ccforest &&) = delete;
    ~ccforest();

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    void insert(const T &i, const T &j); /* insert edge (i,j) */
    T findNextRoot(const T &i);          /* find root where i >= prev i */
    void printRootStream();
    void printEdgeStream();
    off_t size();
};

#endif /* CCFOREST_H */
