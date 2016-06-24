// This file is part of XPD-MPIIO-Driver (XMD).
// It is subject to the license terms in the LICENSE file found in the top-level directory of
// this distribution and at https://opensource.org/licenses/BSD-3-Clause.

// This tool formats the XPD storage for a single file
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <kdsa.h>

void synopsis(char * name){
  printf("Synopsis: %s <XPD_FILE_NAME> [-f]\n", name);
  printf("-f\t full format\n");
  exit(1);
}

int main(int argc, char ** argv){
  if(argc < 2){
    synopsis(argv[0]);
  }
  int fullformat = 0;
  int status;
  if (argc == 3){
    if( strcmp(argv[2], "-f") == 0){
      fullformat = 1;
    }else{
      printf("ERR: invalid argument: %s\n", argv[2]);
      synopsis(argv[0]);
    }
  }
  char * file = argv[1];

  printf("Formating %s\n", file);

  kdsa_vol_handle_t fd;
  status = kdsa_connect((char*)file, 0, & fd);
  if(status < 0)
  {
      printf("Failed to connect to XPD: %s (%d)\n", strerror(errno), errno);
      exit(-1);
  }

  // now format.
  #define BUF_SIZE (1024*1024*10)
  void * buf = malloc(BUF_SIZE);
  memset(buf, 0, BUF_SIZE);
  status = kdsa_write_unregistered(fd, 0, buf, BUF_SIZE);
  if (status < 0){
    printf("Failed to format file: %s (%d)\n", strerror(errno), errno);
    exit(-1);
  }
  if (fullformat){
    off_t pos = 0;
    while(status >= 0){
      status = kdsa_write_unregistered(fd, pos*BUF_SIZE, buf, BUF_SIZE);
      pos++;
      if(pos % 100){
        printf("%u GB\n", (unsigned)(pos/100));
      }
    }
    printf("Stopped at %u GB\n", (unsigned)(pos/100));
  }

  status = kdsa_disconnect(fd);
  if(status < 0)
  {
      printf("Failed to disconnect from XPD: %s (%d)\n", strerror(errno), errno);
      exit(-1);
  }
}
