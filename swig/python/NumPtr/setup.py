import os, glob

# Gather up all the files we need.

files = ["src/getpointer.c","src/test.c"]
files += glob.glob("src/NumPtr.i") 

## pfiles = glob.glob("*.py")
## i = 0
## while i < len(pfiles):
##     pfiles[i] = os.path.splitext(pfiles[i])[0]
##     i += 1

## Distutils Script
from distutils.core import setup, Extension

# Some useful directories.  
from distutils.sysconfig import get_python_inc, get_python_lib

python_incdir = os.path.join( get_python_inc(plat_specific=1) )
python_libdir = os.path.join( get_python_lib(plat_specific=1) )

setup(name="NumPtr",
      version="1.1",
      description="Numeric Pointer module",
      author="Rodrigo Caballero",
      author_email="rca@geosci.uchicago.edu",
      maintainer="Mike Steder",
      maintainer_email="steder@gmail.com",
      url="http://geosci.uchicago.edu/csc/numptr/",
      ext_modules = [Extension('_NumPtr',
                               files,
                               include_dirs=[python_incdir],
                               library_dirs=[python_libdir],
                               ),
                     ],
      # Install these to their own directory
      #   *I want to be able to remove them if I screw up this script
      #   *"Sandboxing", if you will
      extra_path = 'NumPtr',
      package_dir={'':'lib'},
      py_modules=["NumPtr","test"],
      license="GNU GPL",
      
     )
