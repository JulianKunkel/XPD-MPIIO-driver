#ifdef DEBUG
  #define debug(...) printf("[DT] "__VA_ARGS__);
#else
  #define debug(...)
#endif

#define printType(...) printf(__VA_ARGS__);

#define CHECK_RET(ret) if(ret != MPI_SUCCESS){ printf("Critical error decoding datatype in %d\n", __LINE__); MPI_Abort(MPI_COMM_WORLD, 1);}

// return != size if an error occurs
typedef size_t(*MPI_Type_decode_func)(void * usr_ptr, size_t size, char * buff, size_t file_pos);

void mpix_decode_datatype(MPI_Datatype typ);

/*
 * Run the given function on each consecutive data piece.
 * @return the number of bytes processed.
 */
size_t mpix_process_datatype(void * mem_buff, int count, MPI_Datatype mem_type, size_t file_offset, MPI_Datatype file_type, MPI_Type_decode_func func, void * usr_ptr);
