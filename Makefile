CFLAGS	=	-g

default:	shpdiff shp2text

shpopen.o:	shpopen.c shapefil.h
	$(CC) $(CFLAGS) -c shpopen.c

dbfopen.o:	dbfopen.c shapefil.h
	$(CC) $(CFLAGS) -c dbfopen.c

shpdiff:	shpdiff.c shpopen.o dbfopen.o
	$(CC) $(CFLAGS) shpdiff.c shpopen.o dbfopen.o $(LINKOPT) -o shpdiff

shp2text:	shp2text.c shpopen.o
	$(CC) $(CFLAGS) shp2text.c shpopen.o dbfopen.o $(LINKOPT) -o shp2text

zip:
	zip -j shp2text.zip Makefile shp2text.c  shpdiff.c dbfopen.c shpopen.c shpdiff shp2text lcc/shp2text.exe

clean:
	rm -f  *.o
	rm -rf *.lo *.la .libs
	rm -f  shpdiff shp2text

install:
	cp shpdiff ~/bin
	cp shp2text ~/bin
