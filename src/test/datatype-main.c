#include <mpi.h>
#include <stdio.h>

#define CHECK_RET(ret) if(ret != MPI_SUCCESS){ printf("Critical error decoding datatype in %d\n", __LINE__); MPI_Abort(MPI_COMM_WORLD, 1);}

void mpix_decode_datatype(MPI_Datatype typ);

int main(int argc, char ** argv){
  int ret;
  MPI_Init(& argc, & argv);

  MPI_Datatype hvec;
  ret = MPI_Type_create_hvector(2, 3, 4, MPI_INT, & hvec);
  CHECK_RET(ret)
  MPI_Type_commit(& hvec);
  mpix_decode_datatype(hvec);

  printf("\n");

  MPI_Finalize();
  return 0;
}
