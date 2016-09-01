#include <mpi.h>
#include <stdio.h>
#include <assert.h>

#include "datatype.h"

//@see https://www.mpi-forum.org/docs/mpi-2.2/mpi22-report/node82.htm
//TODO: Check MPI-1 compatibility

static void mpix_decode_primitive(MPI_Datatype typ){
  #ifdef SMPI_COMBINER_NAMED
  printType("NAMED(");
  int size;
  MPI_Type_size( typ, &size );
  printType("size=%d,", size);
  if(typ == MPI_LB){
    printType("LB)");
    return;
  }
  if(typ == MPI_UB){
    printType("UB)");
    return;
  }
  if(typ == MPI_CHAR){
    printType("CHAR)");
    return;
  }
  if(typ == MPI_SHORT){
    printType("SHORT)");
    return;
  }
  if(typ == MPI_INT){
    printType("INT)");
    return;
  }
  if(typ == MPI_LONG){
    printType("LONG)");
    return;
  }
  if(typ == MPI_UNSIGNED_CHAR){
    printType("UNSIGNED_CHAR)");
    return;
  }
  if(typ == MPI_UNSIGNED_SHORT){
    printType("UNSIGNED_SHORT)");
    return;
  }
  if(typ == MPI_UNSIGNED){
    printType("UNSIGNED)");
    return;
  }
  if(typ == MPI_UNSIGNED_LONG){
    printType("UNSIGNED_LONG)");
    return;
  }
  if(typ == MPI_FLOAT){
    printType("FLOAT)");
    return;
  }
  if(typ == MPI_DOUBLE){
    printType("DOUBLE)");
    return;
  }
  if(typ == MPI_LONG_DOUBLE){
    printType("LONG_DOUBLE)");
    return;
  }
  if(typ == MPI_BYTE){
    printType("BYTE)");
    return;
  }
  if(typ == MPI_PACKED){
    printType("PACKED)");
    return;
  }
  if(typ == MPI_INTEGER){
    printType("INTEGER)");
    return;
  }
  if(typ == MPI_REAL){
    printType("REAL)");
    return;
  }
  if(typ == MPI_DOUBLE_PRECISION){
    printType("DOUBLE_PRECISION)");
    return;
  }
  if(typ == MPI_COMPLEX){
    printType("COMPLEX)");
    return;
  }
  if(typ == MPI_LOGICAL){
    printType("LOGICAL)");
    return;
  }
  if(typ == MPI_CHARACTER){
    printType("CHARACTER)");
    return;
  }
  if(typ == MPI_BYTE){
    printType("BYTE)");
    return;
  }
  if(typ == MPI_PACKED){
    printType("PACKED)");
    return;
  }
  #ifdef SMPI_INTEGER1
  if(typ == MPI_INTEGER1){
    printType("INTEGER1)");
    return;
  }
  #endif
  #ifdef SMPI_INTEGER2
  if(typ == MPI_INTEGER2){
    printType("INTEGER2)");
    return;
  }
  #endif
  #ifdef SMPI_INTEGER4
  if(typ == MPI_INTEGER4){
    printType("INTEGER4)");
    return;
  }
  #endif
  #ifdef SMPI_REAL2
  if(typ == MPI_REAL2){
    printType("REAL2)");
    return;
  }
  #endif
  #ifdef SMPI_REAL4
  if(typ == MPI_REAL4){
    printType("REAL4)");
    return;
  }
  #endif
  #ifdef SMPI_REAL8
  if(typ == MPI_REAL8){
    printType("REAL8)");
    return;
  }
  #endif
  #ifdef SMPI_LONG_LONG_INT
  if(typ == MPI_LONG_LONG_INT){
    printType("LONG_LONG_INT)");
    return;
  }
  #endif
  printf("Error: unsupported basic data type\n");
  #endif
}

static void printSizeExt(MPI_Datatype typ){
  MPI_Aint extent;
  int size;
  MPI_Type_extent( typ, &extent);
  MPI_Type_size( typ, &size );
  printType(",size=%d,extent=%zu)", size, (size_t) extent);
}

