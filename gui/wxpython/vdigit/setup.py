# currently used only for osgeo4w
# TODO: use instead of Makefile
from distutils.core import setup, Extension

setup(
	ext_modules= [
		Extension(
			'_grass7_wxvdigit',
			sources=[
				"grass7_wxvdigit.i",
				"cats.cpp",
				"driver.cpp",
				"driver_draw.cpp",
				"driver_select.cpp",
				"line.cpp",
				"message.cpp",
				"select.cpp",
				"undo.cpp",
				"vertex.cpp",
				"pseudodc.cpp",
				"digit.cpp"
				],
			swig_opts=['-c++','-shadow'],
			libraries=['grass_dbmibase', 'grass_dbmiclient', 'grass_vect','grass_gis','grass_vedit','gdal_i', 'wxbase28u', 'wxmsw28u_core']
		)
	]
)
