# GRASS 6.X RPM spec file for ITC
# This file is Free Software under GNU GPL v>=2.
# Derived from grass_FC4.spec

# Define snap to use the snapshot version, change cvssnapshot and cvsversion accordingly
#
%define snap 0

%define with_blas	0
%define with_ffmpeg	0
%define with_fftw3	1
%define with_odbc	1
%define with_mysql	0
%define with_postgres	1
%define with_largefiles	1
%define with_motif      0
%define with_readline   0

#
# Turn off automatic generation of dependencies to
# avoid a problem with libgrass* dependency issues.
# Other dependencies listed below.
#
%define _use_internal_dependency_generator 0

# Filter out the library number on provides
%define __find_provides %{_tmppath}/find_provides.sh

# Disable the _find_requires macro.
%define __find_requires %{nil}


%if "%{snap:1}" == "0"
%define PACKAGE_VERSION 6.2.2
%define PACKAGE_RELEASE 1
%define shortver 62
%else
%define cvssnapshot	2006_11_03
%define cvsversion	6.2.2
%define PACKAGE_VERSION %{cvsversion}
%define PACKAGE_RELEASE 1
%define shortver 62
%endif


#Query the RPM database to find which redhat/fedora release we are running.
%if %(rpmquery fedora-release | grep -cv 'not installed$')
    %define FCL 1
    %define VER1 %(rpmquery --qf '%{VERSION}' fedora-release)
%endif
%if %(rpmquery redhat-release | grep -v 'not installed$' | grep -c -e '-[0-9][DAEW]')
    %define ENT 1
    %define VER1 %(rpmquery --qf '%{VERSION}' redhat-release|cut -c1)	
%endif
%if %(rpmquery sl-release | grep -v 'not installed$' | grep -c -e '-[0-9]')
    %define SLC 1
    %define VER1 %(rpmquery --qf '%{VERSION}' sl-release|cut -c1-)	
%endif


Summary:	GRASS - Geographic Resources Analysis Support System
Name:		grass
Version:	%PACKAGE_VERSION
Epoch: 		%PACKAGE_RELEASE
%{?FCL:Release: %{PACKAGE_RELEASE}.fc%{VER1}}
%{?ENT:Release: %{PACKAGE_RELEASE}.E%{VER1}}
%{?SLC:Release: %{PACKAGE_RELEASE}.SL%{VER1}}
%if "%{snap:1}" == "0"
Source:	    http://grass.itc.it/grass62/source/grass-%{PACKAGE_VERSION}.tar.bz2
%else
Source:     http://grass.itc.it/grass62/source/snapshot/grass-%{cvsversion}.cvs_src_snapshot_%{cvssnapshot}.tar.gz
%endif
License:	GPL, Copyright by the GRASS Development Team
Group:		Sciences/Geosciences
Packager:   Brad Douglas <rez@touchofmadness.com>
URL:        http://grass.itc.it/index.php
BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)/%{name}-%{version}
Prefix:     %{_prefix}

Requires:       gdal >= 1.3
BuildRequires:  gdal-devel >= 1.3
Requires:       tcl >= 8.3
BuildRequires:	tcl-devel >= 8.3
Requires:       tk >= 8.3
BuildRequires:	tk-devel >= 8.3
Requires:       proj >= 4.5.0
BuildRequires:	proj-devel >= 4.5.0
Requires:       geos >= 2.2.3
BuildRequires:  geos-devel >= 2.2.3
#Requires:       freetype >= 2.3.0
#BuildRequires:  freetype-devel >= 2.3.0
Requires:       freetype >= 2.2.0
BuildRequires:  freetype-devel >= 2.2.0
Requires:       bash >= 3.0
Requires:       mesa-libGL >= 6.5
BuildRequires:  mesa-libGL-devel >= 6.5
Requires:       libX11 >= 1.0
BuildRequires:  libX11-devel >= 1.0
Requires:		sqlite >= 3.3
BuildRequires:  sqlite-devel >= 3.3
%if "%{with_motif}" == "1"
Requires:       lesstif >= 0.95
BuildRequires:  lesstif-devel >= 0.95
%endif
#Requires:       glibc >= 2.6
#BuildRequires:  glibc-devel >= 2.6
Requires:       glibc >= 2.5
BuildRequires:  glibc-devel >= 2.5
Requires:       libgcc >= 4.1.0
#Requires:       ncurses >= 5.6
#BuildRequires:  ncurses-devel >= 5.6
Requires:       ncurses >= 5.5
BuildRequires:  ncurses-devel >= 5.5
Requires:       libpng >= 1.2.12
BuildRequires:  libpng-devel >= 1.2.12
Requires:		libjpeg
BuildRequires:	libjpeg-devel
Requires:       libstdc++ >= 4.1
BuildRequires:  libstdc++-devel >= 4.1
Requires:       libtiff >= 3.8
BuildRequires:  libtiff-devel >= 3.8
Requires:       zlib >= 1.2
BuildRequires:  zlib-devel >= 1.2
%if "%{with_readline}" == "1"
Requires:       readline >= 5.1
BuildRequires:  readline-devel >= 5.1
%endif
%if "%{with_fftw3}" == "1"
Requires:       fftw3 >= 3.1
BuildRequires:  fftw3-devel >= 3.1
%endif
%if "%{with_blas}" == "1"
Requires:       blas >= 3.1
BuildRequires:  blas >= 3.1
Requires:       lapack >= 3.1
BuildRequires:  lapack >= 3.1
%endif
%if "%{with_ffmpeg}" == "1"
Requires:       ffmpeg
BuildRequires:  ffmpeg-devel
%endif
%if "%{with_odbc}" == "1"
Requires:	    unixODBC >= 2.2
BuildRequires:	unixODBC-devel >= 2.2
%endif
%if "%{with_mysql}" == "1"
Requires:		mysql
BuildRequires:	mysql-devel
%endif
%if "%{with_postgres}" == "1"
#Requires:       postgresql-libs >= 8.2
#BuildRequires:	postgresql-devel >= 8.2
Requires:       postgresql-libs >= 8.1
BuildRequires:	postgresql-devel >= 8.1
%endif
Requires:		python >= 2.4
BuildRequires:	python-devel >= 2.4
BuildRequires:	bison
BuildRequires:	flex
BuildRequires:	man

