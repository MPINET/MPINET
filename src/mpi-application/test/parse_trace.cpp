#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <arpa/inet.h>
#include <unordered_map>

#pragma pack (1)    //取消结构体字节对齐， #pragma pack () 则恢复原来的字节对齐规则
#define DUMPI_ANY_TAG -1
#define DUMPI_STATUS_IGNORE nullptr
#define DUMPI_THREADID_MASK (1<<6)
using namespace std;

/** A reasonably compact type handle for an MPI combiner */
typedef int16_t dumpi_combiner;
/** A reasonably compact type handle for an MPI comm */
typedef int16_t dumpi_comm;
/** A reasonably compact type handle for an MPI comparison */
typedef int8_t  dumpi_comparison;
/** A reasonably compact type handle for an MPI datatype */
typedef int16_t dumpi_datatype;
/** A reasonably compact type handle for an MPI dest */
typedef int32_t dumpi_dest;
/** A reasonably compact type handle for an MPI distribution */
typedef int16_t dumpi_distribution;
/** A reasonably compact type handle for an MPI errcode */
typedef int32_t dumpi_errcode;
/** A reasonably compact type handle for an MPI errhandler */
typedef int16_t dumpi_errhandler;
/** A reasonably compact type handle for an MPI file */
typedef int16_t dumpi_file;
/** A reasonably compact type handle for an MPI filemode */
typedef int16_t dumpi_filemode;
/** A reasonably compact type handle for an MPI group */
typedef int16_t dumpi_group;
/** A reasonably compact type handle for an MPI info */
typedef int16_t dumpi_info;
/** A reasonably compact type handle for an MPI keyval */
typedef int16_t dumpi_keyval;
/** A reasonably compact type handle for an MPI comm_keyval */
typedef int16_t dumpi_comm_keyval;
/** A reasonably compact type handle for an MPI type_keyval */
typedef int16_t dumpi_type_keyval;
/** A reasonably compact type handle for an MPI win_keyval */
typedef int16_t dumpi_win_keyval;
/** A reasonably compact type handle for an MPI locktype */
typedef int16_t dumpi_locktype;
/** A reasonably compact type handle for an MPI op */
typedef uint8_t dumpi_op;
/** A reasonably compact type handle for an MPI ordering */
typedef int8_t  dumpi_ordering;
/** A reasonably compact type handle for an MPI source */
typedef int32_t dumpi_source;
/** Tag value was converted from 16-bit to 32-bit value in 0.6.5
 *  The value has always been stored as a 32-bit value in the file */
typedef int32_t dumpi_tag;
/** A reasonably compact type handle for an MPI threadlevel */
typedef int16_t dumpi_threadlevel;
/** A reasonably compact type handle for an MPI topology */
typedef int16_t dumpi_topology;
/** A reasonably compact type handle for an MPI typeclass */
typedef int16_t dumpi_typeclass;
/** A reasonably compact type handle for an MPI whence */
typedef int16_t dumpi_whence;
/** A reasonably compact type handle for an MPI win */
typedef int16_t dumpi_win;
/** A reasonably compact type handle for an MPI win_assert */
typedef int16_t dumpi_win_assert;
/** A reasonably compact type handle for an MPI request */
typedef int32_t dumpi_request;
/** A reasonably compact type handle for an MPIO request */
typedef int32_t dumpio_request;

typedef struct dumpi_status {
    int32_t bytes;
    int32_t source;
    int32_t tag;
    int8_t  cancelled;
    int8_t  error;
} dumpi_status;

typedef enum dumpi_timestamps {
    DUMPI_TIME_NONE=0, DUMPI_TIME_CPU=(1<<2), DUMPI_TIME_WALL=(1<<3),
    DUMPI_TIME_FULL=(DUMPI_TIME_CPU|DUMPI_TIME_WALL)
} dumpi_timestamps;

typedef struct dumpi_clock {
    int32_t sec;
    int32_t nsec;
} dumpi_clock;

typedef struct dumpi_time {
    dumpi_clock start;   /* stored as 6 bytes */
    dumpi_clock stop;    /* stored as 6 bytes */
} dumpi_time;