void mpix_decode_datatype(MPI_Datatype typ){
  int ret;
  int num_integers, num_addresses, num_datatypes, combiner;
  ret = MPI_Type_get_envelope(typ, & num_integers, & num_addresses, & num_datatypes, & combiner);
  CHECK_RET(ret)
  //debug("%d %d %d %d", num_integers, num_addresses, num_datatypes, combiner);

  if( combiner == MPI_COMBINER_NAMED ){
    mpix_decode_primitive(typ);
    return;
  }

  int integers[num_integers];
  MPI_Aint addresses[num_addresses];
  MPI_Datatype datatypes[num_datatypes];

  ret = MPI_Type_get_contents(typ, num_integers, num_addresses, num_datatypes, integers, addresses, datatypes);
  CHECK_RET(ret)
  // for(int i=0; i < num_integers; i++){
  //   debug("Count: %d", integers[i]);
  // }
  // for(int i=0; i < num_addresses; i++){
  //   debug("Address: %zu", (size_t) addresses[i]);
  // }

  switch(combiner){
  #ifdef SMPI_COMBINER_DUP
  case(MPI_COMBINER_DUP):{
    printType("DUP(typ=");
    mpix_decode_datatype(datatypes[0]);
    break;
  }
  #endif
  #ifdef SMPI_COMBINER_CONTIGUOUS
  case(MPI_COMBINER_CONTIGUOUS):{
    printType("CONTIGUOUS(count=%d,typ=", integers[0]);
    mpix_decode_datatype(datatypes[0]);
    break;
  }
  #endif
  #ifdef SMPI_COMBINER_VECTOR
  case(MPI_COMBINER_VECTOR):{
    printType("VECTOR(count=%d,blocklength=%d,stride=%d,typ=", integers[0], integers[1], integers[2]);
    mpix_decode_datatype(datatypes[0]);
    break;
  }
  #endif
  #ifdef SMPI_COMBINER_HVECTOR_INTEGER
  case(MPI_COMBINER_HVECTOR_INTEGER):
  #endif
  #ifdef SMPI_TYPE_CREATE_HVECTOR
  case(MPI_TYPE_CREATE_HVECTOR):
  #endif
  #ifdef SMPI_COMBINER_HVECTOR
  case(MPI_COMBINER_HVECTOR):
  #endif
  {
    printType("HVECTOR(count=%d,blocklength=%d,stride=%ld,typ=", integers[0], integers[1], addresses[0]);
    mpix_decode_datatype(datatypes[0]);
    break;
  }
  #ifdef SMPI_COMBINER_INDEXED
  case(MPI_COMBINER_INDEXED):{
    printType("INDEXED(count=%d,blocklength=[", integers[0]);

    for(int i=1; i <= integers[0]; i++){
      if( i != 1) printType(";");
      printType("%d", integers[i]);
    }
    printType("], displacement=[");
    for(int i=integers[0]+1; i <= 2*integers[0]; i++){
      if( i != 1) printType(";");
      printType("%d", integers[i]);
    }
    printType("],typ=");
    mpix_decode_datatype(datatypes[0]);
    break;
  }
  #endif
  #ifdef SMPI_COMBINER_HINDEXED_INTEGER
  case(MPI_COMBINER_HINDEXED_INTEGER):
  #endif
  #ifdef SMPI_TYPE_HINDEXED
  case(MPI_TYPE_HINDEXED):
  #endif
  #ifdef SMPI_COMBINER_HINDEXED
  case(MPI_COMBINER_HINDEXED):
  #endif
  #ifdef SMPI_TYPE_CREATE_HINDEXED
  case(MPI_TYPE_CREATE_HINDEXED):
  #endif
  #ifdef SMPI_TYPE_HINDEXED
  case(MPI_TYPE_HINDEXED):
  #endif
  {
    printType("HINDEXED(count=%d,blocklength=[", integers[0]);

    for(int i=1; i <= integers[0]; i++){
      if( i != 1) printType(";");
      printType("%d", integers[i]);
    }
    printType("],displacement=[");
    for(int i=0; i < integers[0]; i++){
      if( i != 0) printType(";");
      printType("%ld", addresses[i]);
    }
    printType("],typ=");
    mpix_decode_datatype(datatypes[0]);
    break;
  }

  #ifdef SMPI_TYPE_CREATE_INDEXED_BLOCK
  case(MPI_TYPE_CREATE_INDEXED_BLOCK):
  #endif
  #ifdef SMPI_COMBINER_INDEXED_BLOCK
  case(MPI_COMBINER_INDEXED_BLOCK):
  #endif
  {
    printType("INDEXED_BLOCK(count=%d,blocklength=%d,displacement=[", integers[0], integers[1]);
    for(int i=2; i <= integers[0] + 1; i++){
      if( i != 2) printType(";");
      printType("%ld", addresses[i]);
    }
    printType("],typ=");
    mpix_decode_datatype(datatypes[0]);
    break;
  }
  #ifdef SMPI_COMBINER_STRUCT_INTEGER
  case(MPI_COMBINER_STRUCT_INTEGER):
  #endif
  #ifdef SMPI_TYPE_STRUCT
  case(MPI_TYPE_STRUCT):
  #endif
  #ifdef SMPI_COMBINER_STRUCT
  case(MPI_COMBINER_STRUCT):
  #endif
  #ifdef SMPI_TYPE_CREATE_STRUCT
  case(MPI_TYPE_CREATE_STRUCT):
  #endif
  {
    printType("STRUCT(count=%d,blocklength=[", integers[0]);

    for(int i=1; i <= integers[0]; i++){
      if( i != 1) printType(";");
      printType("%d", integers[i]);
    }
    printType("],displacement=[");
    for(int i=0; i < integers[0]; i++){
      if( i != 0) printType(";");
      printType("%ld", addresses[i]);
    }
    printType("],typ=[");
    for(int i=0; i < integers[0]; i++){
      if( i != 0) printType(";");
      mpix_decode_datatype(datatypes[i]);
    }
    printType("]");
    break;
  }
  #ifdef SMPI_TYPE_CREATE_SUBARRAY
  case(MPI_TYPE_CREATE_SUBARRAY):
  #endif
  #ifdef SMPI_COMBINER_SUBARRAY
  case(MPI_COMBINER_SUBARRAY):
  #endif
  {
    printType("SUBARRAY(ndims=%d,size=[", integers[0]);
    for(int i=1; i <= integers[0]; i++){
      if( i != 1) printType(";");
      printType("%d", integers[i]);
    }
    printType("],subsize=[");
    for(int i=integers[0] + 1; i <= 2*integers[0]; i++){
      if( i != integers[0] + 1) printType(";");
      printType("%d", integers[i]);
    }
    printType("],starts=[");
    for(int i=2*integers[0] + 1; i <= 3*integers[0]; i++){
      if( i != 2*integers[0] + 1) printType(";");
      printType("%d", integers[i]);
    }
    printType("],order=%d,typ=", integers[3*integers[0]+1]);
    mpix_decode_datatype(datatypes[0]);
    break;
  }
  #ifdef SMPI_COMBINER_DARRAY
  case(MPI_COMBINER_DARRAY):
  #endif
  #ifdef SMPI_TYPE_CREATE_DARRAY
  case(MPI_TYPE_CREATE_DARRAY):{
  #endif
  {
    int ndims = integers[2];
    printType("DARRAY(size=%d,rank=%d,ndims=%d,gsizes=[", integers[0], integers[1], ndims);
    int start;
    start = 3;
    for(int i=start; i < ndims + start; i++){
      if( i != start) printType(";");
      printType("%d", integers[i]);
    }
    printType("],distribs=[");
    start += ndims;
    for(int i=start; i < ndims + start; i++){
      if( i != start) printType(";");
      printType("%d", integers[i]);
    }
    printType("],dargs=[");
    start += ndims;
    for(int i=start; i < ndims + start; i++){
      if( i != start) printType(";");
      printType("%d", integers[i]);
    }
    printType("],psizes=[");
    start += ndims;
    for(int i=start; i < ndims + start; i++){
      if( i != start) printType(";");
      printType("%d", integers[i]);
    }
    printType("],order=%d,typ=", integers[4*ndims+3]);
    mpix_decode_datatype(datatypes[0]);
    break;
  }
  #ifdef SMPI_COMBINER_F90_REAL
  case(MPI_COMBINER_F90_REAL):{
    printType("F90_REAL(p=%d,r=%d)", integers[0], integers[1]);
    break;
  }
  #endif
  #ifdef SMPI_TYPE_CREATE_F90_REAL
  case(MPI_TYPE_CREATE_F90_REAL):{
    printType("F90_REAL(p=%d,r=%d)", integers[0], integers[1]);
    break;
  }
  #endif
  #ifdef SMPI_COMBINER_F90_COMPLEX
  case(MPI_COMBINER_F90_COMPLEX):{
    printType("F90_COMPLEX(p=%d,r=%d)", integers[0], integers[1]);
    break;
  }
  #endif
  #ifdef SMPI_TYPE_CREATE_F90_COMPLEX
  case(MPI_TYPE_CREATE_F90_COMPLEX):{
    printType("F90_COMPLEX(p=%d,r=%d)", integers[0], integers[1]);
    break;
  }
  #endif
  #ifdef SMPI_COMBINER_F90_INTEGER
  case(MPI_COMBINER_F90_INTEGER):{
    printType("F90_INTEGER(r=%d)", integers[0]);
    break;
  }
  #endif
  #ifdef SMPI_TYPE_CREATE_F90_INTEGER
  case(MPI_TYPE_CREATE_F90_INTEGER):{
    printType("F90_INTEGER(r=%d)", integers[0]);
    break;
  }
  #endif
  #ifdef SMPI_TYPE_CREATE_RESIZED
  case(MPI_TYPE_CREATE_RESIZED):
  #endif
  #ifdef SMPI_COMBINER_RESIZED
  case(MPI_COMBINER_RESIZED):
  #endif
  {
    printType("RESIZED(lb=%ld,typ=", addresses[0]);
    mpix_decode_datatype(datatypes[0]);
    break;
  }
  default:{
    printf("ERROR Unsupported combiner: %d\n", combiner);
  }
  }
  printSizeExt(typ);
}