Vendor: GRASS
#
# clean up of provides for other packages: gdal-grass, qgis etc.
#

%description
GRASS (Geographic Resources Analysis Support System) is a Geographic
Information System (GIS) used for geospatial data management and
analysis, image processing, graphics/maps production, spatial
modeling, and visualization. GRASS is currently used in academic and
commercial settings around the world, as well as by many governmental
agencies and environmental consulting companies.


%prep
%if "%{snap:1}" == "0"
%setup  -n %{name}-%{version}  ## name the directory
%else
%setup  -n %{name}-%{version}.cvs_src_snapshot_%{cvssnapshot}  ## name the directory
%endif

#
# Filter out library number
#
cat > %{_tmppath}/find_provides.sh <<EOF
#!/bin/sh
/usr/lib/rpm/redhat/find-provides | sed -e 's/%{version}\.//g' | sort -u
exit 0
EOF
chmod ugo+x %{_tmppath}/find_provides.sh

#
# Edit configure script for libraries
#
%if "%{with_readline}" == "1"
sed -i 's/-lreadline/-lreadline -lcurses/g' configure
chmod +x configure
%endif

%build
#
#configure with shared libs:
#
#%if "%{FCL}" == "1" &&  "%{VER1}" == "4"
#CFLAGS="-O2 -g -Wall"
#%else
#CFLAGS="-O2 -g -Wall -Werror-implicit-function-declaration -fno-common"
#%endif

CXXFLAGS="-O2 -g -Wall"

(%configure  \
    --enable-shared     \
    --with-includes=%{_includedir}  \
    --with-cxx          \
    --with-jpeg         \
    --with-tiff         \
    --with-png          \
    --with-tcltk        \
%if "%{with_postgres}" == "1"
    --with-postgres     \
    --with-postgres-includes=%{_includedir}/pgsql \
    --with-postgres-libs=%{_libdir}               \
%else
    --without-postgres  \
%endif
%if "%{with_mysql}" == "1"
    --with-mysql \
    --with-mysql-includes=%{_includedir}/mysql   \
    --with-mysql-libs=%{_libdir}/mysql           \
%else
    --without-mysql     \
%endif
    --with-sqlite       \
%if "%{with_ffmpeg}" == "1"
    --with-ffmpeg       \
    --with-ffmpeg-includes=%{_includedir}/ffmpeg \
    --with-ffmpeg-libs=%{_libdir}                \
%else
    --without-ffmpeg    \
%endif
    --with-opengl       \
%if "%{with_odbc}" == "1"
    --with-odbc         \
    --with-odbc-libs=%{_libdir}                  \
    --with-odbc-includes=%{_includedir}          \
%else
    --without-odbc      \
%endif
%if "%{with_fftw3}" == "1"
    --with-fftw         \
%else
    --without-fftw      \
%endif
%if "%{with_blas}" == "1"
    --with-blas         \
    --with-lapack       \
%else
    --without-blas      \
    --without-lapack    \
%endif
%if "%{with_motif}" == "1"
    --with-motif        \
%else
    --without-motif     \
%endif
    --with-freetype     \
    --with-freetype-includes=%{_includedir}/freetype2 \
    --without-glw       \
    --with-nls          \
%if "%{with_readline}" == "1"
    --with-readline     \
    --with-readline-includes=%{_includedir}/readline  \
    --with-readline-libs=%{_libdir}                   \
%else
    --without-readline  \
%endif
    --without-opendwg   \
    --with-curses       \
    --with-python       \
    --with-gdal=/usr/bin/gdal-config \
    --with-proj         \
    --with-proj-includes=%{_includedir} \
    --with-proj-libs=%{_libdir}         \
    --with-proj-share=%{_datadir}/proj  \
%if "%{with_largefiles}" == "1"
    --enable-largefile  \
%endif
    --with-x
)

