// This file is part of XPD-MPIIO-Driver (XMD).
// It is subject to the license terms in the LICENSE file found in the top-level directory of
// this distribution and at https://opensource.org/licenses/BSD-3-Clause.

#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include <unistd.h>


#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define FALSE 0

#ifndef CONST
#define CONST
#endif

#include <kdsa.h>

#include "datatype.h"
#include "debug.h"

#define XPD_FLAGS (KDSA_FLAGS_HANDLE_IO_NOSPIN|KDSA_FLAGS_HANDLE_USE_EVENT)
//KDSA_FLAGS_HANDLE_UNSAFE_WRITE

typedef kdsa_vol_handle_t handle;

#define HEADER_SIZE sizeof(uint64_t)

extern int kdsa_set_read_buffer_size(handle, size_t);
extern int kdsa_set_write_buffer_size(handle, size_t);

// internal structure for MPI_File
typedef struct{
  handle fd;
  MPI_Offset offset;
  MPI_Offset file_size;
  MPI_Offset initial_file_size;
  int atomicity;
  MPI_Comm comm;
  char * filename;

  MPI_Datatype etype;
  MPI_Datatype ftype;
  size_t ftype_displacement;

  int onXPD; // is the file stored on the XPD
  MPI_File mpi_fh; // the original file handle
} xpd_fh_t;

int fileIsOnXPD(const char * name){
  return strncmp(name, "xpd:", 4) == 0 || strncmp(name, "XPD:", 4) == 0;
}

static void hexDump(unsigned const char * dest, size_t out_size){
  if (xpdDebugLevel > 4){
    size_t i;
  	for (i=0; i < out_size ; i++){
  		printf("%.2x ", dest[i]);
  	}
  	printf("\n");
  }
}


static void get_file_size(xpd_fh_t * f){
  int rank;
  int ret;
  ret = MPI_Comm_rank(f->comm, & rank);
  assert(ret == MPI_SUCCESS);
  if (rank == 0){
    // it would be possible to pre-register the file_size pointer...
    ret = kdsa_read_unregistered(f->fd, 0, & f->file_size, sizeof(uint64_t));
    assert(ret == 0);

    debug("reading file \"%s\" size:%lld\n", f->filename, (long long) f->file_size);
  }
  ret = MPI_Bcast(& f->file_size, 1, MPI_UINT64_T, 0, f->comm);
  assert(ret == MPI_SUCCESS);
  f->initial_file_size = f->file_size;
}

static inline void flush_file_size(xpd_fh_t * f){
  int ret = MPI_Allreduce(MPI_IN_PLACE, & f->file_size, 1, MPI_UINT64_T, MPI_MAX, f->comm);
  assert(ret == MPI_SUCCESS);
  // update size in the file
  int rank;
  ret = MPI_Comm_rank(f->comm, & rank);
  assert(ret == MPI_SUCCESS);
  if (rank == 0 && f->initial_file_size != f->file_size){
    int ret;
    // it would be possible to pre-register the file_size pointer...
    ret = kdsa_write_unregistered(f->fd, 0, & f->file_size, sizeof(uint64_t));
    assert(ret == 0);

    debug("flushing file size %lld\n", (long long) f->file_size);
  }
}

static inline int pack_data(MPI_Datatype datatype, int count, xpd_fh_t * f, uint64_t * length_out, void ** tmp_buf){
  int size;
  int ret;
  ret = MPI_Type_size(datatype, & size);
  assert(ret == MPI_SUCCESS);
  uint64_t length = ((uint64_t) count) * size;

  MPI_Aint extent, lb;
  ret = MPI_Type_get_extent(datatype, &lb, & extent);
  assert(ret == MPI_SUCCESS);

  *length_out = length;

  if (lb != 0 || size != extent){ // datatype is not contiguous
    int position = 0;
    // printf("Packing Data! %lld %lld %lld\n", (long long) lb, (long long) extent, (long long) size);
    void * outbuf = malloc(length);
    ret = MPI_Pack(*tmp_buf, count, datatype, outbuf, length, & position, f->comm);
    assert(ret == MPI_SUCCESS);
    *tmp_buf = outbuf;
    return 1;
  }

  return 0;
}

