from Numeric import *
from NumPtr import *
from _NumPtr import *

a = array([1,2,3,4,5,6],Float64)
print a
p=getpointer(a)
print p
test1(p,6)

a = array([[1,2,3],[4,5,6]],Float64)
print a
p=getpointer(a)
print p
test2(p,2,3)

a = array([ [[1,2,3],[4,5,6]] , [[7,8,9],[10,11,12]] ],Float64)
print a
print len(a.shape)
p=getpointer(a)
print p
test3(p,2,2,3)

a = array([1,2,3,4,5,6],Float64)
print a
p=getpointer(a)
print p
testd1(p,6)

a = array([[1,2,3],[4,5,6]],Float64)
print a
p=getpointer(a)
print p
testd2(p,2,3)

a = array([ [[1,2,3],[4,5,6]] , [[7,8,9],[10,11,12]] ],Float64)
print a
print len(a.shape)
p=getpointer(a)
print p
testd3(p,2,2,3)

a = array([1,2,3,4,5,6],Float32)
print a
p=getpointer(a)
print p
testf1(p,6)

a = array([[1,2,3],[4,5,6]],Float32)
print a
p=getpointer(a)
print p
testf2(p,2,3)

a = array([ [[1,2,3],[4,5,6]] , [[7,8,9],[10,11,12]] ],Float32)
print a
print len(a.shape)
p=getpointer(a)
print p
testf3(p,2,2,3)

a = array([1,2,3,4,5,6],Int32)
print a
p=getpointer(a)
print p
testi1(p,6)

a = array([[1,2,3],[4,5,6]],Int32)
print a
p=getpointer(a)
print p
testi2(p,2,3)

a = array([ [[1,2,3],[4,5,6]] , [[7,8,9],[10,11,12]] ],Int32)
print a
print len(a.shape)
p=getpointer(a)
print p
testi3(p,2,2,3)
