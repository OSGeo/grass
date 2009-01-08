#ifdef SWIGPYTHON

%typemap(in) string_allows_none {
	if($input==Py_None) {
		$1=NULL;
	} else {
		$1=PyString_AsString($input);
		if(!$1) {
			return NULL;
		}
	}
}

%inline %{
typedef char * string_allows_none;
%}

%typemap(out) char ** {
	int len=0,i;
	PyObject * stringobject;

	if(!$1) {
		$result=PyList_New(0);
	} else {

		for(len=0;$1[len];len++);

		$result=PyList_New(len);
		if(!$result){
			// G_free_list($1);
			 return NULL;
		}
			
		for(i=0;i<len;i++) {
			stringobject=PyString_FromString($1[i]);
			if(!stringobject) return NULL;
			PyList_SetItem($result,i,stringobject);
		}
//		G_free_list($1);
	}
}

%typemap(argout) return_string (char * temp) {
	if($1 && *$1) {
		$result=t_output_helper($result,PyString_FromString(*$1));
		G_free(*$1);
	} else {
		$result=t_output_helper($result,Py_None);
		Py_INCREF(Py_None);
	}
	
}

%inline %{
typedef char ** return_string;
%}

#undef __attribute__
#define __attribute__(x)

%{

static void *pyseq_to_ptr(PyObject *input, int data_type) __attribute__ ((unused));
static void *pyobj_to_ptr(PyObject *input, int data_type) __attribute__ ((unused));

static void *pyseq_to_ptr(PyObject *input, int data_type)
{
	size_t size;
	Py_ssize_t len;
	void *array;
	int i;

	if (!PySequence_Check(input)) {
		PyErr_SetString(PyExc_ValueError,"Expected a CObject, buffer or sequence");
		return NULL;
	}

	switch (data_type) {
	case (int) 'c':	size = sizeof(char		);	break;
	case (int) 'b':	size = sizeof(signed char 	);	break;
	case (int) 'B':	size = sizeof(unsigned char 	);	break;
	case (int) 'u':	size = sizeof(Py_UNICODE 	);	break;
	case (int) 'h':	size = sizeof(signed short 	);	break;
	case (int) 'H':	size = sizeof(unsigned short 	);	break;
	case (int) 'i':	size = sizeof(signed int 	);	break;
	case (int) 'I':	size = sizeof(unsigned int 	);	break;
	case (int) 'l':	size = sizeof(signed long 	);	break;
	case (int) 'L':	size = sizeof(unsigned long 	);	break;
	case (int) 'f':	size = sizeof(float		);	break;
	case (int) 'd':	size = sizeof(double		);	break;
	default:
		PyErr_SetString(PyExc_ValueError,"Invalid type code; must be one of [cbBuhHiIlLfd]");
		return NULL;
	}

	len = PySequence_Length(input);
	array = malloc(len * size);

	fprintf(stderr, "sequence length is %d\n", len);

	for (i = 0; i < len; i++) {
		PyObject *val = PySequence_GetItem(input, i);
		DCELL n;

		if (!PyNumber_Check(val)) {
			PyErr_SetString(PyExc_ValueError,"Sequence elements must be numbers");
			Py_XDECREF(val);
			return NULL;
		}

		n = PyFloat_AsDouble(val);
		if (PyErr_Occurred()) {
			PyErr_SetString(PyExc_ValueError,"Sequence elements must be numbers");
			Py_XDECREF(val);
			return NULL;
		}

		Py_DECREF(val);

		switch (data_type) {
		case (int) 'c': ((char		*) array)[i] = (char		) n;	break;
		case (int) 'b': ((signed char	*) array)[i] = (signed char	) n;	break;
		case (int) 'B': ((unsigned char	*) array)[i] = (unsigned char	) n;	break;
		case (int) 'u': ((Py_UNICODE	*) array)[i] = (Py_UNICODE	) n;	break;
		case (int) 'h': ((signed short	*) array)[i] = (signed short	) n;	break;
		case (int) 'H': ((unsigned short*) array)[i] = (unsigned short	) n;	break;
		case (int) 'i': ((signed int	*) array)[i] = (signed int	) n;	break;
		case (int) 'I': ((unsigned int	*) array)[i] = (unsigned int	) n;	break;
		case (int) 'l': ((signed long	*) array)[i] = (signed long	) n;	break;
		case (int) 'L': ((unsigned long	*) array)[i] = (unsigned long	) n;	break;
		case (int) 'f': ((float		*) array)[i] = (float		) n;	break;
		case (int) 'd': ((double	*) array)[i] = (double		) n;	break;
		default:
		    PyErr_SetString(PyExc_ValueError,"Invalid type code; must be one of [cbBuhHiIlLfd]");
		    return NULL;
		}
	}

	return array;
}

static void *pyobj_to_ptr(PyObject *input, int data_type)
{
	const void *cbuffer;
	void *buffer;
	Py_ssize_t len;

	if (PyCObject_Check(input))
		return PyCObject_AsVoidPtr(input);

	if (PyObject_AsWriteBuffer(input, &buffer, &len) == 0)
		return buffer;

	if (PyObject_AsReadBuffer(input, &cbuffer, &len) == 0)
		return (void *) cbuffer;

	return pyseq_to_ptr(input, data_type);
}

%}

%typemap(in) CELL * {
	$1 = (CELL *) pyobj_to_ptr($input, 'i');
}

%typemap(in) FCELL * {
	$1 = (FCELL *) pyobj_to_ptr($input, 'f');
}

%typemap(in) DCELL * {
	$1 = (DCELL *) pyobj_to_ptr($input, 'd');
}

#endif
