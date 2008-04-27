# GRASS 6 RPM spec file for Mandrake

%define PACKAGE_NAME grass
%define PACKAGE_VERSION 6.1.cvs
%define release 1mdk2006
%define _prefix /usr/lib
%define shortver 61
%define longver %{PACKAGE_VERSION}


Summary:	GRASS - Geographic Resources Analysis Support System
Name:		%PACKAGE_NAME
Version:	%PACKAGE_VERSION
Release:	%release
License:	GPL
Group:		Applications/GIS
Vendor:         GDF Hannover bR <info@gdf-hannover.de>
URL:		http://grass.itc.it
Source:		http://grass.itc.it/grass%{shortver}/source/grass-%{PACKAGE_VERSION}.tar.gz

Requires:	gdal >= 1.3
Requires:	tcl >= 8
Requires:	tk >= 8
Requires:	proj >= 4.4.9
Requires:	lesstif
Requires:	sqlite3-tools
Requires:	libfftw3 
BuildRequires:	bison
BuildRequires:  libfftw3 
BuildRequires:	libfftw3-devel
BuildRequires:	flex
BuildRequires:	freetype2-devel 
BuildRequires:	libjpeg-devel
BuildRequires:	libpng-devel >= 1.2.2
BuildRequires:	man
BuildRequires:	lesstif-devel
BuildRequires:	ncurses-devel >= 5.2
BuildRequires:  zlib-devel
BuildRequires:  libtiff3-devel
BuildRequires:  libxorg-x11-devel
BuildRequires:  libMesaGLU1-devel
BuildRequires:  libsqlite3_0-devel

BuildRoot: %{_builddir}/%{name}-root
Prefix: %{_prefix}

%description
GRASS (Geographic Resources Analysis Support System), commonly
referred to as GRASS, is a Geographic Information System
(GIS) used for geospatial data management and analysis, image
processing, graphics/maps production, spatial modeling, and
visualization. GRASS is currently used in academic and commercial
settings around the world, as well as by many governmental agencies
and environmental consulting companies.

%prep
%setup -n grass-%{version}

#configure with shared libs:
CFLAGS="-O2" LDFLAGS="-s" ./configure \
	--prefix=%{_prefix} \
	--enable-shared \
	--with-cxx \
	--with-gdal=/usr/bin/gdal-config \
	--with-postgres --with-postgres-includes=/usr/include/pgsql \
	--with-sqlite \
	--without-odbc \
	--with-fftw \
	--with-nls \
	--with-glw \
	--with-freetype \
	--with-freetype-includes=/usr/include/freetype2

%build
make prefix=%{buildroot}/%{_prefix} BINDIR=%{buildroot}/%{_bindir} \
PREFIX=%{buildroot}%{_prefix}

%install
make prefix=%{buildroot}%{_prefix} BINDIR=%{buildroot}%{_bindir} \
PREFIX=%{buildroot}%{_prefix} install

# changing GISBASE in startup script (deleting %{buildroot} from path)
mv %{buildroot}%{_prefix}/bin/grass%{shortver} %{buildroot}%{_prefix}/bin/grass%{shortver}.tmp
cat  %{buildroot}%{_prefix}/bin/grass%{shortver}.tmp |
	sed -e "1,\$s&^GISBASE.*&GISBASE=%{_prefix}/grass-%{PACKAGE_VERSION}&" |
    cat - > %{buildroot}%{_prefix}/bin/grass%{shortver}
rm %{buildroot}%{_prefix}/bin/grass%{shortver}.tmp
chmod +x %{buildroot}%{_prefix}/bin/grass%{shortver}

# Make grass libraries available on the system
install -d %{buildroot}/etc/ld.so.conf.d
echo %{_prefix}/grass-%{version}/lib >> %{buildroot}/etc/ld.so.conf.d/grass-%{version}.conf

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc AUTHORS COPYING GPL.TXT README REQUIREMENTS.html

%attr(0755,root,root)
%{_prefix}/bin/grass%{shortver}
%{_prefix}/bin/gem6
%{_prefix}/grass-%{PACKAGE_VERSION}
/etc/ld.so.conf.d/grass-%{version}.conf

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%changelog
* Thu Mar 21 2006 Markus Neteler <neteler gdf-hannover de>
- upgraded to 6.1.cvs - fftw3 also supported now
* Thu Nov 17 2005 Markus Neteler <neteler itc it>
- upgraded to Mandriva 2006, 6.1.cvs
* Fri Aug 05 2005 Otto Dassau <dassau gdf-hannover de> 6.0.1RC2
- changed prefix 
* Fri Aug 05 2005 Markus Neteler <neteler gdf-hannover de> 6.0.1RC1
- updated to GRASS 6.0.1
* Fri Mar 11 2005 Markus Neteler <neteler itc it> 6.0.0-1
- updated to GRASS 6.0.0
* Tue Nov 9 2004 Markus Neteler <neteler itc it> 5.7.0-2
- GRASS 5.3 no longer required as all code moved into this repository
* Wed Jun 17 2004 Markus Neteler <neteler itc it> 5.7.0-1
- removed unixODBC, added mysql
- specfile cleanup
* Tue May 24 2004 Markus Neteler <neteler itc it> 5.7.0-1beta4
- rewritten from 5.3 specs
