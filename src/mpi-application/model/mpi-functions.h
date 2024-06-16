#ifndef NS3_MPI_FUNCTIONS_H
#define NS3_MPI_FUNCTIONS_H

#include <filesystem>
#include <queue>

#include "ns3/mpi-application.h"

/** A reasonably compact type handle for an MPI comm */
using dumpi_comm = int16_t;
/** A reasonably compact type handle for an MPI datatype */
using dumpi_datatype = int16_t;
/** A reasonably compact type handle for an MPI dest */
using dumpi_dest = int32_t;
/** A reasonably compact type handle for an MPI group */
using dumpi_group = int16_t;
/** A reasonably compact type handle for an MPI comm_keyval */
using dumpi_comm_keyval = int16_t;
/** A reasonably compact type handle for an MPI op */
using dumpi_op = uint8_t;
/** A reasonably compact type handle for an MPI source */
using dumpi_source = int32_t;
/** Tag value was converted from 16-bit to 32-bit value in 0.6.5
 *  The value has always been stored as a 32-bit value in the file */
using dumpi_tag = int32_t;
/** A reasonably compact type handle for an MPI request */
using dumpi_request = int32_t;

enum dumpi_function {
    DUMPI_Send = 0, DUMPI_Recv,
    DUMPI_Get_count, DUMPI_Bsend,
    DUMPI_Ssend, DUMPI_Rsend,
    DUMPI_Buffer_attach, DUMPI_Buffer_detach,
    DUMPI_Isend, DUMPI_Ibsend,
    DUMPI_Issend, DUMPI_Irsend,
    DUMPI_Irecv, DUMPI_Wait,
    DUMPI_Test, DUMPI_Request_free,
    DUMPI_Waitany, DUMPI_Testany,
    DUMPI_Waitall, DUMPI_Testall,
    DUMPI_Waitsome, DUMPI_Testsome,
    DUMPI_Iprobe, DUMPI_Probe,
    DUMPI_Cancel, DUMPI_Test_cancelled,
    DUMPI_Send_init, DUMPI_Bsend_init,
    DUMPI_Ssend_init, DUMPI_Rsend_init,
    DUMPI_Recv_init, DUMPI_Start,
    DUMPI_Startall, DUMPI_Sendrecv,
    DUMPI_Sendrecv_replace, DUMPI_Type_contiguous,
    DUMPI_Type_vector, DUMPI_Type_hvector,
    DUMPI_Type_indexed, DUMPI_Type_hindexed,
    DUMPI_Type_struct, DUMPI_Address,
    DUMPI_Type_extent, DUMPI_Type_size,
    DUMPI_Type_lb, DUMPI_Type_ub,
    DUMPI_Type_commit, DUMPI_Type_free,
    DUMPI_Get_elements, DUMPI_Pack,
    DUMPI_Unpack, DUMPI_Pack_size,
    DUMPI_Barrier, DUMPI_Bcast,
    DUMPI_Gather, DUMPI_Gatherv,
    DUMPI_Scatter, DUMPI_Scatterv,
    DUMPI_Allgather, DUMPI_Allgatherv,
    DUMPI_Alltoall, DUMPI_Alltoallv,
    DUMPI_Reduce, DUMPI_Op_create,
    DUMPI_Op_free, DUMPI_Allreduce,
    DUMPI_Reduce_scatter, DUMPI_Scan,
    DUMPI_Group_size, DUMPI_Group_rank,
    DUMPI_Group_translate_ranks, DUMPI_Group_compare,
    DUMPI_Comm_group, DUMPI_Group_union,
    DUMPI_Group_intersection, DUMPI_Group_difference,
    DUMPI_Group_incl, DUMPI_Group_excl,
    DUMPI_Group_range_incl, DUMPI_Group_range_excl,
    DUMPI_Group_free, DUMPI_Comm_size,
    DUMPI_Comm_rank, DUMPI_Comm_compare,
    DUMPI_Comm_dup, DUMPI_Comm_create,
    DUMPI_Comm_split, DUMPI_Comm_free,
    DUMPI_Comm_test_inter, DUMPI_Comm_remote_size,
    DUMPI_Comm_remote_group, DUMPI_Intercomm_create,
    DUMPI_Intercomm_merge, DUMPI_Keyval_create,
    DUMPI_Keyval_free, DUMPI_Attr_put,
    DUMPI_Attr_get, DUMPI_Attr_delete,
    DUMPI_Topo_test, DUMPI_Cart_create,
    DUMPI_Dims_create, DUMPI_Graph_create,
    DUMPI_Graphdims_get, DUMPI_Graph_get,
    DUMPI_Cartdim_get, DUMPI_Cart_get,
    DUMPI_Cart_rank, DUMPI_Cart_coords,
    DUMPI_Graph_neighbors_count, DUMPI_Graph_neighbors,
    DUMPI_Cart_shift, DUMPI_Cart_sub,
    DUMPI_Cart_map, DUMPI_Graph_map,
    DUMPI_Get_processor_name, DUMPI_Get_version,
    DUMPI_Errhandler_create, DUMPI_Errhandler_set,
    DUMPI_Errhandler_get, DUMPI_Errhandler_free,
    DUMPI_Error_string, DUMPI_Error_class,
    DUMPI_Wtime, DUMPI_Wtick,
    DUMPI_Init, DUMPI_Finalize,
    DUMPI_Initialized, DUMPI_Abort,
    DUMPI_Pcontrol, DUMPI_Close_port,
    DUMPI_Comm_accept, DUMPI_Comm_connect,
    DUMPI_Comm_disconnect, DUMPI_Comm_get_parent,
    DUMPI_Comm_join, DUMPI_Comm_spawn,
    DUMPI_Comm_spawn_multiple, DUMPI_Lookup_name,
    DUMPI_Open_port, DUMPI_Publish_name,
    DUMPI_Unpublish_name, DUMPI_Accumulate,
    DUMPI_Get, DUMPI_Put,
    DUMPI_Win_complete, DUMPI_Win_create,
    DUMPI_Win_fence, DUMPI_Win_free,
    DUMPI_Win_get_group, DUMPI_Win_lock,
    DUMPI_Win_post, DUMPI_Win_start,
    DUMPI_Win_test, DUMPI_Win_unlock,
    DUMPI_Win_wait, DUMPI_Alltoallw,
    DUMPI_Exscan, DUMPI_Add_error_class,
    DUMPI_Add_error_code, DUMPI_Add_error_string,
    DUMPI_Comm_call_errhandler, DUMPI_Comm_create_keyval,
    DUMPI_Comm_delete_attr, DUMPI_Comm_free_keyval,
    DUMPI_Comm_get_attr, DUMPI_Comm_get_name,
    DUMPI_Comm_set_attr, DUMPI_Comm_set_name,
    DUMPI_File_call_errhandler, DUMPI_Grequest_complete,
    DUMPI_Grequest_start, DUMPI_Init_thread,
    DUMPI_Is_thread_main, DUMPI_Query_thread,
    DUMPI_Status_set_cancelled, DUMPI_Status_set_elements,
    DUMPI_Type_create_keyval, DUMPI_Type_delete_attr,
    DUMPI_Type_dup, DUMPI_Type_free_keyval,
    DUMPI_Type_get_attr, DUMPI_Type_get_contents,
    DUMPI_Type_get_envelope, DUMPI_Type_get_name,
    DUMPI_Type_set_attr, DUMPI_Type_set_name,
    DUMPI_Type_match_size, DUMPI_Win_call_errhandler,
    DUMPI_Win_create_keyval, DUMPI_Win_delete_attr,
    DUMPI_Win_free_keyval, DUMPI_Win_get_attr,
    DUMPI_Win_get_name, DUMPI_Win_set_attr,
    DUMPI_Win_set_name, DUMPI_Alloc_mem,
    DUMPI_Comm_create_errhandler, DUMPI_Comm_get_errhandler,
    DUMPI_Comm_set_errhandler, DUMPI_File_create_errhandler,
    DUMPI_File_get_errhandler, DUMPI_File_set_errhandler,
    DUMPI_Finalized, DUMPI_Free_mem,
    DUMPI_Get_address, DUMPI_Info_create,
    DUMPI_Info_delete, DUMPI_Info_dup,
    DUMPI_Info_free, DUMPI_Info_get,
    DUMPI_Info_get_nkeys, DUMPI_Info_get_nthkey,
    DUMPI_Info_get_valuelen, DUMPI_Info_set,
    DUMPI_Pack_external, DUMPI_Pack_external_size,
    DUMPI_Request_get_status, DUMPI_Type_create_darray,
    DUMPI_Type_create_hindexed, DUMPI_Type_create_hvector,
    DUMPI_Type_create_indexed_block, DUMPI_Type_create_resized,
    DUMPI_Type_create_struct, DUMPI_Type_create_subarray,
    DUMPI_Type_get_extent, DUMPI_Type_get_true_extent,
    DUMPI_Unpack_external, DUMPI_Win_create_errhandler,
    DUMPI_Win_get_errhandler, DUMPI_Win_set_errhandler,
    DUMPI_File_open, DUMPI_File_close,
    DUMPI_File_delete, DUMPI_File_set_size,
    DUMPI_File_preallocate, DUMPI_File_get_size,
    DUMPI_File_get_group, DUMPI_File_get_amode,
    DUMPI_File_set_info, DUMPI_File_get_info,
    DUMPI_File_set_view, DUMPI_File_get_view,
    DUMPI_File_read_at, DUMPI_File_read_at_all,
    DUMPI_File_write_at, DUMPI_File_write_at_all,
    DUMPI_File_iread_at, DUMPI_File_iwrite_at,
    DUMPI_File_read, DUMPI_File_read_all,
    DUMPI_File_write, DUMPI_File_write_all,
    DUMPI_File_iread, DUMPI_File_iwrite,
    DUMPI_File_seek, DUMPI_File_get_position,
    DUMPI_File_get_byte_offset, DUMPI_File_read_shared,
    DUMPI_File_write_shared, DUMPI_File_iread_shared,
    DUMPI_File_iwrite_shared, DUMPI_File_read_ordered,
    DUMPI_File_write_ordered, DUMPI_File_seek_shared,
    DUMPI_File_get_position_shared, DUMPI_File_read_at_all_begin,
    DUMPI_File_read_at_all_end, DUMPI_File_write_at_all_begin,
    DUMPI_File_write_at_all_end, DUMPI_File_read_all_begin,
    DUMPI_File_read_all_end, DUMPI_File_write_all_begin,
    DUMPI_File_write_all_end, DUMPI_File_read_ordered_begin,
    DUMPI_File_read_ordered_end, DUMPI_File_write_ordered_begin,
    DUMPI_File_write_ordered_end, DUMPI_File_get_type_extent,
    DUMPI_Register_datarep, DUMPI_File_set_atomicity,
    DUMPI_File_get_atomicity, DUMPI_File_sync,
    DUMPIO_Test, DUMPIO_Wait,
    DUMPIO_Testall, DUMPIO_Waitall,
    DUMPIO_Testany, DUMPIO_Waitany,
    DUMPIO_Waitsome, DUMPIO_Testsome,
    DUMPI_ALL_FUNCTIONS,  /* Sentinel to mark last MPI function */
    /* Special lables to indicate profiled functions. */
    DUMPI_Function_enter, DUMPI_Function_exit,
    DUMPI_END_OF_STREAM  /* Sentinel to mark end of trace stream */
};

