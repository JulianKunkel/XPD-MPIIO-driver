#include <mpi.h>
#include <stdio.h>

#define CHECK_RET(ret) if(ret != MPI_SUCCESS){ printf("Critical error decoding datatype in %d\n", __LINE__); MPI_Abort(MPI_COMM_WORLD, 1);}

void mpix_decode_datatype(MPI_Datatype typ);

struct particle{
   int    	type;
   double   a[3];
   char   	b[4];
 };


int main(int argc, char ** argv){
  int ret;
  MPI_Init(& argc, & argv);

  MPI_Datatype hvec;
  ret = MPI_Type_create_hvector(2, 3, 4, MPI_INT, & hvec);
  CHECK_RET(ret)
  MPI_Type_commit(& hvec);
  mpix_decode_datatype(hvec);
  printf("\n");


  MPI_Datatype m_struct;
  MPI_Datatype type[3] = {MPI_INT, MPI_DOUBLE, MPI_CHAR};
  int          blocklen[3] = {1, 3, 4};
  MPI_Aint     disp[3];
  MPI_Aint     base;

  struct particle p;
  disp[0] = 0;
  MPI_Address( & p, & base);
  MPI_Address( p.a, disp+1);
  MPI_Address( p.b, disp+2);
  for (int i=1; i < 3; i++){
    disp[i] -= base;
  }
  MPI_Type_struct( 3, blocklen, disp, type, & m_struct);
  MPI_Type_commit(& m_struct);
  mpix_decode_datatype(m_struct);
  printf("\n");

  MPI_Finalize();
  return 0;
}
