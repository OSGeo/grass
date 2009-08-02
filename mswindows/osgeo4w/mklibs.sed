/Dump of file / {
	s/Dump of file \([^	 ]*\)$/LIBRARY \1/p
  	a\
EXPORTS
}
/[ 	]*ordinal hint/,/^[	]*Summary/ {
 /^[ 	]\+[0-9]\+/ {
   s/^[ 	]\+[0-9]\+[ 	]\+[0-9A-Fa-f]\+[ 	]\+[0-9A-Fa-f]\+[ 	]\+\([^ 	=?]\+\).*$/	\1/p
 }
}
