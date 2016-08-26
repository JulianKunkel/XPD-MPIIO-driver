#include <mpi.h>
#include <stdio.h>

#include "datatype.h"

#define CONST const

int MPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, CONST char *datarep, MPI_Info info){
  printf("MPI_File_set_view\n");
  mpix_decode_datatype(filetype);
  mpix_decode_datatype(etype);
  return PMPI_File_set_view(fh, disp, etype, filetype, datarep, info);
}
