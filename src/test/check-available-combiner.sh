#!/bin/bash

CC=mpicc

SUPPORTED=""
for C in MPI_COMBINER_NAMED MPI_COMBINER_DUP MPI_COMBINER_CONTIGUOUS MPI_COMBINER_VECTOR MPI_COMBINER_HVECTOR_INTEGER MPI_COMBINER_HVECTOR MPI_TYPE_CREATE_HVECTOR MPI_COMBINER_INDEXED MPI_COMBINER_HINDEXED_INTEGER MPI_TYPE_HINDEXED MPI_COMBINER_HINDEXED MPI_TYPE_HINDEXED MPI_TYPE_CREATE_HINDEXED MPI_COMBINER_INDEXED_BLOCK MPI_TYPE_CREATE_INDEXED_BLOCK MPI_COMBINER_STRUCT_INTEGER MPI_TYPE_STRUCT MPI_COMBINER_STRUCT MPI_TYPE_STRUCT MPI_TYPE_CREATE_STRUCT MPI_COMBINER_SUBARRAY MPI_TYPE_CREATE_SUBARRAY MPI_COMBINER_DARRAY MPI_TYPE_CREATE_DARRAY MPI_COMBINER_F90_REAL MPI_TYPE_CREATE_F90_REAL MPI_COMBINER_F90_COMPLEX MPI_TYPE_CREATE_F90_COMPLEX MPI_COMBINER_F90_INTEGER MPI_TYPE_CREATE_F90_INTEGER MPI_COMBINER_RESIZED MPI_TYPE_CREATE_RESIZED ; do

echo "
#include <mpi.h>
int main(){
  int c;
  if (c == $C){
    return 0;
  }
  return 1;
}" > check.c
  $CC check.c 2>/dev/null && SUPPORTED="-DS$C $SUPPORTED"
done

for T in  MPI_INTEGER1 MPI_INTEGER2 MPI_INTEGER4 MPI_REAL2 MPI_REAL4 MPI_REAL8 MPI_LONG_LONG_INT ; do
echo "
#include <mpi.h>
int main(){
  $T;
  return 1;
}" > check.c
$CC check.c 2>/dev/null && SUPPORTED="-DS$T $SUPPORTED"
done

echo $SUPPORTED
rm check.c
