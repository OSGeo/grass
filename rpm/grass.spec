%global shortver 79
%global macrosdir %(d=%{_rpmconfigdir}/macros.d; [ -d $d ] || d=%{_sysconfdir}/rpm; echo $d)

Name:		grass
Version:	7.9.0
Release:	1%{?dist}
Summary:	GRASS GIS - Geographic Resources Analysis Support System

%if 0%{?fedora} >= 33 || 0%{?rhel} >= 9
%bcond_without flexiblas
%endif

%if 0%{?rhel} >= 7
%define __python %{__python3}
%global python3_version_nodots 36
%global main_python3 1
%endif

# Note that the bcond macros are named for the CLI option they create.
# "%%bcond_without" means "ENABLE by default and create a --without option"
%bcond_without python3

License:	GPLv2+
URL:		https://grass.osgeo.org
Source0:	https://grass.osgeo.org/%{name}%{shortver}/source/%{name}-%{version}.tar.gz
# needed?
# Source2:	%%{name}-config.h

Patch1:		grass-7.8.0-buildroot.diff

BuildRequires:	bison
%if %{with flexiblas}
BuildRequires:	flexiblas-devel
%else
BuildRequires:	blas-devel, lapack-devel
%endif
BuildRequires:	cairo-devel
BuildRequires:	gcc-c++
BuildRequires:	desktop-file-utils
BuildRequires:	fftw-devel
BuildRequires:	flex
%if (0%{?rhel} > 6 || 0%{?fedora})
BuildRequires:	freetype-devel
%endif
BuildRequires:	gdal-devel
BuildRequires:	geos-devel
BuildRequires:	gettext
BuildRequires:	laszip-devel
%if (0%{?rhel} > 6 || 0%{?fedora})
BuildRequires:	libappstream-glib
%endif
BuildRequires:	libpng-devel
BuildRequires:	libtiff-devel
BuildRequires:	libXmu-devel
BuildRequires:	mesa-libGL-devel
BuildRequires:	mesa-libGLU-devel
%if (0%{?rhel} > 7 || 0%{?fedora})
BuildRequires:	mariadb-connector-c-devel openssl-devel
%else
BuildRequires:	mysql-devel
%endif
%if (0%{?rhel} > 6 || 0%{?fedora})
BuildRequires:	netcdf-devel
%endif
BuildRequires:	python3
%if 0%{?rhel} == 7
# EPEL7
BuildRequires:	python%{python3_version_nodots}-numpy
%else
BuildRequires:	python3-numpy
%endif
%if 0%{?rhel} && 0%{?rhel} <= 7
BuildRequires:	postgresql-devel
%else
BuildRequires:	libpq-devel
%endif
BuildRequires:	proj-devel
%if (0%{?rhel} > 7 || 0%{?fedora} >= 30)
BuildRequires:	proj-datumgrid
%else
BuildRequires:	proj-epsg
BuildRequires:	proj-nad
%endif
%if 0%{?fedora} >= 30
BuildRequires:	proj-datumgrid-world
%endif
%if (0%{?rhel} <= 6 && !0%{?fedora})
# argparse is included in python2.7+ but not python2.6
BuildRequires:	python-argparse
%endif
%if 0%{?rhel} == 7
# EPEL7
BuildRequires:	python%{python3_version_nodots}-dateutil
%else
BuildRequires:	python3-dateutil
%endif
BuildRequires:	python3-devel
%if (0%{?rhel} > 6 || 0%{?fedora})
BuildRequires:	python3-pillow
%else
# EPEL6
BuildRequires:	python-imaging
%endif
BuildRequires:	PDAL
BuildRequires:	PDAL-libs
BuildRequires:	PDAL-devel
BuildRequires:	readline-devel
BuildRequires:	sqlite-devel
BuildRequires:	subversion
BuildRequires:	unixODBC-devel
BuildRequires:	zlib-devel
BuildRequires:	bzip2-devel
BuildRequires:	libzstd-devel

