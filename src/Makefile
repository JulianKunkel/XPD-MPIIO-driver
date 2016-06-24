CONST=-DCONST=const
CFLAGS=-Wall -g -I dummy/ -I /usr/lib/openmpi/ $(CONST)
LDFLAGS=-L . -l kdsa-dummy -Wl,--rpath=$(PWD)/
MPICC=mpicc

all: dummy


clean:
	rm *.so *.exe

dummy: libkdsa-dummy.so libmpi-xpd-dummy.so mpio-xpd-format-dummy.exe mpi-xpd-copy-dummy.exe Makefile

libkdsa-dummy.so: dummy/kdsa-dummy.c Makefile dummy/kdsa.h
	gcc -shared dummy/kdsa-dummy.c -fPIC -o libkdsa-dummy.so $(CFLAGS)

libmpi-xpd-dummy.so: kove-mpi.c libkdsa-dummy.so Makefile
	mpicc kove-mpi.c ${CONST} -fPIC  -shared -o libmpi-xpd-dummy.so  ${CFLAGS} ${LDFLAGS}

mpi-xpd-copy-dummy.exe: tools/xpd-copy.c libmpi-xpd-dummy.so Makefile
	mpicc tools/xpd-copy.c -o mpi-xpd-copy-dummy.exe  -l mpi-xpd-dummy  ${CFLAGS} ${LDFLAGS}

mpio-xpd-format-dummy.exe: tools/mpio-xpd-format.c libkdsa-dummy.so Makefile
	mpicc tools/mpio-xpd-format.c -o mpio-xpd-format-dummy.exe  ${CFLAGS} ${LDFLAGS}