enum dumpi_datatypes {
    DUMPI_DATATYPE_ERROR = 0,
    DUMPI_DATATYPE_NULL,
    DUMPI_CHAR,
    DUMPI_SIGNED_CHAR,
    DUMPI_UNSIGNED_CHAR,
    DUMPI_BYTE,
    DUMPI_WCHAR,
    DUMPI_SHORT,
    DUMPI_UNSIGNED_SHORT,
    DUMPI_INT,
    DUMPI_UNSIGNED,
    DUMPI_LONG,
    DUMPI_UNSIGNED_LONG,
    DUMPI_FLOAT,
    DUMPI_DOUBLE,
    DUMPI_LONG_DOUBLE,
    DUMPI_LONG_LONG_INT,
    DUMPI_UNSIGNED_LONG_LONG,
    DUMPI_LONG_LONG,
    DUMPI_PACKED,
    DUMPI_LB,
    DUMPI_UB,
    DUMPI_FLOAT_INT,
    DUMPI_DOUBLE_INT,
    DUMPI_LONG_INT,
    DUMPI_SHORT_INT,
    DUMPI_2INT,
    DUMPI_LONG_DOUBLE_INT,
    DUMPI_FIRST_USER_DATATYPE
};

template<typename T, typename ...U>
decltype(auto) type_mapping(dumpi_datatype type, T &&f, U &&...u) {
    switch (type) {
        case DUMPI_CHAR: {
            return f.template operator()<char>(std::forward<U>(u)...);
        }
        case DUMPI_SIGNED_CHAR: {
            return f.template operator()<signed char>(std::forward<U>(u)...);
        }
        case DUMPI_UNSIGNED_CHAR: {
            return f.template operator()<unsigned char>(std::forward<U>(u)...);
        }
        case DUMPI_BYTE: {
            return f.template operator()<unsigned char>(std::forward<U>(u)...);
        }
        case DUMPI_WCHAR: {
            return f.template operator()<wchar_t>(std::forward<U>(u)...);
        }
        case DUMPI_SHORT: {
            return f.template operator()<short>(std::forward<U>(u)...);
        }
        case DUMPI_UNSIGNED_SHORT: {
            return f.template operator()<unsigned short>(std::forward<U>(u)...);
        }
        case DUMPI_INT: {
            return f.template operator()<int>(std::forward<U>(u)...);
        }
        case DUMPI_UNSIGNED: {
            return f.template operator()<unsigned>(std::forward<U>(u)...);
        }
        case DUMPI_LONG: {
            return f.template operator()<long>(std::forward<U>(u)...);
        }
        case DUMPI_UNSIGNED_LONG: {
            return f.template operator()<unsigned long>(std::forward<U>(u)...);
        }
        case DUMPI_FLOAT: {
            return f.template operator()<float>(std::forward<U>(u)...);
        }
        case DUMPI_DOUBLE: {
            return f.template operator()<double>(std::forward<U>(u)...);
        }
        case DUMPI_LONG_DOUBLE: {
            return f.template operator()<long double>(std::forward<U>(u)...);
        }
        case DUMPI_LONG_LONG_INT: {
            return f.template operator()<long long int>(std::forward<U>(u)...);
        }
        case DUMPI_UNSIGNED_LONG_LONG: {
            return f.template operator()<uint64_t>(std::forward<U>(u)...);
        }
        case DUMPI_LONG_LONG: {
            return f.template operator()<int64_t>(std::forward<U>(u)...);
        }
        case DUMPI_LONG_INT: {
            return f.template operator()<long int>(std::forward<U>(u)...);
        }
        case DUMPI_SHORT_INT: {
            return f.template operator()<short int>(std::forward<U>(u)...);
        }
        case DUMPI_PACKED: {
            // 不清楚该数据类型指的是什么，只知道该类型占 1B 内存空间
            return f.template operator()<unsigned char>(std::forward<U>(u)...);
        }
        case DUMPI_FLOAT_INT: {
            return f.template operator()<uint64_t>(std::forward<U>(u)...);
        }
        case DUMPI_DOUBLE_INT: {
            return f.template operator()<uint64_t>(std::forward<U>(u)...);
        }
        case DUMPI_2INT: {
            return f.template operator()<uint64_t>(std::forward<U>(u)...);
        }
        case DUMPI_LONG_DOUBLE_INT: {
            return f.template operator()<uint128_t>(std::forward<U>(u)...);
        }
        case DUMPI_FIRST_USER_DATATYPE: {
            return f.template operator()<uint64_t>(std::forward<U>(u)...);
        }

        default: {
            throw std::runtime_error{"No such datatype!!!"};
        }
    }
}

