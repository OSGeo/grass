#!/bin/bash

############################################################################
#
# MODULE:       dep_tree2sql.sh
# AUTHOR(S):    Glynn Clements
# PURPOSE:      run "nm" on every object file, library and executable to
#               find dependencies, stored in PostgreSQL database
# COPYRIGHT:    (C) 2006 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################
#
# From: Glynn Clements <glynn at gclements.plus.com>
# Date: Mon, 12 Jun 2006 13:00:25 +0100
#
# Usage: after having compiled GRASS, run "tools/dep_tree2sql.sh `pwd`" from the
# top of the GRASS source tree.
#
# Essentially, the script runs "nm" on every object file, library and
# executable it finds (and "ldd" on all of the executables), processes
# the output with egrep/sed/awk, then imports the results into a
# PostgreSQL database.
#
# "nm" lists the symbol table of an object file, library, or executable,
# indicating whether each symbol is imported into, defined in and/or
# exported from the file.
#
# Most of the database tables record which symbols are imported into or
# exported from which object files, libraries or executables. E.g. the
# "obj_imp" table lists which symbols are imported into which object
# files.
#
# You can then use simple queries such as:
#
# 	grass=> SELECT object FROM obj_imp WHERE symbol = 'I_get_target' ;
# 	                               object                               
# 	--------------------------------------------------------------------
# 	 imagery/i.ortho.photo/photo.2image/OBJ.i686-pc-linux-gnu/target.o
# 	 imagery/i.ortho.photo/photo.2target/OBJ.i686-pc-linux-gnu/target.o
# 	 imagery/i.ortho.photo/photo.elev/OBJ.i686-pc-linux-gnu/main.o
# 	 imagery/i.ortho.photo/photo.rectify/OBJ.i686-pc-linux-gnu/target.o
# 	 imagery/i.ortho.photo/photo.target/OBJ.i686-pc-linux-gnu/main.o
# 	 imagery/i.points/OBJ.i686-pc-linux-gnu/target.o
# 	 imagery/i.rectify/OBJ.i686-pc-linux-gnu/target.o
# 	 imagery/i.vpoints/OBJ.i686-pc-linux-gnu/target.o
#
# to discover which files import a given symbol, or more complex queries
# such as:
#
# 	grass=> SELECT DISTINCT b.object FROM lib_exp a, obj_imp b
# 	grass-> WHERE a.library = 'libgrass_form.6.1.cvs.so' AND a.symbol = b.symbol ;
# 	                          object                           
# 	-----------------------------------------------------------
# 	 display/d.what.vect/OBJ.i686-pc-linux-gnu/what.o
# 	 vector/v.digit/OBJ.i686-pc-linux-gnu/attr.o
# 	 vector/v.digit/OBJ.i686-pc-linux-gnu/line.o
# 	 vector/v.what/OBJ.i686-pc-linux-gnu/what.o
# 	 visualization/nviz/src/OBJ.i686-pc-linux-gnu/query_vect.o
# 	(5 rows)
#
# to discover which files import any symbol defined in a specific
# library. And so on.
#
# For simple "which files use this function" queries, a database lookup
# is quicker and more reliable than grep-ing the source tree.
#
# Assuming that the dep_tree2sql.sh script runs successfully (some of it is
# Linux-specific, other bits are PostgreSQL-specific, but the changes
# required for a different OS or RDBMS should be quite minor), the
# easiest way to figure out what is in a given table (apart from looking
# at the name) is to just sample it, e.g.:
#
# 	grass=> SELECT * FROM stlib_exp LIMIT 5 ;
# 	      library      |   object   |    symbol     
# 	-------------------+------------+---------------
# 	 libgrass_manage.a | add_elem.o | add_element
# 	 libgrass_manage.a | ask.o      | ask_in_mapset
# 	 libgrass_manage.a | ask.o      | ask_new
# 	 libgrass_manage.a | ask.o      | ask_old
# 	 libgrass_manage.a | copyfile.o | copyfile
# 	(5 rows)
#


tmpdir=/tmp/sql-grass
dbname=grass

if [ -n "$1" ] ; then
	builddir="$1"
else
	echo "Usage: del_tree2sql.sh <source directory>" >&2
	exit 1
fi

rm -rf "$tmpdir"
mkdir -m 711 "$tmpdir" || exit 1

cd $builddir

