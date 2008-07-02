# GRASS 6.0.0 RPM spec file for Mandrake
# Date: 2006-06-23

%define PACKAGE_NAME grass
%define PACKAGE_VERSION 6.0.0
%define PACKAGE_URL http://grass.itc.it/index.php
%define _prefix /usr/local
%define shortver 60

Summary:	GRASS - Geographic Resources Analysis Support System
Name:		%PACKAGE_NAME
Version:	%PACKAGE_VERSION
Release:	1
Copyright:	GPL
Group:		Applications/GIS
Vendor:         GRASS Development Team
URL:		http://grass.itc.it
Source:		http://grass.itc.it/grass60/source/grass-%{PACKAGE_VERSION}.tar.gz
Packager:       Markus Neteler <neteler itc it>

Requires:	gdal >= 1.2
Requires:	tcl >= 8
Requires:	tk >= 8
#Requires:	mysql
#Requires:	postgresql >= 7.3
Requires:	proj >= 4.4.5
Requires:	lesstif
BuildRequires:	bison
BuildRequires:	fftw3-devel
BuildRequires:	flex
BuildRequires:	freetype2-devel 
BuildRequires:	gcc-g77
BuildRequires:	libjpeg-devel
BuildRequires:	libpng-devel >= 1.2.2
BuildRequires:	man
BuildRequires:	lesstif-devel
BuildRequires:	ncurses-devel >= 5.2
BuildRequires:  zlib-devel
#BuildRequires:	mysql-devel
#BuildRequires:	postgresql-devel
#BuildRequires:	unixODBC-devel

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
#cd src/CMD/generic/
#%patch0 -p0 
#cd ../../..

#configure with shared libs:
CFLAGS="-O2" LDFLAGS="-s" ./configure \
	--prefix=%%{buildroot}/{_prefix} --bindir=%{buildroot}/%{_bindir} \
	--enable-shared \
	--with-cxx \
	--with-gdal=/usr/bin/gdal-config \
	--with-postgres-includes=/usr/include/pgsql --with-postgres-libs=/usr/lib \
	--with-mysql-includes=/usr/include/mysql --with-mysql-libs=/usr/lib/mysql \
	--without-odbc \
	--with-fftw \
	--with-nls \
	--with-freetype --with-freetype-includes=/usr/include/freetype2

%build

make prefix=%{buildroot}/%{_prefix} BINDIR=%{buildroot}/%{_bindir} \
	PREFIX=%{buildroot}%{_prefix}

%install
make strip
make prefix=%{buildroot}/%{_prefix} BINDIR=%{buildroot}/%{_bindir} \
	PREFIX=%{buildroot}%{_prefix} install

# changing GISBASE in startup script (deleting %{buildroot} from path)
mv %{buildroot}%{_prefix}/bin/grass%{shortver} %{buildroot}%{_prefix}/bin/grass%{shortver}.tmp
cat  %{buildroot}%{_prefix}/bin/grass%{shortver}.tmp |
	sed -e "1,\$s&^GISBASE.*&GISBASE=%{_prefix}/grass-%{PACKAGE_VERSION}&" |
    cat - > %{buildroot}%{_prefix}/bin/grass%{shortver}
rm %{buildroot}%{_prefix}/bin/grass%{shortver}.tmp
chmod +x %{buildroot}%{_prefix}/bin/grass%{shortver}

%clean
rm -rf %{buildroot}
cd ..
rm -rf grass-%{version}

# Not functional yet (someone please fix this):
#%post
# # changing GISBASE (deleting %{buildroot} from path), needed for
# # relocatable packaging:
# mv %{_prefix}/bin/grass%{shortver} %{_prefix}/bin/grass%{shortver}.tmp
# cat  %{_prefix}/bin/grass%{shortver}.tmp |
#	sed -e "1,\$s&^GISBASE.*&GISBASE=%{_prefix}/grass-%{PACKAGE_VERSION}&" |
#	cat - > %{_prefix}/bin/grass%{shortver}
# rm %{_prefix}/bin/grass%{shortver}.tmp
# chmod +x %{_prefix}/bin/grass%{shortver}

%files
%defattr(-,root,root)
%attr(0755,root,root) %{_prefix}/bin/grass%{shortver}
%{_prefix}/bin/grass%{shortver}
%{_prefix}/grass-%{PACKAGE_VERSION}/

%doc AUTHORS COPYING GPL.TXT README REQUIREMENTS.html

%changelog
* Fri Mar 11 2005 Markus Neteler <neteler itc it> 6.0.0-1
- updated to GRASS 6.0.0

* Tue Nov 9 2004 Markus Neteler <neteler itc it> 5.7.0-2
- GRASS 5.3 no longer required as all code moved into this repository

* Wed Jun 17 2004 Markus Neteler <neteler itc it> 5.7.0-1
- removed unixODBC, added mysql
- specfile cleanup

* Tue May 24 2004 Markus Neteler <neteler itc it> 5.7.0-1beta4
- rewritten from 5.3 specs