struct dumpi_status {
    int32_t bytes;
    int32_t source;
    int32_t tag;
    int8_t cancelled;
    int8_t error;
};

enum dumpi_timestamps {
    DUMPI_TIME_NONE = 0, DUMPI_TIME_CPU = (1 << 2), DUMPI_TIME_WALL = (1 << 3),
    DUMPI_TIME_FULL = (DUMPI_TIME_CPU | DUMPI_TIME_WALL)
};

struct dumpi_clock {
    int32_t sec;
    int32_t nsec;
};

struct dumpi_time {
    dumpi_clock start;   /* stored as 6 bytes */
    dumpi_clock stop;    /* stored as 6 bytes */
};

struct pack_position {
    int in;
    int out;
};

namespace ns3 {
    /*
     * parse trace
     * success: return the unique_ptr<MPIFunction> queue with all mpi functions
     * failure: throw runtime_error{"No such mpi function!!!"}
     */
    std::queue<ns3::MPIFunction> parse_trace(std::filesystem::path trace_path);

    std::queue<ns3::MPIFunction> parse_trace(std::filesystem::path trace_dir, MPIRankIDType rank_id);

    std::vector<std::queue<ns3::MPIFunction>> parse_traces(std::filesystem::path trace_dir);

    struct MPI_Alltoall {
        int sendcount;
        dumpi_datatype sendtype;
        int recvcount;
        dumpi_datatype recvtype;
        dumpi_comm comm;
    };