static size_t writer_noncontig_func(xpd_fh_t * f, size_t size, char * buff, size_t file_pos){
  size_t ret = kdsa_write_unregistered(f->fd, file_pos, buff, size);
  debug("non-contig write offset: %zu size: %zu\n", file_pos - HEADER_SIZE, size);
  hexDump((unsigned char*) buff, size);

  if (f->file_size < file_pos + size - HEADER_SIZE){
    f->file_size = file_pos + size - HEADER_SIZE;
  }

  return ret == 0 ? size : 0;
}

int MPI_File_write_at(MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
  FUNC_START
  if (count == 0){
    return MPI_SUCCESS;
  }
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (f->onXPD){
    int ret = 0;
    // support more datatypes using MPI_Pack
    uint64_t length;
    void * tmp_buf = (void*) buf;
    int allocated_buffer = pack_data(datatype, count, f, & length, & tmp_buf);
    offset = offset + f->ftype_displacement;

    debug("MPI write offset: %lld count: %lld int count: %d\n", (long long) offset  - HEADER_SIZE, (long long) length, count);

    if (f->ftype != MPI_BYTE){
      if (xpdDebugLevel) mpix_decode_datatype(f->ftype);
      ret = mpix_process_datatype((void*) tmp_buf, length, MPI_BYTE, offset, f->ftype, & writer_noncontig_func, f) != length;
    }else{ // contiguous in file
      ret = kdsa_write_unregistered(f->fd, offset, tmp_buf, length);

      if(xpdDebugLevel > 4){
        void * buffer = malloc(length);
        ret = kdsa_read_unregistered(f->fd, offset, buffer, length);
        assert( ret == 0 );
        ret = memcmp(buffer, tmp_buf, length);
        if (ret != 0){
          printf("ERROR write_at differs\n");
          exit(1);
        }

        hexDump(buffer, length);

        free(buffer);
      }

      if (f->file_size < offset + length - HEADER_SIZE){
        f->file_size = offset + length - HEADER_SIZE;
      }
    }
    if (ret != 0){
      if (allocated_buffer) free(tmp_buf);
      return MPI_ERR_BUFFER;
    }

    MPI_Status_set_elements(status, datatype, count);
    MPI_Status_set_elements_x(status, datatype, length);

    if (allocated_buffer) free(tmp_buf);
    return MPI_SUCCESS;
  }
  return PMPI_File_write_at(f->mpi_fh, offset, buf, count, datatype, status);
}

static size_t reader_noncontig_func(xpd_fh_t * f, size_t size, char * buff, size_t file_pos){
  debug("non-contig read offset: %zu size: %zu\n", file_pos, size);
  int ret;

  if (f->file_size < file_pos + size){
    return 0;
  }

  ret = kdsa_read_unregistered(f->fd, file_pos, buff, size);
  return ret == 0 ? size : 0;
}

int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
  FUNC_START
  if (count == 0){
    return MPI_SUCCESS;
  }
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (f->onXPD){
    int ret = 0;
    uint64_t length;
    // todo fix for arbitrary buffers using MPI_unpack
    {
      int size;
      int ret;
      ret = MPI_Type_size(datatype, & size);
      assert(ret == MPI_SUCCESS);

      MPI_Aint extent, lb;
      ret = MPI_Type_get_extent(datatype, &lb, & extent);
      assert(ret == MPI_SUCCESS);

      assert(lb == 0 && size == extent);

      length = ((uint64_t) count) * size;
    }

    if ((length + offset) > f->file_size){
      if (offset >= f->file_size){
        MPI_Status_set_elements(status, datatype, 0);

        return MPI_SUCCESS;
      }
      length = f->file_size - offset;
    }

    offset = offset + f->ftype_displacement;

    if (f->ftype != MPI_BYTE){
      if (xpdDebugLevel) mpix_decode_datatype(f->ftype);
      ret = mpix_process_datatype(buf, length, MPI_BYTE, offset, f->ftype, & reader_noncontig_func, f) != length;
    }else{
      ret = kdsa_read_unregistered(f->fd, offset, buf, length);
    }
    debug("MPI read offset: %lld count: %lld ret: %d\n", (long long) offset - HEADER_SIZE, (long long) length, ret);
    hexDump(buf, length);

    if (ret != 0){ // TODO proper error handling
      MPI_Status_set_elements(status, datatype, 0);

      return MPI_SUCCESS;
    }
    MPI_Status_set_elements(status, datatype, count);
    MPI_Status_set_elements_x(status, datatype, length);

    return MPI_SUCCESS;
  }
  return PMPI_File_read_at(f->mpi_fh, offset, buf, count, datatype, status);
}

