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
%typemap(in,numinputs=0) return_string (char * temp) {
	temp=NULL;
	$1=&temp;
}


%typemap(argout) CELL * {
		int len=0,i;
		len=G_window_cols();
		$result=PyList_New(len);
                for(i=0;i<len;i++)
                {
                  PyList_SetItem($result,i,PyInt_FromLong($1[i]));
                }
}

%typemap(argout) FCELL * {
		int len=0,i;
		len=G_window_cols();
                $result=PyList_New(len);
                for(i=0;i<len;i++)
                {
                  PyList_SetItem($result,i,PyFloat_FromDouble($1[i]) );
                }                
}

%typemap(argout) DCELL * {
		int len=0,i;
		len=G_window_cols();
		$result=PyList_New(len);
                for(i=0;i<len;i++)
                {
                  PyList_SetItem($result,i,PyFloat_FromDouble($1[i]));
                }                
}


%typemap(in)CELL  * {
		int len=0,i=0; CELL *tmp;
		PyObject obj;
		len=G_window_cols();
		$1=G_allocate_cell_buf();
}

%typemap(in)FCELL  * {
		int len=0,i=0; FCELL *tmp;
		PyObject obj;
		len=G_window_cols();
		$1=G_allocate_f_raster_buf();
}

%typemap(in)DCELL  * {
		int len=0,i=0; FCELL *tmp;
		PyObject obj;
		len=G_window_cols();
		$1=G_allocate_d_raster_buf();
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
#endif