    struct MPI_Alltoallv {
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int commsize;
        /** Array of length [commsize] */
        int *sendcounts;
        /** Array of length [commsize] */
        int *senddispls;
        dumpi_datatype sendtype;
        /** Array of length [commsize] */
        int *recvcounts;
        /** Array of length [commsize] */
        int *recvdispls;
        dumpi_datatype recvtype;
        dumpi_comm comm;
    };

    struct MPI_Alltoallw {
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int commsize;
        /** Array of length [commsize] */
        int *sendcounts;
        /** Array of length [commsize] */
        int *senddispls;
        /** Array of length [commsize] */
        dumpi_datatype *sendtypes;
        /** Array of length [commsize] */
        int *recvcounts;
        /** Array of length [commsize] */
        int *recvdispls;
        /** Array of length [commsize] */
        dumpi_datatype *recvtypes;
        dumpi_comm comm;
    };

    struct MPI_Rsend {
        int count;
        dumpi_datatype datatype;
        dumpi_dest dest;
        dumpi_tag tag;
        dumpi_comm comm;
    };

    struct MPI_Allgather {
        int sendcount;
        dumpi_datatype sendtype;
        int recvcount;
        dumpi_datatype recvtype;
        dumpi_comm comm;
    };

    struct MPI_Allgatherv {
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int commsize;
        int sendcount;
        dumpi_datatype sendtype;
        /** Array of length [commsize] */
        int *recvcounts;
        /** Array of length [commsize] */
        int *displs;
        dumpi_datatype recvtype;
        dumpi_comm comm;
    };

