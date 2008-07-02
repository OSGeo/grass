# GRASS 5.7 RPM spec file for Fedora
# This file is Free Software under GNU GPL v>=2. 
# Date: 2005-08-10

%define PACKAGE_NAME grass
%define PACKAGE_VERSION 5.7.0
%define PACKAGE_URL http://grass.itc.it/index.html
%define _prefix /usr/lib
%define _bindir /usr/bin

Summary: GRASS - Geographic Resources Analysis Support System
Name: %PACKAGE_NAME
Version: %PACKAGE_VERSION
Release: 1.fdr.2
Epoch: 1
Source: grass-5.7.0.tar.gz
# Necessary until RT bug #2526 is solved.
Patch0: grass-readline.patch
# Patch1 disabled because this was fixed in GRASS CVS already.
# See RT bug 2525, fix is in  grass: src/libes/ogsf/GS2.c rev 1.17
#Patch1: grass-zrange.patch
License: GPL; Copyright by the GRASS Development Team
Group: Applications/Productivity

BuildRequires:gdal-devel, tcl-devel >= 0:8, tk-devel >= 0:8, unixODBC-devel,  readline-devel, proj-devel, zlib-devel, libpng-devel, libtiff-devel, postgresql-devel
Requires: gdal,  tcl >= 0:8, tk >= 0:8, readline, proj, unixODBC, postgresql-libs, libtiff, libpng, zlib
BuildRoot: %{_builddir}/%{name}-root
Prefix: %{_prefix}

%description
GRASS (Geographic Resources Analysis Support System)
is a raster-based GIS, vector GIS,
image processing system, graphics production
system, data management system, and spatial
modeling system. A graphical user interface
for X-Windows is provided. 

%prep
%setup -n %{name}-%{version}
%patch0 -p0 
#%patch1 -p0

CFLAGS="-O2" LDFLAGS="-s" ./configure --prefix=%{_prefix} --bindir=%{_bindir}  --with-gdal --with-readline --with-fftw=no

%build
make

# compiling grass
make prefix=%{buildroot}%{_prefix} BINDIR=%{buildroot}%{_bindir} \
PREFIX=%{buildroot}%{_prefix} install-strip

%install
make prefix=%{buildroot}%{_prefix} BINDIR=%{buildroot}%{_bindir} \
PREFIX=%{buildroot}%{_prefix} install-strip

# changing GISBASE (deleting %{buildroot} from path)
mv %{buildroot}%{_bindir}/grass57 %{buildroot}%{_bindir}/grass57.tmp
cat  %{buildroot}%{_bindir}/grass57.tmp |
	sed -e "1,\$s&^GISBASE.*&GISBASE=%{_prefix}/%{name}-%{version}&" |
    cat - > %{buildroot}%{_bindir}/grass57
rm %{buildroot}%{_bindir}/grass57.tmp
chmod +x %{buildroot}%{_bindir}/grass57

if  grep -q ignored error.log ; then
	echo An error occurred and was ignored. 
	echo I will print the error.log and stop for you.
	cat error.log
	exit 1
fi

%clean
rm -rf %{_builddir}/%{name}-%{version}
echo `pwd`
rm -rf %{buildroot}

%files
%defattr(-,root,root)

%{_bindir}
%{_prefix}
%doc CHANGES AUTHORS INSTALL COPYING GPL.TXT README REQUIREMENTS.html TODO 

%Changelog 
* Wed Sep 01 2004 Bernhard Reiter <bernhard@intevation.de>
- made ready to be checked into GRASS CVS: added header, disabled Patch1

* Tue Aug 10 2004 Silke Reimer <silke.reimer@intevation.net>
- small changes to fit to Fedora naming conventions

* Thu Jul 01 2004 Silke Reimer <silke.reimer@intevation.net>
- Initial build
