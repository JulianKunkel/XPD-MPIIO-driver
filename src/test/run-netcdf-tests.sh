#!/bin/bash

NC_BENCH=/home/julian/Dokumente/DKRZ/wr-git/bull-io/netcdf-benchmark/src/
FILE_XPD=/dev/shm/testfile
DUMMY=-dummy
FILE_LOCAL=testfile.nc
DEBUG="dump" # could be dump to dump the files

# export MPI_XPD_DEBUG=x
# export MPI_XPD_DEBUG=SPIN

echo "This script tests various configurations of the netcdf-benchmark and compares the results"

function run(){
  TEST="$1"
  TYP="${2}"
  CALL="${3} ${NC_BENCH}${4}"
  echo -n "$TEST "

  echo "${CALL}${FILE_LOCAL}" > $TEST-orig.out
  ${CALL}${FILE_LOCAL} >> $TEST-orig.out 2>&1

  if [[ "$?" != 0 ]] ; then
    return 1
  fi

  if [[ "$TYP" == "w" ]] ; then
    ./mpi-xpd-format${DUMMY}.exe ${FILE_XPD} > /dev/null
  fi
  echo "LD_PRELOAD=./libmpi-xpd${DUMMY}.so ${CALL}xpd:${FILE_XPD}" > $TEST-xpd.out
  LD_PRELOAD=./libmpi-xpd${DUMMY}.so ${CALL}xpd:${FILE_XPD}  >> $TEST-xpd.out 2>&1

  if [[ "$?" != 0 ]] ; then
    return 1
  fi

  if [[ "$TYP" == "r" ]] ; then
    rm $TEST-xpd.out $TEST-orig.out
    return 0
  fi

  ./mpi-xpd-copy${DUMMY}.exe xpd:${FILE_XPD} xpd.nc >> $TEST-xpd.out

  cmp xpd.nc ${FILE_LOCAL} > $TEST-cmp.out 2>&1
  if [[ "$?" == 0 ]] ; then
    rm $TEST-xpd.out $TEST-orig.out $TEST-cmp.out xpd.nc
    return 0
  fi

  if [[ "$DEBUG" == "dump" ]] ; then
    hd ${FILE_LOCAL} > ${FILE_LOCAL}.hd
    hd xpd.nc > xpd.hd
    diff -u xpd.hd ${FILE_LOCAL}.hd > $TEST-diff-hd.txt

    h5dump ${FILE_LOCAL} > ${FILE_LOCAL}.dmp
    h5dump xpd.nc > xpd.nc.dmp
    diff -u xpd.nc.dmp ${FILE_LOCAL}.dmp > $TEST-diff-hdf.txt
  fi
  return 1
}


function runMultiple1(){
  n=$1
  p=$2
  mpi=$3
  size=$4
  extra=$5
  ERROR=0

  for DATATYPE in "int" "double" "byte" ; do

  for COLL in "ind" "coll" ; do

  for ACCESS in "w" "r" ; do

  if [[ "$ACCESS" == "r" ]]; then
    V="--verify"
    if [[ "$SKIP" == "1" ]] ; then
      continue
    fi
  else
    V=""
  fi
  SKIP=0

  run "$DATATYPE-$n-$p-$COLL-$ACCESS$extra" "$ACCESS" "$mpi" "benchtool-$DATATYPE -n=$n -p=$p -d=$size $extra  -t=$COLL -$ACCESS $V -f="
  if [[ $? != 0 ]] ; then
    echo "ERR"
    ERROR=1
    SKIP=1
  else
    echo "OK"
  fi

  done
  done
  done
  return $ERROR
}


function runMultiple2(){
  n=$1
  p=$2
  mpi=$3
  size=$4
  extra=$5
  ERROR=0

  for DATATYPE in "int" "double" "byte" ; do

  COLL="coll"

  for ACCESS in "w" "r" ; do

  if [[ "$ACCESS" == "r" ]]; then
    V="--verify"
    if [[ "$SKIP" == "1" ]] ; then
      continue
    fi
  else
    V=""
  fi
  SKIP=0

  run "$DATATYPE-$n-$p-$COLL-$ACCESS$extra-u" "$ACCESS" "$mpi" "benchtool-$DATATYPE -n=$n -p=$p -d=$size -u $extra  -t=$COLL -$ACCESS $V -f="
  if [[ $? != 0 ]] ; then
    echo "ERR"
    ERROR=1
    SKIP=1
  else
    echo "OK"
  fi

  done
  done
  return $ERROR
}

function runMultiple(){
	runMultiple1 "$1" "$2" "$3" "$4" "$5" "$6" "$7"
	runMultiple2 "$1" "$2" "$3" "$4" "$5" "$6" "$7"
}


runMultiple 1 1 "" 1:1:1:1 ""
if [[ $? != 0 ]] ; then
  exit 1
fi
runMultiple 1 1 "" 2:2:2:2 "-c=auto"
if [[ $? != 0 ]] ; then
  exit 1
fi

runMultiple 2 1 "mpiexec -np 2" 1:10:10:10 ""
runMultiple 1 2 "mpiexec -np 2" 1:10:10:10 ""
runMultiple 5 2 "mpiexec -np 10" 1:10:10:10 ""
runMultiple 2 5 "mpiexec -np 10" 1:10:10:10 ""
if [[ $? != 0 ]] ; then
  exit 1
fi

runMultiple 2 1 "mpiexec -np 2" 1:10:10:10 "-c=auto"
runMultiple 1 2 "mpiexec -np 2" 1:10:10:10 "-c=auto"
runMultiple 5 2 "mpiexec -np 10" 1:10:10:10 "-c=auto"
runMultiple 2 5 "mpiexec -np 10" 1:10:10:10 "-c=auto"
if [[ $? != 0 ]] ; then
  exit 1
fi


runMultiple 2 1 "mpiexec -np 2" 10:100:100:100 ""
runMultiple 1 2 "mpiexec -np 2" 10:100:100:100 ""
runMultiple 5 2 "mpiexec -np 10" 10:100:100:100 ""
runMultiple 2 5 "mpiexec -np 10" 10:100:100:100 ""
runMultiple 2 1 "mpiexec -np 2" 10:100:100:100 "-c=auto"
runMultiple 1 2 "mpiexec -np 2" 10:100:100:100 "-c=auto"
runMultiple 5 2 "mpiexec -np 10" 10:100:100:100 "-c=auto"
runMultiple 2 5 "mpiexec -np 10" 10:100:100:100 "-c=auto"