    struct MPI_Gather {
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int commrank;
        int sendcount;
        dumpi_datatype sendtype;
        /** Only stored if(commrank==root) */
        int recvcount;
        /** Only stored if(commrank==root) */
        dumpi_datatype recvtype;
        int root;
        dumpi_comm comm;
    };

    struct MPI_Gatherv {
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int commrank;
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int commsize;
        int sendcount;
        dumpi_datatype sendtype;
        /**
         * Array of length [commsize].
         * Only stored if(commrank==root)
         */
        int *recvcounts;
        /**
         * Array of length [commsize].
         * Only stored if(commrank==root)
         */
        int *displs;
        /** Only stored if(commrank==root) */
        dumpi_datatype recvtype;
        int root;
        dumpi_comm comm;
    };

    struct MPI_Initialized {
        int result;
    };

    struct MPI_Testall {
        int count;
        /** Array of length [count] */
        dumpi_request *requests;
        int flag;

        /**
         * Array of length [count]
         * Only stored if(flag != 0)
         */
        dumpi_status *statuses;
    };

    struct MPI_Sendrecv {
        int sendcount;
        dumpi_datatype sendtype;
        dumpi_dest dest;
        dumpi_tag sendtag;
        int recvcount;
        dumpi_datatype recvtype;
        dumpi_source source;
        dumpi_tag recvtag;
        dumpi_comm comm;
        dumpi_status *status;
    };

    struct MPI_Sendrecv_Replace {
        int count;
        dumpi_datatype datatype;
        dumpi_dest dest;
        dumpi_tag sendtag;
        dumpi_source source;
        dumpi_tag recvtag;
        dumpi_comm comm;
        dumpi_status *status;
    };

    struct MPI_Scatter {
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int commrank;
        /** Only stored if(commrank==root) */
        int sendcount;
        /** Only stored if(commrank==root) */
        dumpi_datatype sendtype;
        int recvcount;
        dumpi_datatype recvtype;
        int root;
        dumpi_comm comm;
    };

