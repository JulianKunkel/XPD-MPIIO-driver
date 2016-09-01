// This file is part of XPD-MPIIO-Driver (XMD).
// It is subject to the license terms in the LICENSE file found in the top-level directory of
// this distribution and at https://opensource.org/licenses/BSD-3-Clause.

#include <stdio.h>
#include <mpi.h>
#include <assert.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#define FALSE 0

#include <mpi.h>

int fileIsOnXPD(char const * name);

size_t filesize(char * name){
  size_t size;
  int ret;
  MPI_File fh;
  MPI_Offset off;
  ret = MPI_File_open(MPI_COMM_WORLD, name, MPI_MODE_RDONLY, MPI_INFO_NULL, & fh);
  if(ret != MPI_SUCCESS){
    printf("Could not open file %s\n", name);
    exit(1);
  }
  ret = MPI_File_get_size(fh, & off);
  size = off;
  assert(ret == MPI_SUCCESS);
  ret = MPI_File_close(& fh);
  assert(ret == MPI_SUCCESS);
  return size;
}

#define BLOCK_SIZE (10*1024*1024)

int main(int argc, char ** argv){
  if (argc != 3){
      printf("Synopsis: %s <source> <target>\n", argv[0]);
      printf("Source and/or target can reside on an XPD\n");
      exit(1);
  }

  MPI_Init(& argc, & argv);

  char * source = argv[1];
  char * target = argv[2];
  const int srcXPD = fileIsOnXPD(source);
  const int tgtXPD = fileIsOnXPD(target);
  size_t processed;
  int ret;

  const size_t size = filesize(source);
  printf("Copy %s (%s) size: %lld to %s (%s)\n", source, srcXPD ? "XPD" : "local", (long long) size, target, tgtXPD ? "XPD" : "local");

  MPI_File fr, fw;
  ret = MPI_File_open(MPI_COMM_WORLD, source, MPI_MODE_RDONLY, MPI_INFO_NULL, & fr);
  assert(ret == MPI_SUCCESS);
  MPI_File_delete(target, MPI_INFO_NULL);
  ret = MPI_File_open(MPI_COMM_WORLD, target, MPI_MODE_WRONLY|MPI_MODE_CREATE, MPI_INFO_NULL, & fw);
  assert(ret == MPI_SUCCESS);

  char * buff = (char*) malloc(BLOCK_SIZE);

  for(processed = 0; processed < size; processed += BLOCK_SIZE ){
    MPI_Status status;
    size_t transfer_size = BLOCK_SIZE;
    if (processed + BLOCK_SIZE > size){
      transfer_size = size - processed;
    }
    ret = MPI_File_read_at(fr, processed, buff, transfer_size, MPI_BYTE, & status);
    assert(ret == MPI_SUCCESS);
    printf("%zu \n", transfer_size);
    ret = MPI_File_write_at(fw, processed, buff, transfer_size, MPI_BYTE, & status);
    assert(ret == MPI_SUCCESS);
  }
  free(buff);
  ret = MPI_File_close(& fr);
  assert(ret == MPI_SUCCESS);
  ret = MPI_File_close(& fw);
  assert(ret == MPI_SUCCESS);

  MPI_Finalize();

  return 0;
}