typedef enum dumpi_function {
    DUMPI_Send=0,                    DUMPI_Recv,
    DUMPI_Get_count,                 DUMPI_Bsend,
    DUMPI_Ssend,                     DUMPI_Rsend,
    DUMPI_Buffer_attach,             DUMPI_Buffer_detach,
    DUMPI_Isend,                     DUMPI_Ibsend,
    DUMPI_Issend,                    DUMPI_Irsend,
    DUMPI_Irecv,                     DUMPI_Wait,
    DUMPI_Test,                      DUMPI_Request_free,
    DUMPI_Waitany,                   DUMPI_Testany,
    DUMPI_Waitall,                   DUMPI_Testall,
    DUMPI_Waitsome,                  DUMPI_Testsome,
    DUMPI_Iprobe,                    DUMPI_Probe,
    DUMPI_Cancel,                    DUMPI_Test_cancelled,
    DUMPI_Send_init,                 DUMPI_Bsend_init,
    DUMPI_Ssend_init,                DUMPI_Rsend_init,
    DUMPI_Recv_init,                 DUMPI_Start,
    DUMPI_Startall,                  DUMPI_Sendrecv,
    DUMPI_Sendrecv_replace,          DUMPI_Type_contiguous,
    DUMPI_Type_vector,               DUMPI_Type_hvector,
    DUMPI_Type_indexed,              DUMPI_Type_hindexed,
    DUMPI_Type_struct,               DUMPI_Address,
    DUMPI_Type_extent,               DUMPI_Type_size,
    DUMPI_Type_lb,                   DUMPI_Type_ub,
    DUMPI_Type_commit,               DUMPI_Type_free,
    DUMPI_Get_elements,              DUMPI_Pack,
    DUMPI_Unpack,                    DUMPI_Pack_size,
    DUMPI_Barrier,                   DUMPI_Bcast,
    DUMPI_Gather,                    DUMPI_Gatherv,
    DUMPI_Scatter,                   DUMPI_Scatterv,
    DUMPI_Allgather,                 DUMPI_Allgatherv,
    DUMPI_Alltoall,                  DUMPI_Alltoallv,
    DUMPI_Reduce,                    DUMPI_Op_create,
    DUMPI_Op_free,                   DUMPI_Allreduce,
    DUMPI_Reduce_scatter,            DUMPI_Scan,
    DUMPI_Group_size,                DUMPI_Group_rank,
    DUMPI_Group_translate_ranks,     DUMPI_Group_compare,
    DUMPI_Comm_group,                DUMPI_Group_union,
    DUMPI_Group_intersection,        DUMPI_Group_difference,
    DUMPI_Group_incl,                DUMPI_Group_excl,
    DUMPI_Group_range_incl,          DUMPI_Group_range_excl,
    DUMPI_Group_free,                DUMPI_Comm_size,
    DUMPI_Comm_rank,                 DUMPI_Comm_compare,
    DUMPI_Comm_dup,                  DUMPI_Comm_create,
    DUMPI_Comm_split,                DUMPI_Comm_free,
    DUMPI_Comm_test_inter,           DUMPI_Comm_remote_size,
    DUMPI_Comm_remote_group,         DUMPI_Intercomm_create,
    DUMPI_Intercomm_merge,           DUMPI_Keyval_create,
    DUMPI_Keyval_free,               DUMPI_Attr_put,
    DUMPI_Attr_get,                  DUMPI_Attr_delete,
    DUMPI_Topo_test,                 DUMPI_Cart_create,
    DUMPI_Dims_create,               DUMPI_Graph_create,
    DUMPI_Graphdims_get,             DUMPI_Graph_get,
    DUMPI_Cartdim_get,               DUMPI_Cart_get,
    DUMPI_Cart_rank,                 DUMPI_Cart_coords,
    DUMPI_Graph_neighbors_count,     DUMPI_Graph_neighbors,
    DUMPI_Cart_shift,                DUMPI_Cart_sub,
    DUMPI_Cart_map,                  DUMPI_Graph_map,
    DUMPI_Get_processor_name,        DUMPI_Get_version,
    DUMPI_Errhandler_create,         DUMPI_Errhandler_set,
    DUMPI_Errhandler_get,            DUMPI_Errhandler_free,
    DUMPI_Error_string,              DUMPI_Error_class,
    DUMPI_Wtime,                     DUMPI_Wtick,
    DUMPI_Init,                      DUMPI_Finalize,
    DUMPI_Initialized,               DUMPI_Abort,
    DUMPI_Pcontrol,                  DUMPI_Close_port,
    DUMPI_Comm_accept,               DUMPI_Comm_connect,
    DUMPI_Comm_disconnect,           DUMPI_Comm_get_parent,
    DUMPI_Comm_join,                 DUMPI_Comm_spawn,
    DUMPI_Comm_spawn_multiple,       DUMPI_Lookup_name,
    DUMPI_Open_port,                 DUMPI_Publish_name,
    DUMPI_Unpublish_name,            DUMPI_Accumulate,
    DUMPI_Get,                       DUMPI_Put,
    DUMPI_Win_complete,              DUMPI_Win_create,
    DUMPI_Win_fence,                 DUMPI_Win_free,
    DUMPI_Win_get_group,             DUMPI_Win_lock,
    DUMPI_Win_post,                  DUMPI_Win_start,
    DUMPI_Win_test,                  DUMPI_Win_unlock,
    DUMPI_Win_wait,                  DUMPI_Alltoallw,
    DUMPI_Exscan,                    DUMPI_Add_error_class,
    DUMPI_Add_error_code,            DUMPI_Add_error_string,
    DUMPI_Comm_call_errhandler,      DUMPI_Comm_create_keyval,
    DUMPI_Comm_delete_attr,          DUMPI_Comm_free_keyval,
    DUMPI_Comm_get_attr,             DUMPI_Comm_get_name,
    DUMPI_Comm_set_attr,             DUMPI_Comm_set_name,
    DUMPI_File_call_errhandler,      DUMPI_Grequest_complete,
    DUMPI_Grequest_start,            DUMPI_Init_thread,
    DUMPI_Is_thread_main,            DUMPI_Query_thread,
    DUMPI_Status_set_cancelled,      DUMPI_Status_set_elements,
    DUMPI_Type_create_keyval,        DUMPI_Type_delete_attr,
    DUMPI_Type_dup,                  DUMPI_Type_free_keyval,
    DUMPI_Type_get_attr,             DUMPI_Type_get_contents,
    DUMPI_Type_get_envelope,         DUMPI_Type_get_name,
    DUMPI_Type_set_attr,             DUMPI_Type_set_name,
    DUMPI_Type_match_size,           DUMPI_Win_call_errhandler,
    DUMPI_Win_create_keyval,         DUMPI_Win_delete_attr,
    DUMPI_Win_free_keyval,           DUMPI_Win_get_attr,
    DUMPI_Win_get_name,              DUMPI_Win_set_attr,
    DUMPI_Win_set_name,              DUMPI_Alloc_mem,
    DUMPI_Comm_create_errhandler,    DUMPI_Comm_get_errhandler,
    DUMPI_Comm_set_errhandler,       DUMPI_File_create_errhandler,
    DUMPI_File_get_errhandler,       DUMPI_File_set_errhandler,
    DUMPI_Finalized,                 DUMPI_Free_mem,
    DUMPI_Get_address,               DUMPI_Info_create,
    DUMPI_Info_delete,               DUMPI_Info_dup,
    DUMPI_Info_free,                 DUMPI_Info_get,
    DUMPI_Info_get_nkeys,            DUMPI_Info_get_nthkey,
    DUMPI_Info_get_valuelen,         DUMPI_Info_set,
    DUMPI_Pack_external,             DUMPI_Pack_external_size,
    DUMPI_Request_get_status,        DUMPI_Type_create_darray,
    DUMPI_Type_create_hindexed,      DUMPI_Type_create_hvector,
    DUMPI_Type_create_indexed_block, DUMPI_Type_create_resized,
    DUMPI_Type_create_struct,        DUMPI_Type_create_subarray,
    DUMPI_Type_get_extent,           DUMPI_Type_get_true_extent,
    DUMPI_Unpack_external,           DUMPI_Win_create_errhandler,
    DUMPI_Win_get_errhandler,        DUMPI_Win_set_errhandler,
    DUMPI_File_open,                 DUMPI_File_close,
    DUMPI_File_delete,               DUMPI_File_set_size,
    DUMPI_File_preallocate,          DUMPI_File_get_size,
    DUMPI_File_get_group,            DUMPI_File_get_amode,
    DUMPI_File_set_info,             DUMPI_File_get_info,
    DUMPI_File_set_view,             DUMPI_File_get_view,
    DUMPI_File_read_at,              DUMPI_File_read_at_all,
    DUMPI_File_write_at,             DUMPI_File_write_at_all,
    DUMPI_File_iread_at,             DUMPI_File_iwrite_at,
    DUMPI_File_read,                 DUMPI_File_read_all,
    DUMPI_File_write,                DUMPI_File_write_all,
    DUMPI_File_iread,                DUMPI_File_iwrite,
    DUMPI_File_seek,                 DUMPI_File_get_position,
    DUMPI_File_get_byte_offset,      DUMPI_File_read_shared,
    DUMPI_File_write_shared,         DUMPI_File_iread_shared,
    DUMPI_File_iwrite_shared,        DUMPI_File_read_ordered,
    DUMPI_File_write_ordered,        DUMPI_File_seek_shared,
    DUMPI_File_get_position_shared,  DUMPI_File_read_at_all_begin,
    DUMPI_File_read_at_all_end,      DUMPI_File_write_at_all_begin,
    DUMPI_File_write_at_all_end,     DUMPI_File_read_all_begin,
    DUMPI_File_read_all_end,         DUMPI_File_write_all_begin,
    DUMPI_File_write_all_end,        DUMPI_File_read_ordered_begin,
    DUMPI_File_read_ordered_end,     DUMPI_File_write_ordered_begin,
    DUMPI_File_write_ordered_end,    DUMPI_File_get_type_extent,
    DUMPI_Register_datarep,          DUMPI_File_set_atomicity,
    DUMPI_File_get_atomicity,        DUMPI_File_sync,
    DUMPIO_Test,                     DUMPIO_Wait,
    DUMPIO_Testall,                  DUMPIO_Waitall,
    DUMPIO_Testany,                  DUMPIO_Waitany,
    DUMPIO_Waitsome,                 DUMPIO_Testsome,
    DUMPI_ALL_FUNCTIONS,  /* Sentinel to mark last MPI function */
    /* Special lables to indicate profiled functions. */
    DUMPI_Function_enter,            DUMPI_Function_exit,
    DUMPI_END_OF_STREAM  /* Sentinel to mark end of trace stream */
} dumpi_function;

