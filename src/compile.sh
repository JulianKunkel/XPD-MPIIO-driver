#!/bin/bash -e
CONST="-DCONST=const"
#CONST=""

gcc -Wall -shared kdsa-dummy.c -fPIC -o libkdsa.so -I . -g
mpicc $(cat available.txt) -I /usr/lib/openmpi/ -Wall kove-mpi.c $CONST -fPIC  -shared -o libmpi-xpd-shmio-dummy.so -I . -g -L . -lkdsa -Wl,--rpath=$PWD/ datatype.c datatype-proc.c -g

mpicc tools/mpio-xpd-format.c -o mpio-xpd-format-dummy -I . -g -L . -lkdsa -Wl,--rpath=$PWD/ 
mpicc tools/xpd-copy.c -o mpi-xpd-copy-dummy  -I .  -l mpi-xpd-shmio-dummy   -g -L . -lkdsa -Wl,--rpath=$PWD/ -l mpi

rm testfile  test.nc testfile-orig || echo ""

export MPI_XPD_DEBUG=x
unset MPI_XPD_DEBUG
#CALL="/home/julian/Dokumente/DKRZ/wr-git/bull-io/netcdf-benchmark/src/benchtool-int -d=4:4:4:4 -c=1:1:1:1 -t=coll -w -r --verify -f="
# CALL="mpiexec -np 4 /home/julian/Dokumente/DKRZ/wr-git/bull-io/netcdf-benchmark/src/benchtool-int -d=4:4:4:4 -n=2 -p=2 -c=1:1:1:1 -t=ind -w -r --verify -f="
CALL="mpiexec -np 4 /home/julian/Dokumente/DKRZ/wr-git/bull-io/netcdf-benchmark/src/benchtool-int -d=4:10:10:10 -n=2 -p=2 -c=1:5:5:5 -t=ind -w -r --verify -f="
CALL="mpiexec -np 2 /home/julian/Dokumente/DKRZ/wr-git/bull-io/netcdf-benchmark/src/benchtool-int -n=2 -p=1 -c=auto  -d=1:10:10:10 -t=ind -w -r --verify -f="

${CALL}testfile-orig
LD_PRELOAD=./libmpi-xpd-shmio-dummy.so ${CALL}xpd:testfile
./mpi-xpd-copy-dummy xpd:testfile test.nc

cmp test.nc testfile-orig && exit 0

hd test.nc > test.nc.hd
hd testfile-orig > testfile-orig.hd

diff -u test.nc.hd testfile-orig.hd || echo 0

h5dump test.nc > test.nc.dmp
h5dump testfile-orig > test-orig.nc.dmp

diff -u test.nc.dmp test-orig.nc.dmp || echo 0


