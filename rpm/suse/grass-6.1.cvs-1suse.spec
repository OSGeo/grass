# SPEC file for Suse
# This file is Free Software under GNU GPL v>=2.
# Derived from source: GRASS 6.1 RPM spec file for ITC by Roberto Flor
# Derived from grass_FC4.spec

%define PACKAGE_NAME grass
%define PACKAGE_VERSION 6.1.cvs
%define PACKAGE_URL http://grass.itc.it/
%define shortver 61
%define cvssnapshot	_src_snapshot_2006_06_18
%define release 1suse10.0

%define with_64bitsupport 0
%define with_blas	0
%define with_ffmpeg	0
%define with_odbc	0
%define with_mysql	0
%define with_postgres	1
%define with_fftw	0
%define with_motif	0
%define with_glw        0
%define with_geos       0
%define with_largefiles	0

# Turn off automatic generation of dependencies to
# avoid a problem with libgrass* dependency issues.
# Other dependencies listed below.
%define _use_internal_dependency_generator 0
# Filter out the library number on provides
%define __find_provides %{_tmppath}/find_provides.sh
# Disable the _find_requires macro.
%define __find_requires %{nil}

Summary:	GRASS - Geographic Resources Analysis Support System
Name:		%PACKAGE_NAME
Version:	%PACKAGE_VERSION
Release:	%release
License:	GPL, Copyright by the GRASS Development Team
Group:		Applications/GIS
Packager:       Markus Neteler <neteler@itc.it>
Vendor:		GRASS Development Team
Source:		ftp://grass.itc.it/pub/grass/grass%{shortver}/source/snapshot/grass-%{version}%{cvssnapshot}.tar.gz
URL:            %PACKAGE_URL
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)/%{name}-%{version}
Prefix:         %{_prefix}

Requires:       gdal >= 1.3
Requires:	tcl >= 8.3
Requires:	tk >= 8.3
Requires:	proj >= 4.4.9
%if "%{with_geos}" == "1"
Requires:       geos >= 2.1.1
%endif
Requires:       freetype2 >= 2.0.0
Requires:       bash >= 3.0
%if "%{with_glw}" == "1"
Requires:       xorg-x11-Mesa-libGL >= 6.8
%endif
Requires:       xorg-x11-libs >= 6.8
%if "%{with_motif}" == "1"
Requires:       openmotif >= 2.2.3
%endif
%if "%{with_fftw}" == "1"
Requires:       fftw >= 2.1
%endif
Requires:       glibc >= 2.0
Requires:       libgcc >= 3.4.2
Requires:       ncurses >= 5.4
Requires:       libpng >= 1.2.8
Requires:       libstdc++ >= 3.4
Requires:       libtiff >= 3.6
Requires:       zlib >= 1.2
%if "%{with_blas}" == "1"
Requires:       blas >= 3.0
Requires:       lapack >= 3.0
%endif
%if "%{with_ffmpeg}" == "1"
Requires:       ffmpeg
%endif
%if "%{with_odbc}" == "1"
Requires:	unixODBC
%endif
%if "%{with_mysql}" == "1"
Requires:	mysql
%endif
%if "%{with_postgres}" == "1"
Requires:	postgresql-libs >= 7.3
%endif
BuildRequires:	bison
%if "%{with_fftw}" == "1"
BuildRequires:  fftw-devel >= 2.1
%endif
BuildRequires:	flex
BuildRequires:	freetype2-devel >= 2.0.0
BuildRequires:	libjpeg-devel
BuildRequires:	libpng-devel >= 1.2.2
BuildRequires:	man
BuildRequires:	ncurses-devel >= 5.2
%if "%{with_mysql}" == "1"
BuildRequires:	mysql-devel
%endif
%if "%{with_postgres}" == "1"
BuildRequires:	postgresql-devel
%endif
%if "%{with_odbc}" == "1"
BuildRequires:	unixODBC-devel
%endif
BuildRequires:	zlib-devel

