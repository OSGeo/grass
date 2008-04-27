"""
    Copyright 2003,2004 Christian Dieterich
    rca@geosci.uchicago.edu
    Department of the Geophysical Sciences, University of Chicago
    2003

    The Numeric Pointer Module is free software; you can redistribute it 
    and/or modify it under the terms of the GNU General Public License as 
    published by the Free Software Foundation; either version 2 of the License,
    or (at your option) any later version.

    The Numeric Pointer Module is distributed in the hope that it will be 
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Numeric Pointer Module; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
"""

import _NumPtr

def _swig_setattr(self,class_type,name,value):
    if (name == "this"):
        if isinstance(value, class_type):
            self.__dict__[name] = value.this
            if hasattr(value,"thisown"): self.__dict__["thisown"] = value.thisown
            del value.thisown
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    self.__dict__[name] = value

def _swig_getattr(self,class_type,name):
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0

def getpointer(A):
    """
    Returns a SWIG pointer to the data in NumPy array object A.
    Array can be 1, 2 or 3 dimensional and must be of type Float64,
    Float32 or Int32.
    """
    if (len(A.shape) == 0):
        if (A.typecode() == 'd'):
            p = _NumPtr.getdpointer1(A)
        elif (A.typecode() == 'f'):
            p = _NumPtr.getfpointer1(A)
        elif (A.typecode() == 'i'):
            p = _NumPtr.getipointer1(A)
        else:
            raise TypeError, "Your array does not have typecode 'd', 'f' or 'i'."
        return p
    elif (len(A.shape) == 1):
        if (A.typecode() == 'd'):
            p = _NumPtr.getdpointer1(A)
        elif (A.typecode() == 'f'):
            p = _NumPtr.getfpointer1(A)
        elif (A.typecode() == 'i'):
            p = _NumPtr.getipointer1(A)
        else:
            raise TypeError, "Your array does not have typecode 'd', 'f' or 'i'."
        return p
    elif (len(A.shape) == 2):
        if (A.typecode() == 'd'):
            p = _NumPtr.getdpointer2(A)
        elif (A.typecode() == 'f'):
            p = _NumPtr.getfpointer2(A)
        elif (A.typecode() == 'i'):
            p = _NumPtr.getipointer2(A)
        else:
            raise TypeError, "Your array does not have typecode 'd', 'f' or 'i'."
        return p
    elif (len(A.shape) == 3):
        if (A.typecode() == 'd'):
            p = _NumPtr.getdpointer3(A)
        elif (A.typecode() == 'f'):
            p = _NumPtr.getfpointer3(A)
        elif (A.typecode() == 'i'):
            p = _NumPtr.getipointer3(A)
        else:
            raise TypeError, "Your array does not have typecode 'd', 'f' or 'i'."
        return p
    else:
        raise TypeError, "Your array has rank > 3."

def verifypointer(Aptr, n=0, m=None, l=None):
    """
    Prints the data in NumPy array object A, where Aptr points to.
    Array can be 1, 2 or 3 dimensional and must be of type Float64,
    Float32 or Int32.
    """
    if (Aptr.count('_p') == 0):
        if (Aptr.count('_double') == 1):
            p = _NumPtr.testd1(Aptr, n)
        elif (Aptr.count('_float') == 1):
            p = _NumPtr.testf1(Aptr, n)
        elif (Aptr.count('_int') == 1):
            p = _NumPtr.testi1(Aptr, n)
        else:
            raise TypeError, "Your array does not have typecode 'd', 'f' or 'i'."
        return p
    elif (Aptr.count('_p') == 1):
        if (Aptr.count('_double') == 1):
            p = _NumPtr.testd1(Aptr, n)
        elif (Aptr.count('_float') == 1):
            p = _NumPtr.testf1(Aptr, n)
        elif (Aptr.count('_int') == 1):
            p = _NumPtr.testi1(Aptr, n)
        else:
            raise TypeError, "Your array does not have typecode 'd', 'f' or 'i'."
        return p
    elif (Aptr.count('_p') == 2):
        if m == None: m = 0
        if (Aptr.count('_double') == 1):
            p = _NumPtr.testd2(Aptr, n, m)
        elif (Aptr.count('_float') == 1):
            p = _NumPtr.testf2(Aptr, n, m)
        elif (Aptr.count('_int') == 1):
            p = _NumPtr.testi2(Aptr, n, m)
        else:
            raise TypeError, "Your array does not have typecode 'd', 'f' or 'i'."
        return p
    elif (Aptr.count('_p') == 3):
        if m == None: m = 0
        if l == None: l = 0
        if (Aptr.count('_double') == 1):
            p = _NumPtr.testd3(Aptr, n, m, l)
        elif (Aptr.count('_float') == 1):
            p = _NumPtr.testf3(Aptr, n, m, l)
        elif (Aptr.count('_int') == 1):
            p = _NumPtr.testi3(Aptr, n, m, l)
        else:
            raise TypeError, "Your array does not have typecode 'd', 'f' or 'i'."
        return p
    else:
        raise TypeError, "Your array has rank greater than 3."

def getpointer1(A):
    return getpointer(A)

def getpointer2(A):
    return getpointer(A)

def getpointer3(A):
    return getpointer(A)