Requires:	bzip2-libs
Requires:	libzstd
Requires:	geos
%if (0%{?rhel} > 7 || 0%{?fedora} >= 30)
Requires:	proj-datumgrid
%else
Requires:	proj-epsg
Requires:	proj-nad
%endif
%if 0%{?fedora} >= 30
Requires:  proj-datumgrid-world
%endif
Requires:	python3
%if 0%{?rhel} == 7
# EPEL7
Requires:	python%{python3_version_nodots}-numpy
%else
Requires:	python3-numpy
%endif
%if 0%{?rhel} > 6
# EPEL7/EPEL8
#Requires:  python3-matplotlib-wx
%else
Requires:	python3-matplotlib
%endif
%if 0%{?rhel} == 7
# EPEL7
Requires:	python%{python3_version_nodots}-dateutil
%else
Requires:	python3-dateutil
%endif
%if 0%{?rhel} && 0%{?rhel} < 7
Requires: wxPython
%else
Requires:	python3-wxpython4
%endif
Requires:	PDAL
Requires:	PDAL-libs

%if "%{_lib}" == "lib"
%global cpuarch 32
%else
%global cpuarch 64
%endif

Requires:	%{name}-libs%{?_isa} = %{version}-%{release}

%description
GRASS (Geographic Resources Analysis Support System) is a Geographic
Information System (GIS) used for geospatial data management and
analysis, image processing, graphics/maps production, spatial
modeling, and visualization. GRASS is currently used in academic and
commercial settings around the world, as well as by many governmental
agencies and environmental consulting companies.

%package libs
Summary:	GRASS GIS runtime libraries

%description libs
GRASS GIS runtime libraries

%package gui
Summary:	GRASS GIS GUI
Requires:	%{name}%{?isa} = %{version}-%{release}

%description gui
GRASS GIS GUI

%package devel
Summary:	GRASS GIS development headers
Requires:	%{name}-libs%{?_isa} = %{version}-%{release}

%description devel
GRASS GIS development headers

%prep
%setup -q

%patch1 -p1

# Correct mysql_config query
sed -i -e 's/--libmysqld-libs/--libs/g' configure

%if %{with flexiblas}
sed -i -e 's/-lblas/-lflexiblas/g' -e 's/-llapack/-lflexiblas/g' configure
%endif

# Fixup shebangs
find -name \*.pl | xargs sed -i -e 's,#!/usr/bin/env perl,#!%{__perl},'

%build
# Package is not ready for -Werror=format-security or the C++11 standard
CFLAGS="$(echo ${RPM_OPT_FLAGS} | sed -e 's/ -Werror=format-security//')"
CXXFLAGS="-std=c++98 ${CFLAGS}"
%configure \
	--with-cxx \
	--with-tiff \
	--with-png \
	--with-postgres \
%if 0%{?rhel} > 7
    --with-mysql=no \
%else
	--with-mysql \
%endif
	--with-opengl \
	--with-odbc \
	--with-fftw \
	--with-blas \
	--with-lapack \
%if %{with flexiblas}
	--with-blas-includes=%{_includedir}/flexiblas \
	--with-lapack-includes=%{_includedir}/flexiblas \
%endif
	--with-cairo \
%if (0%{?rhel} > 6 || 0%{?fedora})
	--with-freetype \
%endif
	--with-nls \
	--with-pdal \
	--with-readline \
	--with-regex \
	--with-openmp \
	--with-gdal=%{_bindir}/gdal-config \
	--with-wxwidgets=%{_bindir}/wx-config \
	--with-geos=%{_bindir}/geos-config \
%if (0%{?rhel} > 6 || 0%{?fedora})
	--with-netcdf=%{_bindir}/nc-config \
%endif
	--with-mysql-includes=%{_includedir}/mysql \
%if (0%{?fedora} >= 27)
	--with-mysql-libs=%{_libdir} \
%else
	--with-mysql-libs=%{_libdir}/mysql \
%endif
	--with-postgres-includes=%{_includedir}/pgsql \
	--with-cairo-ldflags=-lfontconfig \
	--with-freetype-includes=%{_includedir}/freetype2 \
	--with-bzlib \
	--with-zstd \
	--with-proj-share=%{_datadir}/proj

make %{?_smp_mflags}