#
# clean up of provides for other packages: gdal-grass, qgis etc.
#
# Turn off automatic generation of dependencies to
# avoid a problem with libgrass* dependency issues.
# Other dependencies listed below.
%define _use_internal_dependency_generator 0
# Filter out the library number on provides
%define __find_provides %{_tmppath}/find_provides.sh
# Disable the _find_requires macro.
%define __find_requires %{nil}


%description
GRASS (Geographic Resources Analysis Support System) is a Geographic
Information System (GIS) used for geospatial data management and
analysis, image processing, graphics/maps production, spatial
modeling, and visualization. GRASS is currently used in academic and
commercial settings around the world, as well as by many governmental
agencies and environmental consulting companies.


%prep
#%setup -q   ## run quietly with minimal output.
%setup  -n %{name}-%{version}%{cvssnapshot}  ## name the directory

#
# Filter out library number
#
cat > %{_tmppath}/find_provides.sh <<EOF
#!/bin/sh
/usr/lib/rpm/find-provides | sed -e 's/%{version}\.//g' | sort -u
exit 0
EOF
chmod ugo+x %{_tmppath}/find_provides.sh


%build
#
#configure with shared libs:
#
CFLAGS="-O2"
CXXFLAGS="-O2"
LDFLAGS="-s"

( %configure  \
   --prefix=%{_prefix} \
   --bindir=%{_bindir} \
   --libdir=%{_libdir}\
   --enable-shared \
%if "%{with_64bitsupport}" == "1"
   --enable-64bit \
%endif
%if "%{with_largefiles}" == "1"
   --enable-largefile \
%endif
%if "%{with_fftw}" == "1"
   --with-fftw \
%else
   --with-fftw=no \
%endif
   --with-includes=%{_includedir} \
   --with-libs=%{_libdir} \
   --with-freetype=yes \
   --with-freetype-includes=%{_includedir}/freetype2 \
   --with-nls \
   --with-gdal=%{_bindir}/gdal-config \
   --with-proj \
   --with-proj-includes=%{_includedir} \
   --with-proj-libs=%{_libdir} \
   --with-proj-share=/usr/share/proj/ \
   --with-sqlite \
%if "%{with_mysql}" == "1"
   --with-mysql \
   --with-mysql-includes=%{_includedir}/mysql \
   --with-mysql-libs=%{_libdir}/mysql \
%else
   --without-mysql \
%endif
%if "%{with_odbc}" == "1"
   --with-odbc  \
   --with-odbc-libs=%{_libdir} \
   --with-odbc-includes=%{_includedir} \
%else
   --without-odbc \
%endif
%if "%{with_postgres}" == "1"
   --with-postgres  \
   --with-postgres-includes=%{_includedir}/pgsql \
   --with-postgres-libs=%{_libdir} \
%else
   --without-postgres  \
%endif
%if "%{with_blas}" == "1"
   --with-blas  \
   --with-lapack  \
%endif
%if "%{with_ffmpeg}" == "1"
   --with-ffmpeg \
%endif
%if "%{with_motif}" == "1"
   --with-motif \
   --with-motif-includes=/usr/X11R6/include/Xm \
%endif
%if "%{with_glw}" == "1"
   --with-glw \
%endif
   --with-cxx
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
# GRASS 6 always installs into lib/:
echo %{_prefix}/grass-%{version}/lib >> %{buildroot}/etc/ld.so.conf.d/grass-%{version}.conf

%if "%{with_64bitsupport}" == "1"
# hack to satisfy SuSe's /usr/lib/rpm/brp-lib64-linux:
(cd %{buildroot}/%{_prefix}/grass-%{version}/ ; ln -s lib lib64)
%endif

%clean
rm -rf %{buildroot}

#cd ..
#rm -rf grass-%{version}

%files
%defattr(-,root,root)

%doc AUTHORS COPYING GPL.TXT README REQUIREMENTS.html

%attr(0755,root,root)

%{_bindir}/grass%{shortver}
%{_prefix}/grass-%{version}
/etc/ld.so.conf.d/grass-%{version}.conf

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%Changelog
* Tue Jun 20 2006 Markus Neteler <neteler@itc.it>
  - migrated to SuSe 10.0
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
