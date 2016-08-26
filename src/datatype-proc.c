#include <mpi.h>
#include <stdio.h>
#include <assert.h>

#include "datatype.h"

// count == number of repeats for the file type
static int mpix_process_datatype_i(char ** mem_buff, MPI_Datatype mem_type, size_t * file_offset, size_t * bytes_to_access, int count, MPI_Datatype file_type, MPI_Type_decode_func func, void * usr_ptr){
  int ret;
  int num_integers, num_addresses, num_datatypes, combiner;

  int typ_size = 0;
  MPI_Type_size(file_type, & typ_size);
  if(typ_size == 0) return 0;

  MPI_Count typ_lb, typ_extent;

  ret = MPI_Type_get_extent_x(file_type, & typ_lb, & typ_extent );
  assert(ret == MPI_SUCCESS);

  size_t processed;

  // check if we can access a contiguous chunk of data now:
  //ret = MPI_Type_get_envelope(file_type, & num_integers, & num_addresses, & num_datatypes, & combiner);
  mpix_decode_datatype(file_type);
  printf("XX %d %zu %zu bytes_to_access: %zu\n", typ_size, (size_t)  typ_extent, (size_t) typ_lb, *bytes_to_access);
  if( typ_size == typ_extent){
    // one call only
    //if(combiner == MPI_COMBINER_NAMED){
    size_t bytes = ((size_t) typ_size)*count;
    if (*bytes_to_access < bytes) bytes = *bytes_to_access;
    processed = func(usr_ptr, bytes, *mem_buff, *file_offset + typ_lb);
    *file_offset += processed + typ_lb;
    *mem_buff += processed;
    *bytes_to_access -= processed;
    return processed != bytes;
  }
  ret = MPI_Type_get_envelope(file_type, & num_integers, & num_addresses, & num_datatypes, & combiner);
  CHECK_RET(ret)
  debug("%d %d %d %d", num_integers, num_addresses, num_datatypes, combiner);

  assert(combiner != MPI_COMBINER_NAMED);

  int integers[num_integers];
  MPI_Aint addresses[num_addresses];
  MPI_Datatype datatypes[num_datatypes];

  ret = MPI_Type_get_contents(file_type, num_integers, num_addresses, num_datatypes, integers, addresses, datatypes);
  CHECK_RET(ret)

  for(int c=0; c < count; c++){
    switch(combiner){
    #ifdef SMPI_COMBINER_DUP
    case(MPI_COMBINER_DUP):{
      return mpix_process_datatype_i(mem_buff, mem_type, file_offset, bytes_to_access, count, datatypes[0], func, usr_ptr);
    }
    #endif
    #ifdef SMPI_COMBINER_CONTIGUOUS
    case(MPI_COMBINER_CONTIGUOUS):{
      return mpix_process_datatype_i(mem_buff, mem_type, file_offset, bytes_to_access, integers[0]*count, datatypes[0], func, usr_ptr);
    }
    #endif
    #ifdef SMPI_COMBINER_VECTOR
    case(MPI_COMBINER_VECTOR):{ // similar to HVECTOR, excect that the stride it is given in elements
      int strideElems = integers[2];
      size_t stride = (size_t) strideElems * typ_extent;
      size_t file_offset_initial = *file_offset;

      for(int i=integers[0] - 1; i >= 0; i--){
        ret = mpix_process_datatype_i(mem_buff, mem_type, file_offset, bytes_to_access, integers[1], datatypes[0], func, usr_ptr);
        if (ret != 0 || *bytes_to_access == 0) return ret;
        *mem_buff += ret;
        if (i != 0){
          *file_offset = file_offset_initial + stride;
        }
      }
      return 0;
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
    { // similar to VECTOR, excect that the stride it is given in bytes
      MPI_Aint stride = addresses[0];

      size_t file_offset_initial = *file_offset;
      for(int i=integers[0] - 1; i >= 0; i--){
        ret = mpix_process_datatype_i(mem_buff, mem_type, file_offset, bytes_to_access, integers[1], datatypes[0], func, usr_ptr);
        if (ret != 0 || *bytes_to_access == 0) return ret;
        *mem_buff += ret;
        if (i != 0){
          *file_offset = file_offset_initial + stride;
        }
      }
      return 0;
    }
    #ifdef SMPI_COMBINER_INDEXED
    case(MPI_COMBINER_INDEXED):{
      size_t file_offset_start = *file_offset;

      int count = integers[0];
      for(int i=0; i < count; i++){
        *file_offset = file_offset_start + integers[count + i + 1] * typ_extent;
        int blocklength = integers[i+1];
        ret = mpix_process_datatype_i(mem_buff, mem_type, file_offset, bytes_to_access, blocklength, datatypes[0], func, usr_ptr);
        if (ret != 0 || *bytes_to_access == 0) return ret;
        *mem_buff += ret;
      }
      return 0;
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
      size_t file_offset_start = *file_offset;

      for(int i=0; i < integers[0]; i++){
        *file_offset = file_offset_start + addresses[i];
        int blocklength = integers[i+1];
        // MPI_Aint displ = addresses[i]
        ret = mpix_process_datatype_i(mem_buff, mem_type, file_offset, bytes_to_access, blocklength, datatypes[0], func, usr_ptr);
        if (ret != 0 || *bytes_to_access == 0) return ret;
        *mem_buff += ret;
      }
      return 0;
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
        if( i != 2) printType(",");
        printType("%ld", addresses[i]);
      }
      printType("],typ=");
      mpix_decode_datatype(datatypes[0]);
      printType(")");
      return -1;
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
      size_t file_offset_start = *file_offset;
      for(int i=0; i < integers[0]; i++){
        *file_offset = file_offset_start + addresses[i];

        ret = mpix_process_datatype_i(mem_buff, mem_type, file_offset, bytes_to_access, integers[i+1], datatypes[i], func, usr_ptr);
        if (ret != 0 || *bytes_to_access == 0) return ret;
        *mem_buff += ret;
      }
      return 0;
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
        if( i != 1) printType(",");
        printType("%d", integers[i]);
      }
      printType("],subsize=[");
      for(int i=integers[0] + 1; i <= 2*integers[0]; i++){
        if( i != integers[0] + 1) printType(",");
        printType("%d", integers[i]);
      }
      printType("],starts=[");
      for(int i=2*integers[0] + 1; i <= 3*integers[0]; i++){
        if( i != 2*integers[0] + 1) printType(",");
        printType("%d", integers[i]);
      }
      printType("],order=%d,typ=", integers[3*integers[0]+1]);
      mpix_decode_datatype(datatypes[0]);
      printType(")");
      return -1;
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
        if( i != start) printType(",");
        printType("%d", integers[i]);
      }
      printType("],distribs=[");
      start += ndims;
      for(int i=start; i < ndims + start; i++){
        if( i != start) printType(",");
        printType("%d", integers[i]);
      }
      printType("],dargs=[");
      start += ndims;
      for(int i=start; i < ndims + start; i++){
        if( i != start) printType(",");
        printType("%d", integers[i]);
      }
      printType("],psizes=[");
      start += ndims;
      for(int i=start; i < ndims + start; i++){
        if( i != start) printType(",");
        printType("%d", integers[i]);
      }
      printType("],order=%d,typ=", integers[4*ndims+3]);
      mpix_decode_datatype(datatypes[0]);
      printType(")");
      return -1;
    }
    #ifdef SMPI_COMBINER_F90_REAL
    case(MPI_COMBINER_F90_REAL):{
      printType("F90_REAL(p=%d,r=%d)", integers[0], integers[1]);
      return -1;
    }
    #endif
    #ifdef SMPI_TYPE_CREATE_F90_REAL
    case(MPI_TYPE_CREATE_F90_REAL):{
      printType("F90_REAL(p=%d,r=%d)", integers[0], integers[1]);
      return -1;
    }
    #endif
    #ifdef SMPI_COMBINER_F90_COMPLEX
    case(MPI_COMBINER_F90_COMPLEX):{
      printType("F90_COMPLEX(p=%d,r=%d)", integers[0], integers[1]);
      return -1;
    }
    #endif
    #ifdef SMPI_TYPE_CREATE_F90_COMPLEX
    case(MPI_TYPE_CREATE_F90_COMPLEX):{
      printType("F90_COMPLEX(p=%d,r=%d)", integers[0], integers[1]);
      return -1;
    }
    #endif
    #ifdef SMPI_COMBINER_F90_INTEGER
    case(MPI_COMBINER_F90_INTEGER):{
      printType("F90_INTEGER(r=%d)", integers[0]);
      return -1;
    }
    #endif
    #ifdef SMPI_TYPE_CREATE_F90_INTEGER
    case(MPI_TYPE_CREATE_F90_INTEGER):{
      printType("F90_INTEGER(r=%d)", integers[0]);
      return -1;
    }
    #endif
    #ifdef SMPI_TYPE_CREATE_RESIZED
    case(MPI_TYPE_CREATE_RESIZED):
    #endif
    #ifdef SMPI_COMBINER_RESIZED
    case(MPI_COMBINER_RESIZED):
    #endif
    { // TODO Check
      MPI_Aint lb = addresses[0];
      MPI_Aint ub = addresses[1];
      MPI_Aint delta = ub - lb;

      size_t file_offset_after = *file_offset + delta;
      ret = mpix_process_datatype_i(mem_buff, mem_type, file_offset, bytes_to_access, 1, datatypes[0], func, usr_ptr);
      if (ret != 0 || *bytes_to_access == 0) return ret;
      *file_offset = file_offset_after;
      *mem_buff += ret;
      return 0;
    }
    default:{
      printf("ERROR Unsupported combiner: %d\n", combiner);
    }
    }
  }
  return -2;
}

size_t mpix_process_datatype(void * mem_buff, int count, MPI_Datatype mem_type, size_t file_offset,  MPI_Datatype file_type, MPI_Type_decode_func func, void * usr_ptr){
  // check if the memory datatype is contiguous in memory
  MPI_Aint      extent;
  int typ_size;
  MPI_Type_extent( mem_type, &extent);
  MPI_Type_size( mem_type, &typ_size );
  assert(typ_size == extent);

  size_t bytes_to_process = ((size_t) typ_size)*count;

  int ret = 0;
  while(ret == 0 && bytes_to_process > 0){
    ret = mpix_process_datatype_i((char**) & mem_buff, mem_type, & file_offset, & bytes_to_process, 1, file_type, func, usr_ptr);
  }
  return ((size_t) typ_size)*count - bytes_to_process;
}