int MPI_File_open(MPI_Comm comm, CONST char *filename, int amode, MPI_Info info, MPI_File *fh){
  FUNC_START
  xpd_fh_t * f = malloc(sizeof(xpd_fh_t));
  *fh = (MPI_File) f;

  f->onXPD = fileIsOnXPD(filename);
  debug("File on XPD: %d\n", f->onXPD);

  if (f->onXPD){
    char * xpdDebugString = getenv("MPI_XPD_DEBUG");
    if (xpdDebugString != NULL){
      xpdDebugLevel = atoi(xpdDebugString);
      char * debug_rank_str = getenv("MPI_XPD_DEBUG_RANK");
      if (debug_rank_str != NULL){
        int rank = -1;
        int debug_rank = -1;
        MPI_Comm_rank(MPI_COMM_WORLD, & rank);
        debug_rank = atoi(debug_rank_str);
        if (rank != debug_rank){
          xpdDebugLevel = 0;
        }
      }
    }
    xpdDebugString = getenv("MPI_XPD_DEBUG_SPIN");
    if (xpdDebugString != NULL){
      printf("WAITING, connect with the debugger, set spin to 0\n");
      volatile int spin=1;
      while(spin == 1) sleep(1);
    }

    f->offset = 0;
    f->filename = strdup(filename);
    f->etype = MPI_BYTE;
    f->ftype = MPI_BYTE;
    f->ftype_displacement = HEADER_SIZE;

    int status = kdsa_connect(((char*) filename)+4, XPD_FLAGS, & f->fd);
    if(status < 0)
    {
      printf("Failed to connect to XPD: %s (%d)\n", strerror(errno), errno);
      exit(-1);
    }
    int ret = MPI_Comm_dup(comm, & f->comm);
    assert(ret == MPI_SUCCESS);

    //if (amode & MPI_MODE_EXCL || amode & MPI_MODE_CREATE){
    //  f->file_size = 0;
    //}else{
    get_file_size(f);
    //}

    const char * buffer_size_str = getenv("KDSA_MPI_BUFFER_SIZE");

    if(buffer_size_str != NULL){
      size_t buffer_size = atoll(buffer_size_str);
      if (buffer_size < 10*1024){
        printf("MPI-XPD: Error: invalid buffer_size %zu\n", buffer_size);
      }else{
        ret = kdsa_set_read_buffer_size(f->fd, buffer_size);
        assert( ret >= 0);
        ret = kdsa_set_write_buffer_size(f->fd, buffer_size);
        assert( ret >= 0);
      }
    }

    f->atomicity = FALSE;
    return MPI_SUCCESS;
  }
  int ret = PMPI_File_open(comm, filename, amode, info, & f->mpi_fh);
  if (ret != MPI_SUCCESS){
    free(f);
    *fh = NULL;
  }
  return ret;
}

int MPI_File_close(MPI_File *fh){
  FUNC_START

  // synchronize the file position
  xpd_fh_t * f = (xpd_fh_t*) (*fh);
  if (f->onXPD){
    flush_file_size(f);

    int status = kdsa_disconnect(f->fd);
  	if(status < 0)
  	{
  		printf("Failed to disconnect\n");
  		exit(-1);
  	}

    MPI_Comm_free(& f->comm);
    free(f->filename);
    free(*fh);
    *fh = NULL;
    return MPI_SUCCESS;
  }
  int ret = PMPI_File_close(& f->mpi_fh);
  free(*fh);
  *fh = NULL;
  return ret;
}




/////////////////////////////////////////////////////////////////////////////////


int MPI_File_write(MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
  FUNC_START
  xpd_fh_t * f = (xpd_fh_t*) fh;
  int ret;
  if (f->onXPD){
    ret = MPI_File_write_at(fh, f->offset, buf, count, datatype, status);
    // todo adjust file position
    if (ret == MPI_SUCCESS){
      int size;
      MPI_Type_size(datatype, & size);

      f->offset += ((MPI_Offset) size)*count;
    }
    return ret;
  }
  return PMPI_File_write(f->mpi_fh, buf, count, datatype, status);
}


int MPI_File_write_at_all(MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (! f->onXPD){
    return PMPI_File_write_at_all(f->mpi_fh, offset, buf, count, datatype, status);
  }

  return MPI_File_write_at(fh, offset, buf, count, datatype, status);
}

