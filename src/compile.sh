#!/bin/bash -e
KOVE=/soft/libraries/kove/api/current/

CONST="-DCONST=const"

if [[ -e $KOVE ]] ; then
  CFLAGS="-Wall -g -I $KOVE/include $CONST "
  LDFLAGS="-L$KOVE/lib64/ -L . -l kdsa -l uuid -Wl,--rpath=$KOVE/lib64/"

  mpicc kove-mpi.c -fPIC  -shared -o libmpi-xpd.so $CFLAGS $LDFLAGS
  mpicc tools/mpio-xpd-format.c -o mpio-xpd-format.exe $CFLAGS $LDFLAGS
  mpicc tools/xpd-copy.c -o mpi-xpd-copy.exe $CFLAGS $LDFLAGS
fi

echo "Building dummy interface"
echo "Due to licensing issues, this not available, yet"

exit 0

CFLAGS="-Wall -g -I dummy/ -I /usr/lib/openmpi/ $CONST"
LDFLAGS="-L . -l kdsa-dummy -Wl,--rpath=$PWD/"

gcc -shared dummy/kdsa-dummy.c -fPIC -o libkdsa-dummy.so $CFLAGS

mpicc kove-mpi.c $CONST -fPIC  -shared -o libmpi-xpd-dummy.so  $CFLAGS $LDFLAGS
mpicc tools/mpio-xpd-format.c -o mpio-xpd-format-dummy.exe  $CFLAGS $LDFLAGS
mpicc tools/xpd-copy.c -o mpi-xpd-copy-dummy.exe  -l mpi-xpd-dummy  $CFLAGS $LDFLAGS
