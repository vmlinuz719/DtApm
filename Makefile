dtapm: apm.c apmsubr.c apm-proto.h pathnames.h
	cc apm.c apmsubr.c -I/usr/local/include/ -I/usr/X11R6/include/ -L/usr/local/lib -L/usr/X11R6/lib -lXm -lXt -lX11 -o dtapm

clean:
	rm -f dtapm *.o