# by default, grass will be installed to /usr/grass-%%{version}
# this is not FHS compliant: hide grass-%%{version} in %%{libdir}
%install
%make_install \
	DESTDIR=%{buildroot}%{_libdir} \
	prefix=%{buildroot}%{_libdir} \
	UNIX_BIN=%{buildroot}%{_bindir} \
	GISBASE_RUNTIME=%{_libdir}/%{name}%{shortver}

# libraries and headers are in GISBASE = %%{_libdir}/%%{name}
# keep them in GISBASE

# fix paths:
# Change GISBASE in startup script
for I in %{buildroot}%{_bindir}/%{name}%{shortver} \
	%{buildroot}%{_libdir}/%{name}%{shortver}/include/Make/Platform.make \
	%{buildroot}%{_libdir}/%{name}%{shortver}/include/Make/Grass.make \
	%{buildroot}%{_libdir}/%{name}%{shortver}/demolocation/.grassrc%{shortver} \
	%{buildroot}%{_libdir}/%{name}%{shortver}/etc/fontcap; do
	sed -i \
		-e 's|%{buildroot}%{_libdir}/%{name}-%{version}|%{_libdir}/%{name}%{shortver}|g' \
		-e 's|%{buildroot}%{_libdir}/%{name}%{shortver}|%{_libdir}/%{name}%{shortver}|g' \
		-e 's|%{buildroot}%{_bindir}|%{_bindir}|g' \
		$I
done

# fix paths in grass.pc
sed -i -e 's|%{_prefix}/%{name}-%{version}|%{_libdir}/%{name}%{shortver}|g' \
	%{name}.pc

mkdir -p %{buildroot}%{_libdir}/pkgconfig
install -p -m 644 %{name}.pc %{buildroot}%{_libdir}/pkgconfig

# Create multilib header
mv %{buildroot}%{_libdir}/%{name}%{shortver}/include/%{name}/config.h \
   %{buildroot}%{_libdir}/%{name}%{shortver}/include/%{name}/config-%{cpuarch}.h
# needed?
# install -p -m 644 %%{SOURCE2} %%{buildroot}%%{_libdir}/%%{name}%%{shortver}/include/%%{name}/config.h

