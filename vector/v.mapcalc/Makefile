CC = gcc
#CFLAGS = -g -Wall -Werror -rdynamic
CFLAGS = -O2 -Wall -Werror -rdynamic
LDFLAGS = -ldl -lm #-lefence
YACC = bison
#YACC = bison -t
YACCFLAGS = -d

all: v.mapcalc

v.mapcalc: mapcalc.o yylex.o v.mapcalc.o number.o vector.o map.o any.o \
	plugin.o list.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

v.mapcalc.o: v.mapcalc.tab.c
	$(CC) $(CFLAGS) -o $@ -c $<

v.mapcalc.tab.c v.mapcalc.tab.h: v.mapcalc.y
	$(YACC) $(YACCFLAGS) $<

mapcalc.o: mapcalc.c list.h mapcalc.h number.h map.h plugin.h vector.h any.h
	$(CC) $(CFLAGS) -o $@ -c $<

number.o: number.c list.h mapcalc.h
	$(CC) $(CFLAGS) -o $@ -c $<

map.o: map.c list.h mapcalc.h map.h
	$(CC) $(CFLAGS) -o $@ -c $<

any.o: any.c list.h mapcalc.h any.h
	$(CC) $(CFLAGS) -o $@ -c $<

vector.o: vector.c list.h mapcalc.h vector.h
	$(CC) $(CFLAGS) -o $@ -c $<

plugin.o: plugin.c list.h mapcalc.h
	$(CC) $(CFLAGS) -o $@ -c $<

yylex.o: yylex.c mapcalc.h yylex.h v.mapcalc.tab.h
	$(CC) $(CFLAGS) -o $@ -c $<

list.o: list.c list.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *~ .*~ *.o v.mapcalc.tab.[ch] v.mapcalc
