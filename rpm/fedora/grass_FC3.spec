# GRASS 6.0 RPM spec file for Fedora
# This file is Free Software under GNU GPL v>=2. 
# $Id$

%define PACKAGE_NAME grass
%define PACKAGE_VERSION 6.0.0
%define PACKAGE_URL http://grass.itc.it/index.php
%define _prefix /usr/local
%define _bindir /usr/bin
%define shortver 60

## Disable the automatic running of the _find_requires macro.  
#%define _find_requires %{nil}  

## Turn off automatic generation of dependencies to 
## avoid a problem with libgrass* dependency issues.  
## Other dependencies listed below.  

Autoreq: 0  

#Query the RPM database to find which redhat/fedora release we are running.
%if %(rpmquery fedora-release | grep -cv 'not installed$')
    %define FCL 1
    %define VER1 %(rpmquery --qf '%{VERSION}' fedora-release)
%endif
%if %(rpmquery redhat-release | grep -cv 'not installed$')
    %define RHL 1
    %define VER1 %(rpmquery --qf '%{VERSION}' redhat-release)	
%endif
%if %(rpmquery redhat-release | grep -v 'not installed$' | grep -c -e '-[0-9][AEW]S')
    %define ENT 1
%endif

%define REL 3



Summary:	GRASS - Geographic Resources Analysis Support System
Name:		%PACKAGE_NAME
Version:	%PACKAGE_VERSION
Epoch: 0
%{?FCL:Release: 0.fdr.%{REL}.fc%{VER1}}
%{?RHL:Release: 0.fdr.%{REL}.rh%{VER1}}
Source:	        ftp://grass.itc.it/pub/grass/grass60/source/grass-%{version}.tar.gz
Copyright:	GPL, Copyright by the GRASS Development Team
Group:		Sciences/Geosciences
Packager:       Craig Aumann <caumann@ualberta.ca> 
URL:            %PACKAGE_URL
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)/%{name}-%{version}
#BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
#BuildRoot:      %{_builddir}/%{name}-%{version}
Prefix:         %{_prefix}

Requires:       gdal >= 1.2
Requires:	tcl >= 8.3
Requires:	tk >= 8.3
Requires:	proj >= 4.4.9
Requires:       geos >= 2.1.1
Requires:       freetype >= 2.0.0
Requires:       bash >= 3.0
Requires:       xorg-x11-Mesa-libGL >= 6.8
Requires:       xorg-x11-libs >= 6.8
Requires:       openmotif >= 2.2.3
#Requires:       blas >= 3.0
#Requires:       lapack >= 3.0
Requires:       fftw >= 2.1
Requires:       fftw < 3.0
Requires:       glibc >= 2.0
Requires:       libgcc >= 3.4.2
Requires:       ncurses >= 5.4
Requires:       libpng >= 1.2.8
Requires:       libstdc++ >= 3.4
Requires:       libtiff >= 3.6
Requires:       zlib >= 1.2

#Requires:	lesstif
#Requires:	unixODBC
#Requires:	mysql
#Requires:	postgresql >= 7.3
BuildRequires:	bison
#BuildRequires:	fftw2-devel
BuildRequires:	flex
#BuildRequires:	freetype2-devel >= 2.0.0
#BuildRequires:	gcc-g77
BuildRequires:	libjpeg-devel
BuildRequires:	libpng-devel >= 1.2.2
BuildRequires:	man
#BuildRequires:	lesstif-devel
BuildRequires:	ncurses-devel >= 5.2
BuildRequires:	mysql-devel
BuildRequires:	postgresql-devel
BuildRequires:	unixODBC-devel
BuildRequires:	zlib-devel


Vendor: GRASS 


%description
GRASS (Geographic Resources Analysis Support System) is a Geographic
Information System (GIS) used for geospatial data management and
analysis, image processing, graphics/maps production, spatial
modeling, and visualization. GRASS is currently used in academic and
commercial settings around the world, as well as by many governmental
agencies and environmental consulting companies.


%prep
%setup -q   ## run quietly with minimal output. 
%setup  -n %{name}-%{version}  ## name the directory 


%build

#configure with shared libs:

CFLAGS="-O2" 
LDFLAGS="-s"   

( %configure  \
   --prefix=%{buildroot}/%{_prefix} \
   --bindir=%{buildroot}/%{_bindir} \
   --with-cxx \
   --enable-shared \
   --with-fftw \
   --with-includes=/usr/include \
   --with-libs=/usr/lib \
   --with-motif \
   --with-freetype=yes \
   --with-freetype-includes=/usr/include/freetype2 \
   --with-nls \
   --with-gdal=/usr/bin/gdal-config \
   --with-proj \
   --with-proj-includes=/usr/include \
   --with-proj-libs=/usr/lib \
   --with-glw \
   --with-x  \
   --with-mysql \
   --with-mysql-includes=/usr/include/mysql \
   --with-mysql-libs=/usr/lib/mysql \
   --with-odbc  \
   --with-odbc-libs=/usr/lib \ 
   --with-odbc-includes=/usr/include \
   --with-postgres  \
   --with-postgres-includes=/usr/include/pgsql \ 
   --with-postgres-libs=/usr/lib )
  

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

%clean
rm -rf %{buildroot}

#cd ..
#rm -rf grass-%{version}

%files
%defattr(-,root,root)

%doc AUTHORS COPYING GPL.TXT README REQUIREMENTS.html 

%attr(0755,root,root) 

%{_bindir}/grass%{shortver}
#%{_prefix}/bin/grass%{shortver}
# %attr(1777,root,root) 
#%{_prefix}/grass-%{version}/locks
%{_prefix}/grass-%{version}

%post
#echo -n "Running ldconfig..."
ldconfig
#echo "done."

echo -n "linking libgrass ..."
cd /usr/lib/
ln -s /usr/local/grass-%{version}/lib/libgrass_I.so .
ln -s /usr/local/grass-%{version}/lib/libgrass_vask.so .
ln -s /usr/local/grass-%{version}/lib/libgrass_gmath.so .
ln -s /usr/local/grass-%{version}/lib/libgrass_gis.so
ln -s /usr/local/grass-%{version}/lib/libgrass_datetime.so .
echo "done."

%postun
#echo -n "Running ldconfig..."
ldconfig
#echo "done."

# delete softlinks for libgrass
rm -f /usr/lib/libgrass_*

%Changelog
* Thu Mar 30 2005 Craig Aumann <caumann@ualberta.ca> 
  - First build of RPM for Fedora Core 3.

* Wed Sep 01 2004 Bernhard Reiter <bernhard@intevation.de>
  - made ready to be checked into GRASS CVS: added header, disabled Patch1

* Tue Aug 10 2004 Silke Reimer <silke.reimer@intevation.net>
  - small changes to fit to Fedora naming conventions

* Thu Jul 01 2004 Silke Reimer <silke.reimer@intevation.net>
  - Initial build