( cd dist.*

#LD_LIBRARY_PATH=`pwd`/lib
#export LD_LIBRARY_PATH

find . -type f -perm +111 \! -name '*.so.*' \
	| while read file ; do ldd $file | sed 's!^!'$file'!' ; done 2>/dev/null \
	| sed -e 's/^\.\///' -e 's/ (0x.*)$//' -e 's/ => \(.*\)$/	\1/' -e 's/ => .*$//' \
	| fgrep -v 'not a dynamic executable' \
	| awk -vOFS='\t' '{print $1,$2,$3 ? $3 : $2}' \
	> "$tmpdir/ldd.lst"

find . -type f -perm +111 \! -name '*.so' \
	| xargs nm -AD 2>/dev/null \
	| egrep ': {8}{1,2} U ' \
	| sed -e 's/:/ /g' -e 's/\.\///' \
	| awk -vOFS='\t' '{print $1,$3}' \
	> "$tmpdir/prog_imp.lst"

find . -type f -perm +111 \! -name '*.so' \
	| xargs nm -AD 2>/dev/null \
	| egrep ':[0-9a-f]{8}{1,2} [BCDGRSTW] ' \
	| sed -e 's/:/ /g' -e 's/\.\///' \
	| awk -vOFS='\t' '{print $1,$4}' \
	> "$tmpdir/prog_exp.lst"

)

find * -type f -name 'lib?*.a' \
	| xargs nm -A \
	| egrep ':[0-9a-f]{8}{1,2} [BCDGRSTW] ' \
	| sed 's/:/ /g' \
	| awk -vOFS='\t' '{print gensub("^[^ ]*/","",1,$1),$2,$5}' \
	> "$tmpdir/stlib_exp.lst"

find * -type f -name 'lib?*.so' \
	| xargs nm -AD \
	| egrep ':[0-9a-f]{8}{1,2} [BCDGRSTW] ' \
	| sed 's/:/ /g' \
	| awk -vOFS='\t' '{print gensub("^[^ ]*/","",1,$1),$4}' \
	> "$tmpdir/shlib_exp.lst"

find * -type f -name '*.o' \
	| xargs nm -A \
	| egrep ':[0-9a-f]{8}{1,2} [BCDGRSTW] ' \
	| sed 's/:/ /g' \
	| awk -vOFS='\t' '{print $1,$4}' \
	> "$tmpdir/obj_exp.lst"

find * -type f -name 'lib?*.a' \
	| xargs nm -A \
	| egrep ': {8}{1,2} U ' \
	| sed 's/:/ /g' \
	| awk -vOFS='\t' '{print gensub("^[^ ]*/","",1,$1),$2,$4}' \
	> "$tmpdir/stlib_imp.lst"

find * -type f -name 'lib?*.so' \
	| xargs nm -AD \
	| egrep ': {8}{1,2} U ' \
	| sed 's/:/ /g' \
	| awk -vOFS='\t' '{print gensub("^[^ ]*/","",1,$1),$3}' \
	> "$tmpdir/shlib_imp.lst"

find * -type f -name '*.o' \
	| xargs nm -A \
	| egrep ': {8}{1,2} U ' \
	| sed 's/:/ /g' \
	| awk -vOFS='\t' '{print $1,$3}' \
	> "$tmpdir/obj_imp.lst"

libs=`awk '{print $3}' "$tmpdir/ldd.lst" | uniq | sort | uniq`

nm -AD $libs \
	| egrep ':[0-9a-f]{8}{1,2} [TWDRC] ' \
	| sed 's/:/ /g' \
	| awk -vOFS='\t' '{print gensub("^[^ ]*/","",1,$1),$4}' \
	> "$tmpdir/libs.lst"

cat > "$tmpdir/ansi.lst" <<EOF
abort
asctime
atexit
atof
atoi
atol
bsearch
calloc
clearerr
ctime
exit
fclose
feof
ferror
fflush
fgetc
fgetpos
fgets
fopen
fprintf
fputc
fputs
fread
free
freopen
fscanf
fseek
fsetpos
ftell
fwrite
getc
getchar
getenv
gets
isalnum
isalpha
isdigit
islower
isspace
ldiv
localtime
longjmp
malloc
memcmp
memcpy
memmove
memset
perror
printf
putc
putchar
puts
qsort
rand
realloc
remove
rename
rewind
scanf
setbuf
setjmp
setvbuf
signal
sprintf
srand
sscanf
stderr
stdin
stdout
strcat
strchr
strcmp
strcpy
strcspn
strerror
strftime
strlen
strncat
strncmp
strncpy
strpbrk
strrchr
strspn
strstr
strtod
strtok
strtol
time
tmpfile
tmpnam
tolower
toupper
ungetc
vfprintf
vsprintf
fgetpos64
fopen64
freopen64
fsetpos64
tmpfile64
EOF