int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
  FUNC_START
  xpd_fh_t * f = (xpd_fh_t*) fh;
  int ret;
  if (f->onXPD){
    ret = MPI_File_read_at(fh, f->offset, buf, count, datatype, status);
    if (ret == MPI_SUCCESS){
      int size;
      MPI_Type_size(datatype, & size);

      f->offset += ((MPI_Offset) size)*count;
    }
    return MPI_SUCCESS;
  }
  return PMPI_File_read(f->mpi_fh, buf, count, datatype, status);
}


int MPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status){
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (! f->onXPD){
    return PMPI_File_read_at_all(f->mpi_fh, offset, buf, count, datatype, status);
  }
  return MPI_File_read_at(fh, offset, buf, count, datatype, status);
}

// we never delete the file.
int MPI_File_delete(CONST char *filename, MPI_Info info){
  FUNC_START
  if(fileIsOnXPD(filename)){
    handle fd;
    // set the file size to 0
    int status = kdsa_connect(((char*) filename)+4, XPD_FLAGS, & fd);
    if(status < 0){
      printf("Failed to connect to XPD: %s (%d)\n", strerror(errno), errno);
      exit(-1);
    }
    size_t size = 0;
    status = kdsa_write_unregistered(fd, 0, & size, sizeof(size_t));

    status = kdsa_disconnect(fd);
    if(status < 0){
    	printf("Failed to disconnect\n");
    	exit(-1);
    }

    return MPI_SUCCESS;
  }

  return PMPI_File_delete(filename, info);
}


int MPI_File_sync(MPI_File fh){
  FUNC_START
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (f->onXPD){
    assert(sizeof(uint64_t) == sizeof(MPI_Offset));
    flush_file_size(f);

    return MPI_SUCCESS;
  }
  return PMPI_File_sync(f->mpi_fh);
}

int MPI_File_seek(MPI_File fh, MPI_Offset offset, int whence){
  FUNC_START
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (f->onXPD){
    switch(whence){
      case SEEK_SET:
        f->offset = offset;
        break;
      case SEEK_CUR:
        f->offset += offset;
        break;
      case SEEK_END:
        // TODO: this may actually not completely correct, as it requires synchronization of the file position
        // However, from the user side, one should do a sync, which synchronizes the position
        f->offset = f->file_size - offset;
        break;
    }
    return MPI_SUCCESS;
  }
  return PMPI_File_seek(f->mpi_fh, offset, whence);
}

int MPI_File_get_position(MPI_File fh, MPI_Offset *offset){
  FUNC_START
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (f->onXPD){
    *offset = f->offset;
    return MPI_SUCCESS;
  }
  return PMPI_File_get_position(f->mpi_fh, offset);
}

int MPI_File_set_atomicity(MPI_File fh, int flag){
  FUNC_START
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (f->onXPD){
    f->atomicity = flag;
    return MPI_SUCCESS;
  }
  return PMPI_File_set_atomicity(f->mpi_fh, flag);
}

int MPI_File_get_atomicity(MPI_File fh, int *flag){
  FUNC_START
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (f->onXPD){
    *flag = f->atomicity;
    return MPI_SUCCESS;
  }
  return PMPI_File_get_atomicity(f->mpi_fh, flag);
}

int MPI_File_set_size(MPI_File fh, MPI_Offset size){
  FUNC_START
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (f->onXPD){
    f->file_size = size;
    return MPI_SUCCESS;
  }
  return PMPI_File_set_size(f->mpi_fh, size);
}


int MPI_File_get_size(MPI_File fh, MPI_Offset *size){
  FUNC_START
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (f->onXPD){
    *size = f->file_size;
    return MPI_SUCCESS;
  }
  return PMPI_File_get_size(f->mpi_fh, size);
}


int MPI_File_preallocate(MPI_File fh, MPI_Offset size){
  FUNC_START
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (f->onXPD){
    f->file_size = size;
    // We may actually fill the file with 0
    return MPI_SUCCESS;
  }
  return PMPI_File_preallocate(f->mpi_fh, size);
}

int MPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, CONST char *datarep, MPI_Info info){
  FUNC_START
  xpd_fh_t * f = (xpd_fh_t*) fh;
  if (f->onXPD){
    f->etype = etype;
    f->ftype = filetype;
    f->ftype_displacement = disp + HEADER_SIZE;
    return MPI_SUCCESS;
  }
  return PMPI_File_set_view(f->mpi_fh, disp, etype, filetype, datarep, info);
}
