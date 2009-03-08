# currently used only for osgeo4w
# TODO: use instead of Makefile
from distutils.core import setup, Extension

setup(
	ext_modules= [
		Extension(
			'_grass7_wxnviz',
			sources=[
				"grass7_wxnviz.i",
				"change_view.cpp",
				"draw.cpp",
				"init.cpp",
				"lights.cpp",
				"load.cpp",
				"surface.cpp",
				"vector.cpp",
				"volume.cpp",
				],
			swig_opts=['-c++','-shadow'],
			libraries=['grass_gis','grass_nviz','grass_ogsf','grass_g3d']
		)
	]
)
