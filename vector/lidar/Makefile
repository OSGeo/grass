MODULE_TOPDIR = ../..

SUBDIRS1 = \
	v.surf.bspline \
	v.outlier \
	v.lidar.correction \
	v.lidar.edgedetection \
	v.lidar.growing

SUBDIRS = lidarlib $(SUBDIRS1)

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs

clean: cleansubdirs

$(SUBDIRS1): lidarlib