typedef struct dumpi_send {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_dest  dest;
    /** Argument value before PMPI call */
    dumpi_tag  tag;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_send;

typedef struct dumpi_recv {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_source  source;
    /** Argument value before PMPI call */
    dumpi_tag  tag;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value after PMPI call */
    dumpi_status * status;
} dumpi_recv;

typedef struct dumpi_isend {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_dest  dest;
    /** Argument value before PMPI call */
    int  tag;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value after PMPI call */
    dumpi_request  request;
} dumpi_isend;

typedef struct dumpi_irecv {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_source  source;
    /** Argument value before PMPI call */
    dumpi_tag  tag;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value after PMPI call */
    dumpi_request  request;
} dumpi_irecv;

typedef struct dumpi_init {
    /** Argument value before PMPI call */
    int  argc;
    /**
     * Argument value before PMPI call.
     * Array of length [argc] containing NUL-terminated std::strings.
     */
    char ** argv;
} dumpi_init;

typedef struct dumpi_comm_size {
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value after PMPI call */
    int  size;
} dumpi_comm_size;

typedef struct dumpi_comm_rank {
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value after PMPI call */
    int  rank;
} dumpi_comm_rank;

typedef struct dumpi_waitall {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call.  Array of length [count] */
    dumpi_request * requests;
    /** Argument value after PMPI call.  Array of length [count] */
    dumpi_status * statuses;
} dumpi_waitall;

typedef struct dumpi_wait {
    /** Argument value before PMPI call */
    dumpi_request  request;
    /** Argument value after PMPI call */
    dumpi_status * status;
} dumpi_wait;

typedef struct dumpi_barrier {
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_barrier;

typedef struct dumpi_wtime {
    uint64_t psec;
} dumpi_wtime;

typedef struct dumpi_allreduce {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_op  op;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_allreduce;

typedef struct dumpi_reduce {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_op  op;
    /** Argument value before PMPI call */
    int  root;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_reduce;

typedef struct dumpi_finalize {
    int dummy;
} dumpi_finalize;

typedef struct dumpi_waitsome {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call.  Array of length [count] */
    dumpi_request * requests;
    /** Argument value after PMPI call */
    int  outcount;
    /** Argument value after PMPI call.  Array of length [*outcount] */
    int * indices;
    /** Argument value after PMPI call.  Array of length [*outcount] */
    dumpi_status * statuses;
} dumpi_waitsome;

typedef struct dumpi_bcast {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    int  root;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_bcast;

typedef struct dumpi_group_free {
    /** Argument value before PMPI call */
    dumpi_group  group;
} dumpi_group_free;

typedef struct dumpi_comm_group {
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value after PMPI call */
    dumpi_group  group;
} dumpi_comm_group;

typedef struct dumpi_group_incl {
    /** Argument value before PMPI call */
    dumpi_group  group;
    /** Argument value before PMPI call */
    int count;
    /** Argument value before PMPI call.  Array of length [count] */
    int * ranks;
    /** Argument value after PMPI call */
    dumpi_group  newgroup;
} dumpi_group_incl;

typedef struct dumpi_comm_create {
    /** Argument value before PMPI call */
    dumpi_comm  oldcomm;
    /** Argument value before PMPI call */
    dumpi_group  group;
    /** Argument value after PMPI call */
    dumpi_comm  newcomm;
} dumpi_comm_create;

typedef struct dumpi_type_free {
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
} dumpi_type_free;

typedef struct dumpi_type_commit {
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
} dumpi_type_commit;

typedef struct dumpi_type_vector {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call */
    int  blocklength;
    /** Argument value before PMPI call */
    int  stride;
    /** Argument value before PMPI call */
    dumpi_datatype  oldtype;
    /** Argument value after PMPI call */
    dumpi_datatype  newtype;
} dumpi_type_vector;

typedef struct dumpi_alltoallv {
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  commsize;
    /** Argument value before PMPI call.  Array of length [commsize] */
    int * sendcounts;
    /** Argument value before PMPI call.  Array of length [commsize] */
    int * senddispls;
    /** Argument value before PMPI call */
    dumpi_datatype  sendtype;
    /** Argument value before PMPI call.  Array of length [commsize] */
    int * recvcounts;
    /** Argument value before PMPI call.  Array of length [commsize] */
    int * recvdispls;
    /** Argument value before PMPI call */
    dumpi_datatype  recvtype;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_alltoallv;

typedef struct dumpi_alltoall {
    /** Argument value before PMPI call */
    int  sendcount;
    /** Argument value before PMPI call */
    dumpi_datatype  sendtype;
    /** Argument value before PMPI call */
    int  recvcount;
    /** Argument value before PMPI call */
    dumpi_datatype  recvtype;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_alltoall;

typedef struct dumpi_alltoallw {
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  commsize;
    /** Argument value before PMPI call.  Array of length [commsize] */
    int * sendcounts;
    /** Argument value before PMPI call.  Array of length [commsize] */
    int * senddispls;
    /** Argument value before PMPI call.  Array of length [commsize] */
    dumpi_datatype * sendtypes;
    /** Argument value before PMPI call.  Array of length [commsize] */
    int * recvcounts;
    /** Argument value before PMPI call.  Array of length [commsize] */
    int * recvdispls;
    /** Argument value before PMPI call.  Array of length [commsize] */
    dumpi_datatype * recvtypes;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_alltoallw;

typedef struct dumpi_rsend_init {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_dest  dest;
    /** Argument value before PMPI call */
    dumpi_tag  tag;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value after PMPI call */
    dumpi_request  request;
} dumpi_rsend_init;

typedef struct dumpi_rsend {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_dest  dest;
    /** Argument value before PMPI call */
    dumpi_tag  tag;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_rsend;

typedef struct dumpi_allgather {
    /** Argument value before PMPI call */
    int  sendcount;
    /** Argument value before PMPI call */
    dumpi_datatype  sendtype;
    /** Argument value before PMPI call */
    int  recvcount;
    /** Argument value before PMPI call */
    dumpi_datatype  recvtype;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_allgather;

typedef struct dumpi_op_free {
    /** Argument value before PMPI call */
    dumpi_op  op;
} dumpi_op_free;

typedef struct dumpi_op_create {
    /** Argument value before PMPI call */
    int  commute;
    /** Argument value after PMPI call */
    dumpi_op  op;
} dumpi_op_create;

typedef struct dumpi_initialized {
    /** Argument value after PMPI call */
    int  result;
} dumpi_initialized;

typedef struct dumpi_comm_free {
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_comm_free;

typedef struct dumpi_comm_free_keyval {
    /** Argument value before PMPI call */
    dumpi_comm_keyval  keyval;
} dumpi_comm_free_keyval;

typedef struct dumpi_comm_dup {
    /** Argument value before PMPI call */
    dumpi_comm  oldcomm;
    /** Argument value after PMPI call */
    dumpi_comm  newcomm;
} dumpi_comm_dup;

typedef struct dumpi_gatherv {
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  commrank;
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  commsize;
    /** Argument value before PMPI call */
    int  sendcount;
    /** Argument value before PMPI call */
    dumpi_datatype  sendtype;
    /**
     * Argument value before PMPI call.  Array of length [commsize].
     * Only stored if(commrank==root)
     */
    int * recvcounts;
    /**
     * Argument value before PMPI call.  Array of length [commsize].
     * Only stored if(commrank==root)
     */
    int * displs;
    /** Argument value before PMPI call.  Only stored if(commrank==root) */
    dumpi_datatype  recvtype;
    /** Argument value before PMPI call */
    int  root;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_gatherv;

typedef struct dumpi_gather {
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  commrank;
    /** Argument value before PMPI call */
    int  sendcount;
    /** Argument value before PMPI call */
    dumpi_datatype  sendtype;
    /** Argument value before PMPI call.  Only stored if(commrank==root) */
    int  recvcount;
    /** Argument value before PMPI call.  Only stored if(commrank==root) */
    dumpi_datatype  recvtype;
    /** Argument value before PMPI call */
    int  root;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_gather;

typedef struct dumpi_comm_split {
    /** Argument value before PMPI call */
    dumpi_comm  oldcomm;
    /** Argument value before PMPI call */
    int  color;
    /** Argument value before PMPI call */
    int  key;
    /** Argument value after PMPI call */
    dumpi_comm  newcomm;
} dumpi_comm_split;

typedef struct dumpi_allgatherv {
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  commsize;
    /** Argument value before PMPI call */
    int  sendcount;
    /** Argument value before PMPI call */
    dumpi_datatype  sendtype;
    /** Argument value before PMPI call.  Array of length [commsize] */
    int * recvcounts;
    /** Argument value before PMPI call.  Array of length [commsize] */
    int * displs;
    /** Argument value before PMPI call */
    dumpi_datatype  recvtype;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_allgatherv;

typedef struct dumpi_testall {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call.  Array of length [count] */
    dumpi_request * requests;
    /** Argument value after PMPI call */
    int  flag;
    /**
     * Argument value after PMPI call.  Array of length [count].
     * Only stored if(flag != 0)
     */
    dumpi_status * statuses;
} dumpi_testall;

typedef struct dumpi_pack_size {
    /** Argument value before PMPI call */
    int  incount;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value after PMPI call */
    int  size;
} dumpi_pack_size;

typedef struct dumpi_pack {
    /** Argument value before PMPI call */
    int  incount;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    int  outcount;
    /** Argument value before and after PMPI call. */
    struct { int in, out; }  position;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_pack;

typedef struct dumpi_unpack {
    /** Argument value before PMPI call */
    int  incount;
    /** Argument value before and after PMPI call. */
    struct { int in, out; }  position;
    /** Argument value before PMPI call */
    int  outcount;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_unpack;

typedef struct dumpi_sendrecv {
    /** Argument value before PMPI call */
    int  sendcount;
    /** Argument value before PMPI call */
    dumpi_datatype  sendtype;
    /** Argument value before PMPI call */
    dumpi_dest  dest;
    /** Argument value before PMPI call */
    dumpi_tag  sendtag;
    /** Argument value before PMPI call */
    int  recvcount;
    /** Argument value before PMPI call */
    dumpi_datatype  recvtype;
    /** Argument value before PMPI call */
    dumpi_source  source;
    /** Argument value before PMPI call */
    dumpi_tag  recvtag;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value after PMPI call */
    dumpi_status * status;
} dumpi_sendrecv;

typedef struct dumpi_sendrecv_replace {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_dest  dest;
    /** Argument value before PMPI call */
    dumpi_tag  sendtag;
    /** Argument value before PMPI call */
    dumpi_source  source;
    /** Argument value before PMPI call */
    dumpi_tag  recvtag;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value after PMPI call */
    dumpi_status * status;
} dumpi_sendrecv_replace;

typedef struct dumpi_cart_sub {
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  ndim;
    /** Argument value before PMPI call */
    dumpi_comm  oldcomm;
    /** Argument value before PMPI call.  Array of length [ndim] */
    int * remain_dims;
    /** Argument value after PMPI call */
    dumpi_comm  newcomm;
} dumpi_cart_sub;

typedef struct dumpi_cart_create {
    /** Argument value before PMPI call */
    dumpi_comm  oldcomm;
    /** Argument value before PMPI call */
    int  ndim;
    /** Argument value before PMPI call.  Array of length [ndim] */
    int * dims;
    /** Argument value before PMPI call.  Array of length [ndim] */
    int * periods;
    /** Argument value before PMPI call */
    int  reorder;
    /** Argument value after PMPI call */
    dumpi_comm  newcomm;
} dumpi_cart_create;

typedef struct dumpi_cart_coords {
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  ndim;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value before PMPI call */
    int  rank;
    /** Argument value before PMPI call */
    int  maxdims;
    /** Argument value after PMPI call.  Array of length [ndim] */
    int * coords;
} dumpi_cart_coords;

typedef struct dumpi_cart_rank {
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  ndim;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value before PMPI call.  Array of length [ndim] */
    int * coords;
    /** Argument value after PMPI call */
    int  rank;
} dumpi_cart_rank;

typedef struct dumpi_get_count {
    /** Argument value before PMPI call */
    dumpi_status * status;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value after PMPI call */
    int  count;
} dumpi_get_count;

typedef struct dumpi_attr_get {
    /** Argument value before PMPI call */
    dumpi_comm  comm;
    /** Argument value before PMPI call */
    int  key;
    /** Argument value after PMPI call */
    int  flag;
} dumpi_attr_get;

typedef struct dumpi_type_indexed {
    /** Argument value before PMPI call */
    int  count;
    /** Argument value before PMPI call.  Array of length [count] */
    int * lengths;
    /** Argument value before PMPI call.  Array of length [count] */
    int * indices;
    /** Argument value before PMPI call */
    dumpi_datatype  oldtype;
    /** Argument value after PMPI call */
    dumpi_datatype  newtype;
} dumpi_type_indexed;

typedef struct dumpi_scatter {
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  commrank;
    /** Argument value before PMPI call.  Only stored if(commrank==root) */
    int  sendcount;
    /** Argument value before PMPI call.  Only stored if(commrank==root) */
    dumpi_datatype  sendtype;
    /** Argument value before PMPI call */
    int  recvcount;
    /** Argument value before PMPI call */
    dumpi_datatype  recvtype;
    /** Argument value before PMPI call */
    int  root;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_scatter;

typedef struct dumpi_scatterv {
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  commrank;
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  commsize;
    /**
     * Argument value before PMPI call.  Array of length [commsize].
     * Only stored if(commrank==root)
     */
    int * sendcounts;
    /**
     * Argument value before PMPI call.  Array of length [commsize].
     * Only stored if(commrank==root)
     */
    int * displs;
    /** Argument value before PMPI call */
    dumpi_datatype  sendtype;
    /** Argument value before PMPI call */
    int  recvcount;
    /** Argument value before PMPI call */
    dumpi_datatype  recvtype;
    /** Argument value before PMPI call */
    int  root;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_scatterv;

typedef struct dumpi_reduce_scatter {
    /** Not an MPI argument.  Added to index relevant data in the struct. */
    int  commsize;
    /** Argument value before PMPI call.  Array of length [commsize] */
    int * recvcounts;
    /** Argument value before PMPI call */
    dumpi_datatype  datatype;
    /** Argument value before PMPI call */
    dumpi_op  op;
    /** Argument value before PMPI call */
    dumpi_comm  comm;
} dumpi_reduce_scatter;

unordered_map<uint16_t, string> mpi_function_map {
        {124, "mpi_init"},
        {81, "mpi_comm_size"},
        {82, "mpi_comm_rank"},
        {12, "mpi_irecv"},
        {8, "mpi_isend"},
        {18, "mpi_waitall"},
        {13, "mpi_wait"},
        {52, "mpi_barrier"},
        {122, "mpi_wtime"},
        {65, "mpi_allreduce"},
        {62, "mpi_reduce"},
        {125, "mpi_finalize"}
};

uint32_t get32(FILE* fp){
    uint32_t scratch, retval;
    fread(&scratch, sizeof(uint32_t), 1, fp);
    retval = ntohl(scratch);

    return retval;
}

uint16_t get16(FILE* fp){
    uint16_t scratch, retval;
    fread(&scratch, sizeof(uint16_t), 1, fp);
    retval = ntohs(scratch);

    return retval;
}

uint8_t get8(FILE* fp){
    uint8_t scratch;
    fread(&scratch, sizeof(uint8_t), 1, fp);

    return scratch;
}

void get32arr(FILE* fp, int32_t *count, int32_t **arr){
    int i;
    *count = get32(fp);
    if(*count > 0)
        *arr = (int32_t*)malloc(*count * sizeof(int32_t));
    else
        *arr = nullptr;
    for(i = 0; i < *count; ++i) {
        (*arr)[i] = get32(fp);
    }
}

void get_datatype_arr(FILE* fp, int len, dumpi_datatype* val){
    len = get32(fp);
    val = (dumpi_datatype*)calloc((len)+1, sizeof(dumpi_datatype));
    for(int i=0; i<len; i++){
        val[i] = get16(fp);
    }
}

void handle_header(FILE* fp, int32_t* cpu_time_bias, int32_t* wall_time_bias){
    uint64_t magic_head;

    for(int i=0; i<4; i++){
        magic_head = get16(fp);
        cout << magic_head << endl;
    }

    *cpu_time_bias = get32(fp);
    *wall_time_bias = get32(fp);
    cout << "cpu time bias: " << *cpu_time_bias << " wall_time_bias: " << *wall_time_bias << endl;
}

uint16_t find_next_function(FILE* fp){
    uint16_t mpi_function_name;
    mpi_function_name = get16(fp);
    cout << "mpi function: " << mpi_function_map[mpi_function_name] << endl;

    return mpi_function_name;
}

void get_times(FILE* fp, dumpi_time* cpu, dumpi_time* wall, int32_t cpu_time_offset, int32_t wall_time_offset, uint8_t config_mask){
    if(config_mask & DUMPI_TIME_CPU) {
        cpu->start.sec  = get16(fp) + cpu_time_offset;
        cpu->start.nsec = get32(fp);
        cpu->stop.sec   = get16(fp) + cpu_time_offset;
        cpu->stop.nsec  = get32(fp);
        // cout << "dumpi_time_cpu" << endl;
    }
    else {
        cpu->start.sec = cpu->start.nsec = 0;
        cpu->stop.sec  = cpu->stop.nsec  = 0;
    }
    if(config_mask & DUMPI_TIME_WALL) {
        wall->start.sec  = get16(fp) + wall_time_offset;
        wall->start.nsec = get32(fp);
        wall->stop.sec   = get16(fp) + wall_time_offset;
        wall->stop.nsec  = get32(fp);
        // cout << "dumpi_time_wall" << endl;
    }
    else {
        wall->start.sec = wall->start.nsec = 0;
        wall->stop.sec  = wall->stop.nsec  = 0;
    }
}

uint8_t start_read(FILE* fp, dumpi_time* cpu, dumpi_time* wall, int32_t cpu_time_offset, int32_t wall_time_offset, uint16_t* thread){
    uint8_t config_mask;
    config_mask = get8(fp);
    uint64_t pos = ftello(fp);
    // cout << "cur location before get times: " << ftello(fp) << endl;
    if(config_mask & DUMPI_THREADID_MASK){
        *thread = get16(fp);
    }
    get_times(fp, cpu, wall, cpu_time_offset, wall_time_offset, config_mask);
    // cout << "cur location after get times: " << ftello(fp) << endl;
    fseek(fp, 26-(ftello(fp)-pos), SEEK_CUR);
    // cout << "cur location before reading mpi function actually: " << ftello(fp) << endl;

    return config_mask;
}

dumpi_status* get_statuses(FILE *fp, uint8_t config_mask){
    dumpi_status *statuses = nullptr;
    if(config_mask) {
        int count = get32(fp);
        if(count > 0) {
            statuses = (dumpi_status*)malloc(count * sizeof(dumpi_status));
            for(int i = 0; i < count; i++) {
                statuses[i].bytes = get32(fp);
                statuses[i].source = get32(fp);
                statuses[i].cancelled = get8(fp);
                statuses[i].error = get8(fp);
                statuses[i].tag = get32(fp);
            }
        }
        else {
            statuses = DUMPI_STATUS_IGNORE;
        }
    }

    return statuses;
}

dumpi_init handle_mpi_init(FILE* fp){
    dumpi_init mpi_init{};
    uint32_t argc_count;
    char** argv;
    argc_count = get32(fp);
    argv = (char**)calloc(argc_count+1, sizeof(char*));
    cout << "arg count: " << argc_count << endl;
    for(int i=0; i<argc_count; i++){
        uint32_t arg_len;
        arg_len = get32(fp);
        argv[i] = (char*)calloc((arg_len+1), sizeof(char));
        fread(argv[i], arg_len * sizeof(char), 1, fp);
        cout << argv[i] << endl;
    }
    cout << "cur location after reading mpi_init: " << ftello(fp) << endl;
    mpi_init.argc = argc_count;
    mpi_init.argv = argv;

    return mpi_init;
}


int main()
{
    FILE* fp = fopen("src/mpi-application/test/dumpi-2013.09.09.04.32.40-0000.bin", "r");
    int32_t cpu_time_bias, wall_time_bias;
    uint16_t thread;
    handle_header(fp, &cpu_time_bias, &wall_time_bias);

    uint16_t mpi_function;
    mpi_function = find_next_function(fp);
    uint8_t config_mask;
    dumpi_time *cpu = new dumpi_time, *wall = new dumpi_time;
    config_mask = start_read(fp, cpu, wall, cpu_time_bias, wall_time_bias, &thread);
    cout << "config mask of mpi_init: ";
    for (int i = 7; i >= 0; i--) {
        cout << ((config_mask >> i) & 1);
    }
    cout << endl;
//    cout << "cpu start time: " << cpu->start.sec << "." << cpu->start.nsec
//         << " cpu end time: " << cpu->stop.sec << "." << cpu->stop.nsec << endl;
//    cout << "wall start time: " << wall->start.sec << "." << wall->start.nsec
//         << " wall end time: " << wall->stop.sec << "." << wall->stop.nsec << endl;

    // int count = 0;
    while(mpi_function < DUMPI_END_OF_STREAM) {
        printf("entering at walltime %d.%09d, cputime %d.%09d seconds in thread %d\n",
               wall->start.sec, wall->start.nsec, cpu->start.sec, cpu->start.nsec, thread);
        switch(mpi_function) {
            case DUMPI_Init: {
                dumpi_init mpi_init{};
                mpi_init = handle_mpi_init(fp);
                break;
            }
            case DUMPI_Comm_size: {
                dumpi_comm_size mpi_comm_size{};
                mpi_comm_size.comm = get16(fp);
                mpi_comm_size.size = get32(fp);
                // fread(&mpi_comm_size, sizeof(mpi_comm_size), 1, fp);
                break;
            }
            case DUMPI_Comm_rank: {
                dumpi_comm_rank mpi_comm_rank{};
                mpi_comm_rank.comm = get16(fp);
                mpi_comm_rank.rank = get32(fp);
                break;
            }
            case DUMPI_Irecv: {
                dumpi_irecv mpi_irecv{};
                mpi_irecv.count = get32(fp);
                mpi_irecv.datatype = get16(fp);
                mpi_irecv.source = get32(fp);
                mpi_irecv.tag = get32(fp);
                mpi_irecv.comm = get16(fp);
                mpi_irecv.request = get32(fp);
                break;
            }
            case DUMPI_Isend: {
                dumpi_isend mpi_isend{};
                mpi_isend.count = get32(fp);
                mpi_isend.datatype = get16(fp);
                mpi_isend.dest = get32(fp);
                mpi_isend.tag = get32(fp);
                mpi_isend.comm = get16(fp);
                mpi_isend.request = get32(fp);
                break;
            }
            case DUMPI_Waitall: {
                dumpi_waitall mpi_waitall{};
                mpi_waitall.count = get32(fp);
                get32arr(fp, &(mpi_waitall.count), &(mpi_waitall.requests));
                mpi_waitall.statuses = get_statuses(fp, config_mask);
                break;
            }
            case DUMPI_Wait: {
                dumpi_wait mpi_wait{};
                mpi_wait.request = get32(fp);
                mpi_wait.status = get_statuses(fp, config_mask);
                break;
            }
            case DUMPI_Waitsome: {
                dumpi_waitsome mpi_waitsome{};
                mpi_waitsome.count = get32(fp);
                get32arr(fp, &(mpi_waitsome.count), &(mpi_waitsome.requests));
                mpi_waitsome.outcount = get32(fp);
                get32arr(fp, &(mpi_waitsome.outcount), &(mpi_waitsome.indices));
                mpi_waitsome.statuses = get_statuses(fp, config_mask);
                break;
            }
            case DUMPI_Barrier: {
                dumpi_barrier mpi_barrier{};
                mpi_barrier.comm = get16(fp);
                break;
            }
            case DUMPI_Wtime: { //该函数压根没读取二进制数据，可以忽略
                dumpi_wtime mpi_wtime{};
                break;
            }
            case DUMPI_Allreduce: {
                dumpi_allreduce mpi_allreduce{};
                mpi_allreduce.count = get32(fp);
                mpi_allreduce.datatype = get16(fp);
                mpi_allreduce.op = get8(fp);
                mpi_allreduce.comm = get16(fp);
                break;
            }
            case DUMPI_Reduce: {
                dumpi_reduce mpi_reduce{};
                mpi_reduce.count = get32(fp);
                mpi_reduce.datatype = get16(fp);
                mpi_reduce.op = get8(fp);
                mpi_reduce.root = get32(fp);
                mpi_reduce.comm = get16(fp);
                break;
            }
            case DUMPI_Finalize: {  //该函数也没读取任何二进制数据，可以忽略
                dumpi_finalize mpi_finalize{};
                break;
            }
            case DUMPI_Send: {
                dumpi_send mpi_send{};
                mpi_send.count = get32(fp);
                mpi_send.datatype = get16(fp);
                mpi_send.dest = get32(fp);
                mpi_send.tag = get32(fp);
                mpi_send.comm = get16(fp);
                break;
            }
            case DUMPI_Recv: {
                dumpi_recv mpi_recv{};
                mpi_recv.count = get32(fp);
                mpi_recv.datatype = get16(fp);
                mpi_recv.source = get32(fp);
                mpi_recv.tag = get32(fp);
                mpi_recv.comm = get16(fp);
                mpi_recv.status = get_statuses(fp, config_mask);
                break;
            }
            case DUMPI_Bcast: {
                dumpi_bcast mpi_bcast{};
                mpi_bcast.count = get32(fp);
                mpi_bcast.datatype = get16(fp);
                mpi_bcast.root = get32(fp);
                mpi_bcast.comm = get16(fp);
                break;
            }
            case DUMPI_Group_free: {
                dumpi_group_free mpi_group_free{};
                mpi_group_free.group = get16(fp);
                break;
            }
            case DUMPI_Comm_group: {
                dumpi_comm_group mpi_comm_group{};
                mpi_comm_group.comm = get16(fp);
                mpi_comm_group.group = get16(fp);
                break;
            }
            case DUMPI_Group_incl: {
                dumpi_group_incl mpi_group_incl{};
                mpi_group_incl.group = get16(fp);
                mpi_group_incl.count = get32(fp);
                // TODO 涉及新操作需要验证下正确性
                get32arr(fp, &(mpi_group_incl.count), &(mpi_group_incl.ranks));
                mpi_group_incl.newgroup = get16(fp);
                break;
            }
            case DUMPI_Comm_create: {
                dumpi_comm_create mpi_comm_create{};
                mpi_comm_create.oldcomm = get16(fp);
                mpi_comm_create.group = get16(fp);
                mpi_comm_create.newcomm = get16(fp);
                break;
            }
            case DUMPI_Type_free: {
                dumpi_type_free mpi_type_free{};
                mpi_type_free.datatype = get16(fp);
                break;
            }
            case DUMPI_Type_commit: {
                dumpi_type_commit mpi_type_commit{};
                mpi_type_commit.datatype = get16(fp);
                break;
            }
            case DUMPI_Type_vector: {
                dumpi_type_vector mpi_type_vector{};
                mpi_type_vector.count = get32(fp);
                mpi_type_vector.blocklength = get32(fp);
                mpi_type_vector.stride = get32(fp);
                mpi_type_vector.oldtype = get16(fp);
                mpi_type_vector.newtype = get16(fp);
                break;
            }
            case DUMPI_Alltoall: {
                dumpi_alltoall mpi_alltoall{};
                mpi_alltoall.sendcount = get32(fp);
                mpi_alltoall.sendtype = get16(fp);
                mpi_alltoall.recvcount = get32(fp);
                mpi_alltoall.recvtype = get16(fp);
                mpi_alltoall.comm = get16(fp);
                break;
            }
            // TODO 涉及新操作需要验证下正确性
            case DUMPI_Alltoallv: {
                dumpi_alltoallv mpi_alltoallv{};
                mpi_alltoallv.commsize = get32(fp);
                get32arr(fp, &(mpi_alltoallv.commsize), &(mpi_alltoallv.sendcounts));
                get32arr(fp, &(mpi_alltoallv.commsize), &(mpi_alltoallv.senddispls));
                mpi_alltoallv.sendtype = get16(fp);
                get32arr(fp, &(mpi_alltoallv.commsize), &(mpi_alltoallv.recvcounts));
                get32arr(fp, &(mpi_alltoallv.commsize), &(mpi_alltoallv.recvdispls));
                mpi_alltoallv.recvtype = get16(fp);
                mpi_alltoallv.comm = get16(fp);
                break;
            }
            // TODO 涉及新操作需要验证下正确性
            case DUMPI_Alltoallw: {
                dumpi_alltoallw mpi_alltoallw{};
                mpi_alltoallw.commsize = get32(fp);
                get32arr(fp, &(mpi_alltoallw.commsize), &(mpi_alltoallw.sendcounts));
                get32arr(fp, &(mpi_alltoallw.commsize), &(mpi_alltoallw.senddispls));
                get_datatype_arr(fp, mpi_alltoallw.commsize, mpi_alltoallw.sendtypes);
                get32arr(fp, &(mpi_alltoallw.commsize), &(mpi_alltoallw.recvcounts));
                get32arr(fp, &(mpi_alltoallw.commsize), &(mpi_alltoallw.recvdispls));
                get_datatype_arr(fp, mpi_alltoallw.commsize, mpi_alltoallw.recvtypes);
                mpi_alltoallw.comm = get16(fp);
                break;
            }
            case DUMPI_Rsend_init: {
                dumpi_rsend_init mpi_rsend_init{};
                mpi_rsend_init.count = get32(fp);
                mpi_rsend_init.datatype = get16(fp);
                mpi_rsend_init.dest = get32(fp);
                mpi_rsend_init.tag = get32(fp);
                mpi_rsend_init.comm = get16(fp);
                mpi_rsend_init.request = get32(fp);
                break;
            }
            case DUMPI_Rsend: {
                dumpi_rsend mpi_rsend{};
                mpi_rsend.count = get32(fp);
                mpi_rsend.datatype = get16(fp);
                mpi_rsend.dest = get32(fp);
                mpi_rsend.tag = get32(fp);
                mpi_rsend.comm = get16(fp);
                break;
            }
            case DUMPI_Allgather: {
                dumpi_allgather mpi_allgather{};
                mpi_allgather.sendcount = get32(fp);
                mpi_allgather.sendtype = get16(fp);
                mpi_allgather.recvcount = get32(fp);
                mpi_allgather.recvtype = get16(fp);
                mpi_allgather.comm = get16(fp);
                break;
            }
            case DUMPI_Allgatherv: {
                dumpi_allgatherv mpi_allgatherv{};
                mpi_allgatherv.commsize = get32(fp);
                mpi_allgatherv.sendcount = get32(fp);
                mpi_allgatherv.sendtype = get16(fp);
                get32arr(fp, &(mpi_allgatherv.commsize), &(mpi_allgatherv.recvcounts));
                get32arr(fp, &(mpi_allgatherv.commsize), &(mpi_allgatherv.displs));
                mpi_allgatherv.recvtype = get16(fp);
                mpi_allgatherv.comm = get16(fp);
                break;
            }
            case DUMPI_Gather: {
                dumpi_gather mpi_gather{};
                mpi_gather.commrank = get32(fp);
                mpi_gather.sendcount = get32(fp);
                mpi_gather.sendtype = get16(fp);
                mpi_gather.root = get32(fp);
                mpi_gather.comm = get16(fp);
                if(mpi_gather.commrank == mpi_gather.root)
                    mpi_gather.recvcount = get32(fp);
                if(mpi_gather.commrank == mpi_gather.root)
                    mpi_gather.recvtype = get16(fp);
                break;
            }
            // TODO 涉及新操作需要验证下正确性
            case DUMPI_Gatherv: {
                dumpi_gatherv mpi_gatherv{};
                mpi_gatherv.commrank = get32(fp);
                mpi_gatherv.commsize = get32(fp);
                mpi_gatherv.sendcount = get32(fp);
                mpi_gatherv.sendtype = get16(fp);
                mpi_gatherv.root = get32(fp);
                mpi_gatherv.comm = get16(fp);
                if(mpi_gatherv.commrank == mpi_gatherv.root)
                    get32arr(fp, &(mpi_gatherv.commsize), &(mpi_gatherv.recvcounts));
                if(mpi_gatherv.commrank == mpi_gatherv.root)
                    get32arr(fp, &(mpi_gatherv.commsize), &(mpi_gatherv.displs));
                mpi_gatherv.recvtype = get16(fp);
                break;
            }
            case DUMPI_Op_free: {
                dumpi_op_free mpi_op_free{};
                mpi_op_free.op = get8(fp);
                break;
            }
            case DUMPI_Op_create: {
                dumpi_op_create mpi_op_create{};
                mpi_op_create.commute = get32(fp);
                mpi_op_create.op = get8(fp);
                break;
            }
            case DUMPI_Initialized: {
                dumpi_initialized mpi_initialized{};
                mpi_initialized.result = get32(fp);
                break;
            }
            case DUMPI_Comm_free: {
                dumpi_comm_free mpi_comm_free{};
                mpi_comm_free.comm = get16(fp);
                break;
            }
            case DUMPI_Comm_free_keyval: {
                dumpi_comm_free_keyval mpi_comm_free_keyval{};
                mpi_comm_free_keyval.keyval = get16(fp);
                break;
            }
            case DUMPI_Comm_split: {
                dumpi_comm_split mpi_comm_split{};
                mpi_comm_split.oldcomm = get16(fp);
                mpi_comm_split.color = get32(fp);
                mpi_comm_split.key = get32(fp);
                mpi_comm_split.newcomm = get16(fp);
                break;
            }
            // TODO 涉及新操作需要验证下正确性
            case DUMPI_Testall: {
                dumpi_testall mpi_testall{};
                mpi_testall.count = get32(fp);
                get32arr(fp, &(mpi_testall.count), &(mpi_testall.requests));
                mpi_testall.flag = get32(fp);
                mpi_testall.statuses = get_statuses(fp, config_mask);
                break;
            }
            case DUMPI_Pack_size: {
                dumpi_pack_size mpi_pack_size{};
                mpi_pack_size.incount = get32(fp);
                mpi_pack_size.datatype = get16(fp);
                mpi_pack_size.comm = get16(fp);
                mpi_pack_size.size = get32(fp);
                break;
            }
            case DUMPI_Pack: {
                dumpi_pack mpi_pack{};
                mpi_pack.incount = get32(fp);
                mpi_pack.datatype = get16(fp);
                mpi_pack.outcount = get32(fp);
                mpi_pack.position.in = get32(fp);
                mpi_pack.position.out = get32(fp);
                mpi_pack.comm = get16(fp);
                break;
            }
            case DUMPI_Unpack: {
                dumpi_unpack mpi_unpack{};
                mpi_unpack.incount = get32(fp);
                mpi_unpack.position.in = get32(fp);
                mpi_unpack.position.out = get32(fp);
                mpi_unpack.outcount = get32(fp);
                mpi_unpack.datatype = get16(fp);
                mpi_unpack.comm = get16(fp);
                break;
            }
            case DUMPI_Sendrecv: {
                dumpi_sendrecv mpi_sendrecv{};
                mpi_sendrecv.sendcount = get32(fp);
                mpi_sendrecv.sendtype = get16(fp);
                mpi_sendrecv.dest = get32(fp);
                mpi_sendrecv.sendtag = get32(fp);
                mpi_sendrecv.recvcount = get32(fp);
                mpi_sendrecv.recvtype = get16(fp);
                mpi_sendrecv.source = get32(fp);
                mpi_sendrecv.recvtag = get32(fp);
                mpi_sendrecv.comm = get16(fp);
                mpi_sendrecv.status = get_statuses(fp, config_mask);
                break;
            }
            case DUMPI_Sendrecv_replace: {
                dumpi_sendrecv_replace mpi_sendrecv_replace{};
                mpi_sendrecv_replace.count = get32(fp);
                mpi_sendrecv_replace.datatype = get16(fp);
                mpi_sendrecv_replace.dest = get32(fp);
                mpi_sendrecv_replace.sendtag = get32(fp);
                mpi_sendrecv_replace.source = get32(fp);
                mpi_sendrecv_replace.recvtag = get32(fp);
                mpi_sendrecv_replace.comm = get16(fp);
                mpi_sendrecv_replace.status = get_statuses(fp, config_mask);
                break;
            }
            case DUMPI_Comm_dup: {
                dumpi_comm_dup mpi_comm_dup{};
                mpi_comm_dup.oldcomm = get16(fp);
                mpi_comm_dup.newcomm = get16(fp);
                break;
            }
            case DUMPI_Cart_sub: {
                dumpi_cart_sub mpi_cart_sub{};
                mpi_cart_sub.ndim = get32(fp);
                mpi_cart_sub.oldcomm = get16(fp);
                get32arr(fp, &(mpi_cart_sub.ndim), &(mpi_cart_sub.remain_dims));
                mpi_cart_sub.newcomm = get16(fp);
                break;
            }
            case DUMPI_Cart_create: {
                dumpi_cart_create mpi_cart_create{};
                mpi_cart_create.oldcomm = get16(fp);
                mpi_cart_create.ndim = get32(fp);
                get32arr(fp, &(mpi_cart_create.ndim), &(mpi_cart_create.dims));
                get32arr(fp, &(mpi_cart_create.ndim), &(mpi_cart_create.periods));
                mpi_cart_create.reorder = get32(fp);
                mpi_cart_create.newcomm = get16(fp);
                break;
            }
            case DUMPI_Cart_coords: {
                dumpi_cart_coords mpi_cart_coords{};
                mpi_cart_coords.ndim = get32(fp);
                mpi_cart_coords.comm = get16(fp);
                mpi_cart_coords.rank = get32(fp);
                mpi_cart_coords.maxdims = get32(fp);
                get32arr(fp, &(mpi_cart_coords.ndim), &(mpi_cart_coords.coords));
                break;
            }
            case DUMPI_Cart_rank: {
                dumpi_cart_rank mpi_cart_rank{};
                mpi_cart_rank.ndim = get32(fp);
                mpi_cart_rank.comm = get16(fp);
                get32arr(fp, &(mpi_cart_rank.ndim), &(mpi_cart_rank.coords));
                mpi_cart_rank.rank = get32(fp);
                break;
            }
            case DUMPI_Get_count: {
                dumpi_get_count mpi_get_count{};
                mpi_get_count.status = get_statuses(fp, config_mask);
                mpi_get_count.datatype = get16(fp);
                mpi_get_count.count = get32(fp);
                break;
            }
            case DUMPI_Attr_get: {
                dumpi_attr_get mpi_attr_get{};
                mpi_attr_get.comm = get16(fp);
                mpi_attr_get.key = get32(fp);
                mpi_attr_get.flag = get32(fp);
                break;
            }
            case DUMPI_Type_indexed: {
                dumpi_type_indexed mpi_type_indexed{};
                mpi_type_indexed.count = get32(fp);
                get32arr(fp, &(mpi_type_indexed.count), &(mpi_type_indexed.lengths));
                get32arr(fp, &(mpi_type_indexed.count), &(mpi_type_indexed.indices));
                mpi_type_indexed.oldtype = get16(fp);
                mpi_type_indexed.newtype = get16(fp);
                break;
            }
            case DUMPI_Scatter: {
                dumpi_scatter mpi_scatter{};
                mpi_scatter.commrank = get32(fp);
                mpi_scatter.recvcount = get32(fp);
                mpi_scatter.recvtype = get16(fp);
                mpi_scatter.root = get32(fp);
                mpi_scatter.comm = get16(fp);
                if(mpi_scatter.commrank == mpi_scatter.root)
                    mpi_scatter.sendcount = get32(fp);
                if(mpi_scatter.commrank == mpi_scatter.root)
                    mpi_scatter.sendtype = get16(fp);
                break;
            }
            case DUMPI_Scatterv: {
                dumpi_scatterv mpi_scatterv{};
                mpi_scatterv.commrank = get32(fp);
                mpi_scatterv.commsize = get32(fp);
                mpi_scatterv.sendtype = get16(fp);
                mpi_scatterv.recvcount = get32(fp);
                mpi_scatterv.recvtype = get16(fp);
                mpi_scatterv.root = get32(fp);
                mpi_scatterv.comm = get16(fp);
                if(mpi_scatterv.commrank == mpi_scatterv.root)
                    get32arr(fp, &(mpi_scatterv.commsize), &(mpi_scatterv.sendcounts));
                if(mpi_scatterv.commrank == mpi_scatterv.root)
                    get32arr(fp, &(mpi_scatterv.commsize), &(mpi_scatterv.displs));
                break;
            }
            case DUMPI_Reduce_scatter: {
                dumpi_reduce_scatter mpi_reduce_scatter{};
                mpi_reduce_scatter.commsize = get32(fp);
                get32arr(fp, &(mpi_reduce_scatter.commsize), &(mpi_reduce_scatter.recvcounts));
                mpi_reduce_scatter.datatype = get16(fp);
                mpi_reduce_scatter.op = get8(fp);
                mpi_reduce_scatter.comm = get16(fp);
                break;
            }
            default: {
                cout << "No such mpi function!!!" << endl;
                return -1;
            }
        }
        printf("returning at walltime %d.%09d, cputime %d.%09d seconds in thread %d\n",
               wall->stop.sec, wall->stop.nsec, cpu->stop.sec, cpu->stop.nsec, thread);

        cout << "cur location after reading the function: " << ftello(fp) << endl;
        mpi_function = find_next_function(fp);
        if(mpi_function < DUMPI_END_OF_STREAM)
            config_mask = start_read(fp, cpu, wall, cpu_time_bias, wall_time_bias, &thread);
        // count++;
    }

    delete cpu;
    delete wall;
    fclose(fp);

    return 0;
}