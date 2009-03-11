from distutils.core import setup, Extension
import os

setup(
	ext_modules= [
		Extension(
			'_grass6_wxvdigit',
			sources=[
				"python_grass6.i",
				],
			swig_opts=['-c++', '-shadow', "-I%s" % os.environ.get("GRASS_INC"), "-I%s\\include" % os.environ.get("OSGEO4W_ROOT") ],
			libraries=['grass_gmath', 'grass_imagery','grass_gis','grass_vedit','gdal_i']
		)
	]
)
