CONST=-DCONST=const
CFLAGS=-Wall -g -I dummy/ -I /usr/lib/openmpi/ $(CONST) -std=gnu99
LDFLAGS=-L/usr/lib  -l kdsa
LDFLAGS_DUMMY=-L . -l kdsa-dummy -Wl,--rpath=$(PWD)/
CC=mpicc

all: dummy xpd


clean:
	rm *.so *.exe

xpd: libmpi-xpd.so mpi-xpd-format.exe mpi-xpd-copy.exe Makefile

libmpi-xpd.so: kove-mpi.c Makefile  datatype.c datatype-proc.c datatypes-supported.txt
	${CC} kove-mpi.c ${shell cat datatypes-supported.txt} ${CONST} -fPIC   datatype.c datatype-proc.c -shared -o libmpi-xpd.so  ${CFLAGS} ${LDFLAGS}

mpi-xpd-copy.exe: tools/xpd-copy.c libmpi-xpd.so Makefile
	${CC} tools/xpd-copy.c -o mpi-xpd-copy.exe  -l mpi-xpd  ${CFLAGS} ${LDFLAGS}

mpi-xpd-format.exe: tools/mpi-xpd-format.c Makefile
	${CC} tools/mpi-xpd-format.c -o mpi-xpd-format.exe  ${CFLAGS} ${LDFLAGS}


dummy: libkdsa-dummy.so libmpi-xpd-dummy.so mpi-xpd-format-dummy.exe mpi-xpd-copy-dummy.exe Makefile

libkdsa-dummy.so: dummy/kdsa-dummy.c Makefile dummy/kdsa.h  datatype.c datatype-proc.c datatypes-supported.txt
	${CC} -shared ${shell cat datatypes-supported.txt} dummy/kdsa-dummy.c  datatype.c datatype-proc.c -fPIC -o libkdsa-dummy.so $(CFLAGS)

libmpi-xpd-dummy.so: kove-mpi.c libkdsa-dummy.so Makefile  datatype.c datatype-proc.c datatypes-supported.txt
	${CC} kove-mpi.c ${shell cat datatypes-supported.txt} ${CONST} -fPIC   datatype.c datatype-proc.c -shared -o libmpi-xpd-dummy.so  ${CFLAGS} ${LDFLAGS_DUMMY}

mpi-xpd-copy-dummy.exe: tools/xpd-copy.c libmpi-xpd-dummy.so Makefile
	${CC} tools/xpd-copy.c -o mpi-xpd-copy-dummy.exe  -l mpi-xpd-dummy  ${CFLAGS} ${LDFLAGS_DUMMY}

mpi-xpd-format-dummy.exe: tools/mpi-xpd-format.c libkdsa-dummy.so Makefile
	${CC} tools/mpi-xpd-format.c -o mpi-xpd-format-dummy.exe  ${CFLAGS} ${LDFLAGS_DUMMY}

datatypes-supported.txt: check-available-combiner.sh
	./check-available-combiner.sh > datatypes-supported.txt