dropdb "$dbname"
createdb "$dbname"

psql -n -q -d "$dbname" << EOF

-- ----------------------------------------------------------------------

CREATE TABLE stlib_exp (
	library VARCHAR(80) NOT NULL,
	object VARCHAR(40) NOT NULL,
	symbol VARCHAR(256) NOT NULL
	) ;

\copy stlib_exp FROM '$tmpdir/stlib_exp.lst'

CREATE TABLE shlib_exp (
	library VARCHAR(80) NOT NULL,
	symbol VARCHAR(256) NOT NULL
	) ;

\copy shlib_exp FROM '$tmpdir/shlib_exp.lst'

CREATE TABLE obj_exp (
	object VARCHAR(100) NOT NULL,
	symbol VARCHAR(256) NOT NULL
	) ;

\copy obj_exp FROM '$tmpdir/obj_exp.lst'

CREATE TABLE stlib_imp (
	library VARCHAR(80) NOT NULL,
	object VARCHAR(40) NOT NULL,
	symbol VARCHAR(256) NOT NULL
	) ;

\copy stlib_imp FROM '$tmpdir/stlib_imp.lst'

CREATE TABLE shlib_imp (
	library VARCHAR(80) NOT NULL,
	symbol VARCHAR(256) NOT NULL
	) ;

\copy shlib_imp FROM '$tmpdir/shlib_imp.lst'

CREATE TABLE obj_imp (
	object VARCHAR(100) NOT NULL,
	symbol VARCHAR(256) NOT NULL
	) ;

\copy obj_imp FROM '$tmpdir/obj_imp.lst'

CREATE TABLE prog_imp (
	program VARCHAR(80) NOT NULL,
	symbol VARCHAR(256) NOT NULL
	) ;

\copy prog_imp FROM '$tmpdir/prog_imp.lst'

CREATE TABLE prog_exp (
	program VARCHAR(80) NOT NULL,
	symbol VARCHAR(256) NOT NULL
	) ;

\copy prog_exp FROM '$tmpdir/prog_exp.lst'

CREATE TABLE libs (
	library VARCHAR(80) NOT NULL,
	symbol VARCHAR(1000) NOT NULL
	) ;

\copy libs FROM '$tmpdir/libs.lst'

CREATE TABLE ldd (
	program VARCHAR(80) NOT NULL,
	library VARCHAR(256) NOT NULL,
	path VARCHAR(256)
	) ;

\copy ldd FROM '$tmpdir/ldd.lst'

CREATE TABLE ansi (
	symbol VARCHAR(256) NOT NULL
	) ;

\copy ansi FROM '$tmpdir/ansi.lst'

-- ----------------------------------------------------------------------

SELECT DISTINCT library, symbol
	INTO TABLE lib_exp
	FROM stlib_exp
UNION
SELECT DISTINCT library, symbol
	FROM shlib_exp ;

CREATE TABLE duplicates AS
	SELECT DISTINCT symbol
	FROM lib_exp
	GROUP BY symbol
	HAVING COUNT(*) > 1 ;

CREATE TABLE duplicates2 AS
	SELECT *
	FROM lib_exp
	WHERE symbol IN (
		SELECT symbol
		FROM duplicates
	) ;

SELECT DISTINCT library, symbol
	INTO TABLE lib_imp
	FROM stlib_imp
UNION
SELECT DISTINCT library, symbol
	FROM shlib_imp ;

CREATE TABLE imports AS
	SELECT a.library, a.symbol
	FROM lib_imp a
	WHERE NOT EXISTS (
		SELECT b.library, b.symbol
		FROM lib_exp b
		WHERE b.symbol = a.symbol
		AND b.library = a.library
	) ;

CREATE TABLE defined AS
	SELECT DISTINCT symbol
	FROM lib_exp ;

CREATE TABLE used AS
	SELECT DISTINCT symbol
	FROM imports ;

CREATE TABLE undefined AS
	SELECT symbol
	FROM used u
	WHERE NOT EXISTS (
		SELECT *
		FROM defined d
		WHERE d.symbol = u.symbol
	) ;

SELECT symbol
INTO TABLE undefined_1
	FROM undefined
EXCEPT
SELECT b.symbol
	FROM undefined a, libs b
	WHERE a.symbol = b.symbol ;

CREATE TABLE undefined_2 AS
	SELECT i.symbol, i.object, i.library
	FROM stlib_imp i, undefined_1 u
	WHERE i.symbol = u.symbol ;

