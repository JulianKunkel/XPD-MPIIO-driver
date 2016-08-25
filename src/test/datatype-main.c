#include <mpi.h>
#include <stdio.h>

#include "datatype.h"

struct particle{
   int    	type;
   double   a[3];
   char   	b[4];
 };

size_t decode_print(void * usr_ptr, size_t size, char * buff, size_t file_pos){
  printf("%zu %zu\n", file_pos, size);
  return size;
}

int main(int argc, char ** argv){
  int ret;
  MPI_Init(& argc, & argv);

  MPI_Datatype hvec;
  ret = MPI_Type_create_hvector(2, 3, 100, MPI_INT, & hvec);
  CHECK_RET(ret)
  MPI_Type_commit(& hvec);
  mpix_decode_datatype(hvec);
  printf("\n");


  MPI_Datatype m_struct;
  MPI_Datatype type[4] = {MPI_INT, MPI_DOUBLE, MPI_CHAR, MPI_UB};
  int          blocklen[4] = {1, 3, 4, 1};
  MPI_Aint     disp[4];
  MPI_Aint     base;

  struct particle p = {.type = 5, .a = {1,2,3}, .b="tst"};
  disp[0] = 0;
  MPI_Address( & p, & base);
  MPI_Address( p.a, disp+1);
  MPI_Address( p.b, disp+2);
  for (int i=1; i < 3; i++){
    disp[i] -= base;
  }
  disp[3] = sizeof(p);
  MPI_Type_struct( 4, blocklen, disp, type, & m_struct);
  MPI_Type_commit(& m_struct);
  mpix_decode_datatype(m_struct);
  printf("\n");

  MPI_Datatype hvec_extended;
  MPI_Type_create_resized(hvec, 0, 1000, & hvec_extended);
  MPI_Type_commit(& hvec_extended);
  mpix_decode_datatype(hvec_extended);
  printf("\n");

  MPI_Aint ex;
  int count;
  MPI_Type_extent(m_struct, & ex);
  MPI_Type_size(m_struct, & count);
  printf("Struct has %d of %ld\n", count, ex);

  MPI_Type_extent(hvec, & ex);
  MPI_Type_size(hvec, & count);
  printf("hvec has %d of %ld\n", count, ex);

  MPI_Type_extent(hvec_extended, & ex);
  MPI_Type_size(hvec_extended, & count);
  printf("hvec extended has %d of %ld\n", count, ex);


  char buffer[100];
  size_t accessed;

  accessed = mpix_process_datatype(buffer, 97, MPI_CHAR, 0, m_struct, decode_print, NULL);
  printf("Accessed: %zu \n", accessed);

  accessed = mpix_process_datatype(buffer, 97, MPI_CHAR, 0, hvec, decode_print, NULL);
  printf("Accessed: %zu \n", accessed);

  accessed = mpix_process_datatype(buffer, 97, MPI_CHAR, 0, hvec_extended, decode_print, NULL);
  printf("Accessed: %zu \n", accessed);


  MPI_Finalize();
  return 0;
}