# Make man pages available on the system, convert to utf8 and avoid name conflict
mkdir -p %{buildroot}%{_mandir}/man1
for man in $(ls %{buildroot}%{_libdir}/%{name}%{shortver}/docs/man/man1/*.1)
do
	iconv -f iso8859-1 -t utf8 $man > %{buildroot}%{_mandir}/man1/$(basename $man)"%{name}"
done

# create symlink to unversioned name
ln -s %{_bindir}/%{name}%{shortver} %{buildroot}%{_bindir}/%{name}

# symlink docs from GISBASE to standard system location
mkdir -p %{buildroot}%{_docdir}
# append shortver to destination since man pages are unversioned
ln -s %{_libdir}/%{name}%{shortver}/docs %{buildroot}%{_docdir}/%{name}%{shortver}

# Make desktop, appdata and icon files available on the system
mv %{buildroot}%{_libdir}/%{name}%{shortver}/share/* %{buildroot}%{_datadir}
desktop-file-validate %{buildroot}/%{_datadir}/applications/*.desktop
appstream-util validate-relax --nonet %{buildroot}/%{_datadir}/metainfo/org.osgeo.%{name}.appdata.xml

# Cleanup: nothing to do
#rm -rf %%{buildroot}%%{_prefix}/%%{name}-%%{version}

# Finally move entire tree to shortver subdir
#mv %%{buildroot}%%{_libdir}/%%{name}-%%{version} %%{buildroot}%%{_libdir}/%%{name}%%{shortver}

# rpm macro for version checking (not from buildroot!)
mkdir -p ${RPM_BUILD_ROOT}%{macrosdir}
cat > ${RPM_BUILD_ROOT}%{macrosdir}/macros.%{name} <<EOF
%%%{name}_version %{version}
EOF

# Add custom lib path to ld.conf.so.d
mkdir -p %{buildroot}%{_sysconfdir}/ld.so.conf.d
cat >  %{buildroot}%{_sysconfdir}/ld.so.conf.d/%{name}-%{_arch}.conf<<EOF
%{_libdir}/%{name}%{shortver}/lib
EOF

%if 0%{?rhel} && 0%{?rhel} <= 7
%post
/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :

%postun
if [ $1 -eq 0 ] ; then
	/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null
	/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi

%posttrans
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
%endif

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%files
%exclude %{_sysconfdir}/ld.so.conf.d/%{name}-%{_arch}.conf
%exclude %{_libdir}/%{name}%{shortver}/driver/db/*
%exclude %{_libdir}/%{name}%{shortver}/lib
%exclude %{_libdir}/%{name}%{shortver}/include
%exclude %{_libdir}/%{name}%{shortver}/gui
%{_libdir}/%{name}%{shortver}
%{_bindir}/*
%{_datadir}/metainfo/*
%{_datadir}/applications/*
%{_datadir}/icons/hicolor/*/apps/*
%{_mandir}/man1/*
%{_docdir}/%{name}%{shortver}

%files libs
%license AUTHORS COPYING GPL.TXT CHANGES
%{_sysconfdir}/ld.so.conf.d/%{name}-%{_arch}.conf
%{_libdir}/%{name}%{shortver}/lib/*.so
%{_libdir}/%{name}%{shortver}/lib/*.a
%dir %{_libdir}/%{name}%{shortver}/driver
%dir %{_libdir}/%{name}%{shortver}/driver/db
%{_libdir}/%{name}%{shortver}/driver/db/*

%files gui
%{_libdir}/%{name}%{shortver}/gui

%files devel
%doc TODO doc/* SUBMITTING
%{macrosdir}/macros.%{name}
%{_libdir}/pkgconfig/*
%dir %{_libdir}/%{name}%{shortver}/lib
%{_libdir}/%{name}%{shortver}/include

%changelog
* Mon Oct 05 2020 Markus Neteler <neteler@mundialis.de> - 7.8.4-1
- New upstream version GRASS GIS 7.8.4
- disabled %{name}-config.h

* Thu Aug 27 2020 Iñaki Úcar <iucar@fedoraproject.org> - 7.8.3-10
- https://fedoraproject.org/wiki/Changes/FlexiBLAS_as_BLAS/LAPACK_manager

* Sat Aug 01 2020 Fedora Release Engineering <releng@fedoraproject.org> - 7.8.3-9
- Second attempt - Rebuilt for
  https://fedoraproject.org/wiki/Fedora_33_Mass_Rebuild

* Tue Jul 28 2020 Fedora Release Engineering <releng@fedoraproject.org> - 7.8.3-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_33_Mass_Rebuild

* Fri Jun 26 2020 Markus Neteler <neteler@mundialis.de> - 7.8.3-7
- added PDAL support

* Sun Jun 21 2020 Markus Neteler <neteler@mundialis.de> - 7.8.3-6
- disable automagic byte compilation (BZ#1847153)

* Tue May 26 2020 Markus Neteler <neteler@mundialis.de> - 7.8.3-5
- fixed wxPython for F33 (BZ#1836761)

* Thu May 21 2020 Sandro Mani <manisandro@gmail.com> - 7.8.3-4
- Rebuild (gdal)

* Sat May 09 2020 Markus Neteler <neteler@mundialis.de> - 7.8.3-3
- disabled mysql on EPEL8 due to header problem
- updated PROJ package names for EPEL8

* Wed May 06 2020 Markus Neteler <neteler@mundialis.de> - 7.8.3-2
- enforce Python3 on EPEL7
- do not hardcode python version on Fedora but use macro

* Tue May 05 2020 Markus Neteler <neteler@mundialis.de> - 7.8.3
- new upstream version GRASS GIS 7.8.3

* Tue Mar 03 2020 Sandro Mani <manisandro@gmail.com> - 7.8.2-3
- Rebuild (gdal)

* Wed Jan 29 2020 Fedora Release Engineering <releng@fedoraproject.org> - 7.8.2-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_32_Mass_Rebuild

* Thu Dec 12 2019 Markus Neteler <neteler@mundialis.de> - 7.8.2
- new upstream version GRASS GIS 7.8.2

* Thu Oct 24 2019 Markus Neteler <neteler@mundialis.de> - 7.8.1
- new upstream version GRASS GIS 7.8.1
- it supports GDAL 3 and PROJ 6

* Thu Oct 24 2019 Markus Neteler <neteler@mundialis.de> - 7.8.0-5
- fix EPEL7 support

* Mon Sep 23 2019 Markus Metz <metz@mundialis.de> - 7.8.0-4
- enable bzip2 compression

* Fri Sep 20 2019 Markus Neteler <neteler@mundialis.de> - 7.8.0-3
- added missing proj-datumgrid and proj-datumgrid-world for >= F30
- fix create symlink to unversioned name

* Sat Sep 14 2019 Elliot C. Lee <elliot.c.lee@gmail.com> - 7.8.0-2
- patch for upstream package and SPEC file

* Fri Sep 13 2019 Markus Neteler <neteler@mundialis.de> - 7.8.0-1
- new upstream version of GRASS GIS 7.8.0
- dropped outdated liblas dependency

* Wed Aug 14 2019 Markus Neteler <neteler@mundialis.de> - 7.8.0-RC1
- test version of GRASS GIS 7.8.0RC1 with Python 3 support
- removal of no longer required Python 2/3 shebang update

* Thu Jul 25 2019 Fedora Release Engineering <releng@fedoraproject.org> - 7.6.0-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_31_Mass_Rebuild

* Mon Mar 18 2019 Orion Poplawski <orion@nwra.com> - 7.6.0-5
- Rebuild for netcdf 4.6.3

* Sun Feb 17 2019 Igor Gnatenko <ignatenkobrain@fedoraproject.org> - 7.6.0-4
- Rebuild for readline 8.0

* Mon Feb 04 2019 Devrim Gündüz <devrim@gunduz.org> - 7.6.0-3
- Rebuild for new GeOS and Proj

* Fri Feb 01 2019 Fedora Release Engineering <releng@fedoraproject.org> - 7.6.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_30_Mass_Rebuild

* Thu Jan 17 2019 Markus Neteler <neteler@mundialis.de> - 7.6.0-1
- new upstream version GRASS GIS 7.6.0

* Fri Jan 04 2019 Markus Neteler <neteler@mundialis.de> - 7.4.4-1
- new upstream version GRASS GIS 7.4.4

* Tue Nov 27 2018 Markus Neteler <neteler@mundialis.de> - 7.4.3-1
- new upstream version GRASS GIS 7.4.3

* Tue Nov 06 2018 Markus Neteler <neteler@mundialis.de> - 7.4.2-2
- fix to include libgrass_*.so files in grass-libs.rpm

* Sun Oct 28 2018 Markus Neteler <neteler@mundialis.de> - 7.4.2-1
- new upstream version GRASS GIS 7.4.2

* Sun Sep 09 2018 Pavel Raiskup <praiskup@redhat.com> - 7.4.1-8
- Clean up of PostgreSQL support (PR#4)

* Tue Jul 31 2018 Florian Weimer <fweimer@redhat.com> - 7.4.1-7
- Rebuild with fixed binutils

* Sun Jul 29 2018 Markus Neteler <neteler@mundialis.de> - 7.4.1-6
- added BuildRequires gcc-c++ to address RHBZ #1604262 due to RHBZ #1551327 (removing gcc and gcc-c++ from default buildroot)

* Fri Jul 13 2018 Fedora Release Engineering <releng@fedoraproject.org> - 7.4.1-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_29_Mass_Rebuild

* Sun Jul 08 2018 Markus Neteler <neteler@mundialis.de> - 7.4.1-4
- fix Python macro to explicitely use Python 2 interpreter

* Sat Jul 07 2018 Scott Talbert <swt@techie.net> - 7.4.1-3
- Update BRs: remove wxGTK-devel and add cairo-devel

* Sat Jun 23 2018 Markus Neteler <neteler@mundialis.de> - 7.4.1-2
- fix wxPython package dependency name for CentOS7

* Tue Jun 12 2018 Markus Neteler <neteler@mundialis.de> - 7.4.1-1
- new upstream version 7.4.1
- do not fail on EPEL6 with appstream-util

* Wed Apr 25 2018 Markus Neteler <neteler@mundialis.de> - 7.4.0-2
- add /etc/ld.so.conf.d/grass-*.conf to find libs by Daniele Viganò <daniele@vigano.me> (RHBZ #1571441)

* Mon Apr 09 2018 Markus Neteler <neteler@mundialis.de> - 7.4.0-1
- New upstream version 7.4.0

* Mon Mar 26 2018 Iryna Shcherbina <ishcherb@redhat.com> - 7.2.3-2
- Update Python 2 dependency declarations to new packaging standards
  (See https://fedoraproject.org/wiki/FinalizingFedoraSwitchtoPython3)

* Thu Mar 22 2018 Markus Neteler <neteler@mundialis.de> - 7.2.3-1
- new upstream version 7.2.3
- selected backport of major SPEC file cleanup from 7.4.0 done by Markus Metz <metz@mundialis.de>
- Fix grass-devel which needs include/grass and include/Make dirs done by Markus Metz <metz@mundialis.de
- store binaries in /usr/lib[64]/grass74/

* Sun Jan 28 2018 Markus Neteler <neteler@mundialis.de> - 7.2.2-2
- fix for EPEL in r.random.surface to avoid variable collision with GDAL
- SPEC cleanup with fix of dependencies between packages (review #1539116)
- appdata.xml file also go into '/usr/share/metainfo'
- use icon cache scriplets only on EPEL

* Mon Jan 15 2018 Markus Metz <metz@mundialis.de> - 7.2.2-1
- Major cleanup of SPEC file
- Fix grass-devel which needs include/grass and include/Make dirs

* Fri Jul 21 2017 Kalev Lember <klember@redhat.com> - 7.2.1-2
- Rebuilt for Boost 1.64

* Wed May 3 2017 Markus Neteler <neteler@mundialis.de> - 7.2.1-1
- New upstream version 7.2.1

* Thu Mar 2 2017 Markus Neteler <neteler@mundialis.de> - 7.2.0-3
- Fix for g.extension which needs include/ dir in grass-devel

* Sat Feb 4 2017 Markus Neteler <neteler@mundialis.de> - 7.2.0-2
- Fixes for EPEL

* Thu Jan 12 2017 Markus Neteler <neteler@mundialis.de> - 7.2.0-1
- New upstream version 7.2.0

* Thu Jan 12 2017 Igor Gnatenko <ignatenko@redhat.com> - 7.0.4-5
- Rebuild for readline 7.x

* Sun Jan 01 2017 Volker Froehlich <volker27@gmx.at> - 7.0.4-4
- Rebuild for libgeos

* Fri Oct 7 2016 Orion Poplawski <orion@cora.nwra.com> - 7.0.4-3
- Add patch to fix desktop file
- Validate desktop and appdata files
- Fix script interpreters
- Convert files to UTF-8 if needed

* Wed Sep 21 2016 Orion Poplawski <orion@cora.nwra.com> - 7.0.4-2
- Generate rpm macro file for version tracking

* Mon May 9 2016 Devrim Gündüz <devrim@gunduz.org> - 7.0.4-1
- Update to 7.0.4

* Mon Mar 07 2016 Thomas Kreuzer <thomas.kreuzer@uni-vechta.de> - 7.0.3-1
- New SPEC file for GRASS GIS 7.0.3

* Tue Sep 23 2014 Richard Hughes <richard@hughsie.com> - 6.4.4-5
- Install the shipped AppData file

* Sat Aug 16 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 6.4.4-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Sat Aug 09 2014 Ralf Corsépius <corsepius@fedoraproject.org> 6.4.4-2
- Rebase patches against grass-6.4.4.
- Convert -Werror=format-security into warnings (RHBZ#1106720).
- Minor spec cleanup.

* Fri Jul 25 2014 Peter Robinson <pbrobinson@fedoraproject.org> 6.4.4-1
- Update to 6.4.4
- Make 64bit conditionals generic

* Sat Jun 07 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 6.4.3-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Wed May 21 2014 Jaroslav Škarvada <jskarvad@redhat.com> - 6.4.3-7
- Rebuilt for https://fedoraproject.org/wiki/Changes/f21tcl86

* Thu Mar 27 2014 Ville Skyttä <ville.skytta@iki.fi> - 6.4.3-6
- Don't ship patch backup files

* Mon Oct 14 2013 Volker Fröhlich <volker27@gmx.at> - 6.4.3-5
- Solve build failure on PPC tests (BZ#961838)

* Wed Oct 9 2013 Devrim Gündüz <devrim@gunduz.org> - 6.4.3-4
- Rebuild against new GEOS

* Thu Oct  3 2013 Volker Fröhlich <volker27@gmx.at> - 6.4.3-3
- Add patch for release name encoding crash
- Use upstream desktop file (BZ #986852)
- Install icons of different sizes

* Sat Sep 14 2013 Volker Fröhlich <volker27@gmx.at> - 6.4.3-2
- Remove gcc patch (upstream)
- Remove useless BR for libjpeg
- Make config.h a source file instead of defining it in the spec file
- Truncate changelog

* Thu Sep 12 2013 Devrim GÜNDÜZ <devrim@gunduz.org> - 6.4.3-1
- Update to 6.4.3
- Rebuild with new geos.

* Tue Aug 27 2013 Orion Poplawski <orion@cora.nwra.com> - 6.4.2-11
- Rebuild for gdal 1.10.0

* Sat Aug 03 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 6.4.2-10
- Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild

* Fri Apr 12 2013 Jon Ciesla <limburgher@gmail.com> - 6.4.2-9
- Drop desktop vendor tag.

* Wed Mar 06 2013 Devrim GÜNDÜZ <devrim@gunduz.org> - 6.4.2-8
- Rebuild with new geos.

* Fri Jan 25 2013 Devrim GÜNDÜZ <devrim@gunduz.org> - 6.4.2-7
- Rebuild with geos 3.3.7.

* Sun Nov 18 2012 Volker Fröhlich <volker27@gmx.at> - 6.4.2-6
- Rebuild with ever newer geos

* Wed Nov 14 2012 Devrim GÜNDÜZ <devrim@gunduz.org> - 6.4.2-5
- Rebuild with new geos.

* Thu Jul 19 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 6.4.2-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Mon Jul 16 2012 Devrim GÜNDÜZ <devrim@gunduz.org> - 6.4.2-3
- Rebuilt

* Sun Mar  4 2012 Volker Fröhlich <volker27@gmx.at> - 6.4.2-2
- Solve name conflict with "parallel" man pages (BZ 797824)
- Correct man page encoding conversion
- Build with multiple workers; assumuption on race-condition was wrong

* Fri Mar  2 2012 Tom Callaway <spot@fedoraproject.org> - 6.4.2-1
- update to 6.4.2

* Tue Feb 28 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 6.4.1-8
- Rebuilt for c++ ABI breakage

* Tue Jan 10 2012 Volker Fröhlich <volker27@gmx.at> - 6.4.1-7
- Race condition in build system assumed -- going back to one worker

* Mon Jan 9 2012 Devrim GÜNDÜZ <devrim@gunduz.org> - 6.4.1-6
- Rebuilt with new geos

* Wed Nov 23 2011 Volker Fröhlich <volker27@gmx.at> - 6.4.1-5
- Move string substitution back to the install section, because
  it causes problems otherwise
- Add patch for libpng API change

* Wed Nov 02 2011 Volker Fröhlich <volker27@gmx.at> - 6.4.1-4
- Remove encoding from desktop file
- Remove BR on wxGTK, because wxGTK requires it anyway
- Disable Ubuntu patches, because they don't seem to work in Fedora
- Move all the string substitution for locales and docs to prep
- Use name macro in Source
- Drop custom compiler flags -- no evidence they serve a purpose
- Remove 2 unnecessary chmods
- Don't use sysconfdir macro in places, where etc means something different
- Add contributors to documentation

* Wed Nov 02 2011 Volker Fröhlich <volker27@gmx.at> - 6.4.1-3
- Patch locale and documentation paths properly for the GUI

* Thu Sep 22 2011 Volker Fröhlich <volker27@gmx.at> - 6.4.1-2
- Remove duplicate documentation
- Correct further documentation paths
- Create version-less symlinks for library directory and binary
- Supply all lang files to the files section directly
- Add ternary operator patch for Python 2.4 (ELGIS)

* Tue Aug 02 2011 Volker Fröhlich <volker27@gmx.at> - 6.4.1-1
- Update to 6.4.1
- Remove explicit lib and include dirs, where not necessary
- Really build with geos
- Remove sed call on ncurses

* Tue Aug 02 2011 Volker Fröhlich <volker27@gmx.at> - 6.4.0-4
- Correct license to GPLv2+
- Update URL
- Replace define with global macro
- Devel package required itself
- Simplify setup macro
- Don't add -lm manually anymore
- Correct FSF postal address
- Drop cstdio patch
- Correct Exec and Icon entry in desktop file
- Remove wrong and unnecessary translation entries from desktop file
  GRASS didn't start for the first issue
- Add numpy as requirement
- Delete defattr, as the defaults work right
- Use name macro where possible
- Devel package required itself
- Changelog doesn't need encoding conversion anymore
  Same goes for translators and infrastructure files
- Use mandir macro on one occasion
- Introduce "shortversion" macro
- Beautify case construction for 64 bit build flags
- Update syntax for Require on base package to guidelines
- Don't list LOCALE files twice, own directory
- Don't ship same documentation in different packages
- Drop README
- Simplify file list in devel package
- Replace extra icon source with one from the tarball

* Wed Mar 23 2011 Dan Horák <dan@danny.cz> - 6.4.0-3
- rebuilt for mysql 5.5.10 (soname bump in libmysqlclient)

* Wed Feb 09 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 6.4.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Mon Nov 22 2010 Viji Nair <viji [AT] fedoraproject DOT org> - 6.4.0-1
- Rebuilt with new gdal 1.7.3.
- Updated to upstream version 6.4.0.
- Removed grass-gdilib.patch
- Spec review

* Fri Dec 4 2009 Devrim GÜNDÜZ <devrim@gunduz.org> - 6.3.0-15
- Rebuilt with new geos

* Fri Aug 21 2009 Tomas Mraz <tmraz@redhat.com> - 6.3.0-14
- rebuilt with new openssl

* Fri Jul 24 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 6.3.0-13
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Mon Mar 23 2009 Lubomir Rintel <lkundrak@v3.sk> - 6.3.0-12
- Fix build with GCC 4.4

* Tue Feb 24 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 6.3.0-11
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Thu Jan 29 2009 Balint Cristian <cristian.balint@gmail.com> - 6.3.0-10
- email change
- rebuild for new mysql

* Sun Dec 07 2008 Balint Cristian <rezso@rdsor.ro> 6.3.0-9
- rebuild against newer gdal

* Sun Dec 07 2008 Balint Cristian <rezso@rdsor.ro> 6.3.0-8
- rebuild against newer gdal

* Sun Nov 30 2008 Ignacio Vazquez-Abrams <ivazqueznet+rpm@gmail.com> 6.3.0-7
- Rebuild for Python 2.6

* Sun Aug 24 2008 Balint Cristian <rezso@rdsor.ro> 6.3.0-6
- bz#458427 (prelink fail)
- bz#458563 (grass not able to display documentation)

* Sat Jul 05 2008 Balint Cristian <rezso@rdsor.ro> 6.3.0-5
- address bz#454146 (wxPython miss)

* Thu Jun 12 2008 Balint Cristian <rezso@rdsor.ro> 6.3.0-4
- address bz#341391 (multilib issue)

* Mon May 26 2008 Balint Cristian <rezso@rdsor.ro> 6.3.0-3
- bugfix initscripts permission

* Thu May 15 2008 Balint Cristian <rezso@rdsor.ro> 6.3.0-2
- require swig to build

* Thu May 15 2008 Balint Cristian <rezso@rdsor.ro> 6.3.0-1
- final stable release upstream
