BASEDIR:=~
all:
	gcc -o ibecho ibecho.c -libverbs

ibtcp:
	gcc -Wall -fPIC -c ibtcp.c -libverbs
	gcc -shared -Wl,-soname,libibtcp.so.1 -o libibtcp.so.1.0 ibtcp.o
	mv libibtcp.so.1.0 $(BASEDIR)/lib/
	ln -sf $(BASEDIR)/lib/libibtcp.so.1.0 $(BASEDIR)/lib/libibtcp.so.1
	ln -sf $(BASEDIR)/lib/libibtcp.so.1.0 $(BASEDIR)/lib/libibtcp.so
	cp ibtcp.h $(BASEDIR)/usr/include/ibtcp.h

practice: ibtcp
	gcc -o structs structs.c -L. -libtcp
.PHONY: clean

clean: 
	rm ibecho
	rm structs
	rm *.o