    struct MPI_Scatterv {
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int commrank;
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int commsize;
        /**
         * Array of length [commsize]
         * Only stored if(commrank==root)
         */
        int *sendcounts;
        /**
         * Array of length [commsize]
         * Only stored if(commrank==root)
         */
        int *displs;
        dumpi_datatype sendtype;
        int recvcount;
        dumpi_datatype recvtype;
        int root;
        dumpi_comm comm;
    };

    struct MPI_Reduce_Scatter {
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int commsize;
        /** Array of length [commsize] */
        int *recvcounts;
        dumpi_datatype datatype;
        dumpi_op op;
        dumpi_comm comm;
    };

    struct MPI_Group_Free {
        dumpi_group group;
    };

    struct MPI_Comm_Group {
        dumpi_comm comm;
        dumpi_group group;
    };

    struct MPI_Group_Incl {
        dumpi_group group;
        int count;
        /** Array of length [count] */
        int *ranks;
        dumpi_group newgroup;
    };

    struct MPI_Comm_Create {
        dumpi_comm oldcomm;
        dumpi_group group;
        dumpi_comm newcomm;
    };

    struct MPI_Type_Free {
        dumpi_datatype datatype;
    };

    struct MPI_Type_Commit {
        dumpi_datatype datatype;
    };

    struct MPI_Type_Vector {
        int count;
        int blocklength;
        int stride;
        dumpi_datatype oldtype;
        dumpi_datatype newtype;
    };

    struct MPI_Rsend_Init {
        int count;
        dumpi_datatype datatype;
        dumpi_dest dest;
        dumpi_tag tag;
        dumpi_comm comm;
        dumpi_request request;
    };

    struct MPI_Op_Free {
        dumpi_op op;
    };

    struct MPI_Op_Create {
        int commute;
        dumpi_op op;
    };

    struct MPI_Comm_Free {
        dumpi_comm comm;
    };

    struct MPI_Comm_Free_Keyval {
        dumpi_comm_keyval keyval;
    };

    struct MPI_Comm_Dup {
        dumpi_comm oldcomm;
        dumpi_comm newcomm;
    };

    struct MPI_Comm_Split {
        dumpi_comm oldcomm;
        int color;
        int key;
        dumpi_comm newcomm;
    };

    struct MPI_Pack_Size {
        int incount;
        dumpi_datatype datatype;
        dumpi_comm comm;
        int size;
    };

    struct MPI_Pack {
        int incount;
        dumpi_datatype datatype;
        int outcount;
        pack_position position;
        dumpi_comm comm;
    };

    struct MPI_Unpack {
        int incount;
        pack_position position;
        int outcount;
        dumpi_datatype datatype;
        dumpi_comm comm;
    };

    struct MPI_Cart_Sub {
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int ndim;
        dumpi_comm oldcomm;
        /** Array of length [ndim] */
        int *remain_dims;
        dumpi_comm newcomm;
    };

    struct MPI_Cart_Create {
        dumpi_comm oldcomm;
        int ndim;
        /** Array of length [ndim] */
        int *dims;
        /** Array of length [ndim] */
        int *periods;
        int reorder;
        dumpi_comm newcomm;
    };

    struct MPI_Cart_Coords {
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int ndim;
        dumpi_comm comm;
        int rank;
        int maxdims;
        /** Array of length [ndim] */
        int *coords;
    };

    struct MPI_Cart_Rank {
        /** Not an MPI argument.  Added to index relevant data in the struct. */
        int ndim;
        dumpi_comm comm;
        /** Array of length [ndim] */
        int *coords;
        int rank;
    };

    struct MPI_Get_Count {
        dumpi_status *status;
        dumpi_datatype datatype;
        int count;
    };

    struct MPI_Attr_Get {
        dumpi_comm comm;
        int key;
        int flag;
    };

    struct MPI_Type_Indexed {
        int count;
        /** Array of length [count] */
        int *lengths;
        /** Array of length [count] */
        int *indices;
        dumpi_datatype oldtype;
        dumpi_datatype newtype;
    };

}

#endif // NS3_MPI_FUNCTIONS_H