CREATE TABLE depends AS
	SELECT	i.library AS im_lib,
		i.symbol AS symbol,
		e.library AS ex_lib
	FROM imports i, lib_exp e
	WHERE i.symbol = e.symbol ;

CREATE TABLE lib_deps AS
	SELECT DISTINCT im_lib, ex_lib
	FROM depends
	WHERE im_lib <> ex_lib ;

CREATE TABLE lib_deps_1 AS
	SELECT	a.im_lib,
		a.ex_lib AS in_lib,
		b.ex_lib
	FROM lib_deps a, lib_deps b
	WHERE a.ex_lib = b.im_lib ;

CREATE TABLE lib_deps_2 AS
	SELECT	a.im_lib,
		a.in_lib AS in1_lib,
		a.ex_lib AS in2_lib,
		b.ex_lib
	FROM lib_deps_1 a, lib_deps b
	WHERE a.ex_lib = b.im_lib
	AND a.im_lib <> a.ex_lib ;

SELECT im_lib, ex_lib
INTO TABLE lib_deps_trans
	FROM lib_deps
UNION
SELECT im_lib, ex_lib
	FROM lib_deps_1
UNION
SELECT im_lib, ex_lib
	FROM lib_deps_2 ;

CREATE TABLE prog_libs AS
SELECT DISTINCT a.program, b.library
FROM prog_imp a, lib_exp b
WHERE a.symbol = b.symbol ;

SELECT DISTINCT a.symbol
INTO TABLE libc
	FROM prog_imp a, libs b
	WHERE a.symbol = b.symbol
	AND b.library = 'libc.so.6' 
UNION
	SELECT DISTINCT a.symbol
	FROM imports a, libs b
	WHERE a.symbol = b.symbol
	AND b.library = 'libc.so.6' ;

SELECT symbol
INTO nonansi
	FROM libc
	WHERE symbol !~ '_.*'
EXCEPT
SELECT symbol
	FROM ansi ;

CREATE TABLE nonansi_progs AS
	SELECT a.program, a.symbol
	FROM prog_imp a, nonansi b
	WHERE a.symbol = b.symbol ;

CREATE TABLE nonansi_libs AS
	SELECT a.library, a.symbol
	FROM imports a, nonansi b
	WHERE a.symbol = b.symbol ;

CREATE TABLE nonansi_prog_counts AS
	SELECT symbol, COUNT(*)
	FROM nonansi_progs
	GROUP BY symbol ;

CREATE TABLE nonansi_lib_counts AS
	SELECT symbol, COUNT(*)
	FROM nonansi_libs
	GROUP BY symbol ;

SELECT symbol
INTO TABLE nonansi_counts
	FROM nonansi_prog_counts
UNION
SELECT symbol
	FROM nonansi_lib_counts ;

ALTER TABLE nonansi_counts
	ADD COLUMN progs INTEGER ;

ALTER TABLE nonansi_counts
	ADD COLUMN libs INTEGER ;

UPDATE nonansi_counts
	SET progs = 0, libs = 0 ;

UPDATE nonansi_counts
	SET progs = b.count
	FROM nonansi_prog_counts b
	WHERE nonansi_counts.symbol = b.symbol ;

UPDATE nonansi_counts
	SET libs = c.count
	FROM nonansi_lib_counts c
	WHERE nonansi_counts.symbol = c.symbol;

-- SELECT a.symbol, a.program
-- 	FROM prog_imp a, nonansi_prog_counts b
-- 	WHERE a.symbol = b.symbol
-- 	AND a.program NOT LIKE 'bin/%'
-- 	ORDER BY b.count DESC, b.symbol ;

-- SELECT symbol, library
-- 	FROM duplicates2
-- 	ORDER BY symbol ;

-- SELECT a.im_lib, a.ex_lib
-- 	FROM lib_deps a, lib_deps b
-- 	WHERE	a.ex_lib = b.im_lib
-- 	AND	b.ex_lib = a.im_lib ;

-- SELECT * FROM lib_deps_2
-- 	WHERE im_lib = ex_lib ;

-- SELECT * FROM lib_deps_1
-- 	WHERE im_lib = ex_lib ;

-- SELECT im_lib FROM lib_deps_trans
-- 	WHERE im_lib = ex_lib ;

-- SELECT a.program, a.library
-- FROM ldd a
-- WHERE a.library LIKE 'libgrass_%.so'
-- AND a.library NOT IN (
-- SELECT b.library
-- FROM prog_libs b
-- WHERE b.program = a.program
-- ) ;

-- ----------------------------------------------------------------------

EOF