#configure with shared libs:

make prefix=%{buildroot}%{_prefix} BINDIR=%{buildroot}%{_bindir}  \
     PREFIX=%{buildroot}%{_prefix}

%install

rm -rf %{buildroot}

make prefix=%{buildroot}%{_prefix} BINDIR=%{buildroot}%{_bindir} \
   PREFIX=%{buildroot}%{_prefix} install

# changing GISBASE in startup script (deleting %{buildroot} from path)
mv %{buildroot}%{_bindir}/grass%{shortver} %{buildroot}%{_bindir}/grass%{shortver}.tmp

cat  %{buildroot}%{_bindir}/grass%{shortver}.tmp | \
    sed -e "1,\$s&^GISBASE.*&GISBASE=%{_prefix}/grass-%{version}&" | \
    cat - > %{buildroot}%{_bindir}/grass%{shortver}

rm %{buildroot}%{_bindir}/grass%{shortver}.tmp
chmod +x %{buildroot}%{_bindir}/grass%{shortver}

# Make grass libraries available on the system
install -d %{buildroot}/etc/ld.so.conf.d
echo %{_prefix}/grass-%{version}/%{_lib} >> %{buildroot}/etc/ld.so.conf.d/grass-%{version}.conf

# Install pkg-config
if [ ! -d %{buildroot}%{_libdir}/pkgconfig ]
then
 mkdir -p %{buildroot}%{_libdir}/pkgconfig
fi
install -m 644 grass.pc %{buildroot}%{_libdir}/pkgconfig/

%clean
rm -rf %{buildroot}

#cd ..
#rm -rf grass-%{version}

%files
%defattr(-,root,root)

%doc AUTHORS COPYING GPL.TXT README REQUIREMENTS.html

%attr(0755,root,root)

%{_bindir}/grass%{shortver}
%{_bindir}/gem
%{_prefix}/grass-%{version}
%{_libdir}/pkgconfig/grass.pc
/etc/ld.so.conf.d/grass-%{version}.conf

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%Changelog
* Tue Jul 24 2007 Brad Douglas <rez@touchofmadness.com>
  - Commented out FLAGS section - use default %configure eval
  - Reorganized configure layout and specified all options
* Tue Jul 17 2007 Brad Douglas <rez@touchofmadness.com>
  - Conditionalized readline dependency
* Tue Dec 12 2006 Brad Douglas <rez@touchofmadness.com>
  - Conditionalized openmotif/lesstif dependency for (>=FC5)
* Tue Nov 15 2006 Brad Douglas <rez@touchofmadness.com>
  - sed configure to add ncurses dependency for readline (>=FC5)
* Fri Oct 20 2006 Markus Neteler <neteler@itc.it>
  - fftw3 conditionalized (needed for FC4); less strict compiler flags
* Tue Oct 17 2006 Roberto Flor <flor@itc.it>
  - Moved to 6.2.0RC3, enabled ffmpeg, added snapshot/version flag
* Tue Mar 17 2006 Roberto Flor <flor@itc.it>
  - Added with/without option
  - Added cvs snapshot option
  - Fixed a lot of inconsistency on x86_64 lib
* Tue Feb 28 2006 Roberto Flor <flor@itc.it>
  - Small changes and cleanup. Requires FC4 or RH Enterprise 4.
  - Dirty fix for provides error
* Thu Oct 12 2005 Markus Neteler <neteler@itc.it>
  - First build of RPM for Fedora Core 4.
* Thu Mar 30 2005 Craig Aumann <caumann@ualberta.ca>
  - First build of RPM for Fedora Core 3.
* Wed Sep 01 2004 Bernhard Reiter <bernhard@intevation.de>
  - made ready to be checked into GRASS CVS: added header, disabled Patch1
* Tue Aug 10 2004 Silke Reimer <silke.reimer@intevation.net>
  - small changes to fit to Fedora naming conventions
* Thu Jul 01 2004 Silke Reimer <silke.reimer@intevation.net>
  - Initial build
