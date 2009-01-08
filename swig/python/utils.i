
%include "common.i"

%include "carrays.i"
%array_functions(int, intArray);
%array_functions(float, floatArray);
%array_functions(double, doubleArray);

%include "cpointer.i"
%pointer_functions(int, intp);
%pointer_functions(float, floatp);
%pointer_functions(double, doublep);

PyObject *ptr_to_cobj(void *p);
void *cobj_to_ptr(PyObject *o);
PyObject *ptr_to_buffer_const(const void *p, int size);
PyObject *ptr_to_buffer(void *p, int size);
const void *buffer_to_ptr_const(PyObject *o);
void *buffer_to_ptr(PyObject *o);

%{

static PyObject *ptr_to_cobj(void *p)
{
	return PyCObject_FromVoidPtr(p, NULL);
}

static void *cobj_to_ptr(PyObject *o)
{
	if (PyCObject_Check(o))
		return PyCObject_AsVoidPtr(o);

	PyErr_SetString(PyExc_ValueError,"CObject Expected");
	return NULL;
}

static PyObject *ptr_to_buffer_const(const void *p, int size)
{
	return PyBuffer_FromMemory((void *) p, size);
}

static PyObject *ptr_to_buffer(void *p, int size)
{
	return PyBuffer_FromReadWriteMemory(p, size);
}

static const void *buffer_to_ptr_const(PyObject *o)
{
	const void *p;
	Py_ssize_t len;

	if (PyObject_AsReadBuffer(o, &p, &len) == 0)
		return p;

	PyErr_SetString(PyExc_ValueError,"buffer object expected");
	return NULL;
}

static void *buffer_to_ptr(PyObject *o)
{
	void *p;
	Py_ssize_t len;
	
	if (PyObject_AsWriteBuffer(o, &p, &len) == 0)
		return p;

	PyErr_SetString(PyExc_ValueError,"buffer object expected");
	return NULL;
}

%}

