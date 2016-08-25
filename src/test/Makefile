CFLAGS=-Wall -std=c99 -g3 
# -DDEBUG
CC=mpicc

all: datatype.exe

datatype.exe: datatypes-supported.txt datatype.c datatype-main.c Makefile datatype.h datatype-proc.c
	${CC} ${CFLAGS} ${shell cat datatypes-supported.txt} datatype.c datatype-proc.c datatype-main.c -o datatype.exe

datatypes-supported.txt: check-available-combiner.sh
	./check-available-combiner.sh > datatypes-supported.txt

clean:
	rm datatype.exe
	datatypes-supported.txt
