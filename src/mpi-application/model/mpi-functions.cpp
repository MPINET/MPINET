#include "mpi-functions.h"

#include <string>
#include <arpa/inet.h>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

#pragma pack (1)    //取消结构体字节对齐， #pragma pack () 则恢复原来的字节对齐规则
#define DUMPI_ANY_TAG -1
#define DUMPI_STATUS_IGNORE nullptr
#define DUMPI_THREADID_MASK (1<<6)

uint32_t get32(std::fstream &fp) {
    uint32_t scratch;
    uint32_t retval;
    fp.read((char *) (&scratch), sizeof(uint32_t));
    retval = ntohl(scratch);
    return retval;
}

uint16_t get16(std::fstream &fp) {
    uint16_t scratch;
    uint16_t retval;
    fp.read((char *) (&scratch), sizeof(uint16_t));
    retval = ntohs(scratch);
    return retval;
}

uint8_t get8(std::fstream &fp) {
    uint8_t scratch;
    fp.read((char *) (&scratch), sizeof(uint8_t));
    return scratch;
}

std::vector<int32_t> get32arr(std::fstream &fp) {
    auto count = get32(fp);
    std::vector<int32_t> arr;
    for (uint32_t i = 0; i < count; i++) {
        arr.push_back(get32(fp));
    }
    return arr;
}

std::vector<dumpi_datatype> get_datatype_arr(std::fstream &fp) {
    auto len = get32(fp);
    std::vector<dumpi_datatype> val;
    for (uint32_t i = 0; i < len; i++) {
        val.push_back(get16(fp));
    }
    return val;
}

int64_t get_interval(int32_t start_sec, int32_t start_nsec, int32_t stop_sec, int32_t stop_nsec) {
    int64_t interval = 0;
    if (start_sec == stop_sec) {
        interval = stop_nsec - start_nsec;
    } else {
        interval = (1e+9 - start_nsec) + (1e+9 * (stop_sec - start_sec - 1)) + stop_nsec;
    }
    return interval;
}

void handle_header(std::fstream &fp, int32_t *cpu_time_bias, int32_t *wall_time_bias) {
    for (int i = 0; i < 4; i++) {
        get16(fp); // magic_head
    }
    *cpu_time_bias = get32(fp);
    *wall_time_bias = get32(fp);
}

uint16_t find_next_function(std::fstream &fp) {
    uint16_t mpi_function_name;
    mpi_function_name = get16(fp);
    return mpi_function_name;
}

uint8_t start_read(std::fstream &fp, dumpi_time *cpu, dumpi_time *wall, int32_t cpu_time_offset, int32_t wall_time_offset, uint16_t *thread) {
    uint8_t config_mask;
    config_mask = get8(fp);
    uint64_t pos = fp.tellg();
    if (config_mask & DUMPI_THREADID_MASK) {
        *thread = get16(fp);
    }

    if (config_mask & DUMPI_TIME_CPU) {
        (*cpu).start.sec = get16(fp) + cpu_time_offset;
        (*cpu).start.nsec = get32(fp);
        (*cpu).stop.sec = get16(fp) + cpu_time_offset;
        (*cpu).stop.nsec = get32(fp);
    } else {
        (*cpu).start.sec = (*cpu).start.nsec = 0;
        (*cpu).stop.sec = (*cpu).stop.nsec = 0;
    }
    if (config_mask & DUMPI_TIME_WALL) {
        (*wall).start.sec = get16(fp) + wall_time_offset;
        (*wall).start.nsec = get32(fp);
        (*wall).stop.sec = get16(fp) + wall_time_offset;
        (*wall).stop.nsec = get32(fp);
    } else {
        (*wall).start.sec = (*wall).start.nsec = 0;
        (*wall).stop.sec = (*wall).stop.nsec = 0;
    }
    // get_times(fp, cpu, wall, cpu_time_offset, wall_time_offset, config_mask);
    fp.seekg(26 - ((uint64_t) fp.tellg() - pos), std::ios::cur);

    return config_mask;
}

std::vector<dumpi_status> get_statuses(std::fstream &fp, uint8_t config_mask) {
    std::vector<dumpi_status> statuses;
    if (config_mask) {
        int count = get32(fp);
        if (count > 0) {
            dumpi_status tmp_status;
            for (int i = 0; i < count; i++) {
                tmp_status.bytes = get32(fp);
                tmp_status.source = get32(fp);
                tmp_status.cancelled = get8(fp);
                tmp_status.error = get8(fp);
                tmp_status.tag = get32(fp);
                statuses.push_back(tmp_status);
            }
        }
    }

    return statuses;
}

std::queue<ns3::MPIFunction> ns3::parse_trace(std::filesystem::path trace_path) {
    std::queue<MPIFunction> mpi_functions;
    std::fstream fp;
    fp.open(trace_path, std::ios::in);
    if (!fp.good()) {
        throw std::runtime_error("Could not open trace file");
    }
    int32_t cpu_time_bias;
    int32_t wall_time_bias;
    uint16_t thread;
    handle_header(fp, &cpu_time_bias, &wall_time_bias);

    uint16_t mpi_function;
    mpi_function = find_next_function(fp);
    uint8_t config_mask;
    dumpi_time cpu;
    dumpi_time wall;
    config_mask = start_read(fp, &cpu, &wall, cpu_time_bias, wall_time_bias, &thread);
    while (mpi_function < DUMPI_END_OF_STREAM) {
        // printf("entering at walltime %d.%09d, cputime %d.%09d seconds in thread %d\n",
            //    wall.start.sec, wall.start.nsec, cpu.start.sec, cpu.start.nsec, thread);
        mpi_functions.emplace([mpi_function](auto &) -> MPIOperation<void> {
            std::cout << "mpi function type: " << mpi_function << std::endl;
            co_return;
        });
        switch (mpi_function) {
            case DUMPI_Init: {
                int argc_count;
                std::vector<std::string> argv;
                argc_count = get32(fp);
                for (int i = 0; i < argc_count; i++) {
                    uint32_t arg_len;
                    arg_len = get32(fp);
                    char *tmp_str = (char *) calloc((arg_len + 1), sizeof(char));
                    fp.read(tmp_str, arg_len * sizeof(char));
                    std::string str(tmp_str);
                    argv.push_back(str);
                    free(tmp_str);
                }
                mpi_functions.emplace([](MPIApplication &application) -> MPIOperation<void> {
                    co_await application.Initialize();
                });
                break;
            }
            case DUMPI_Comm_size: {
                dumpi_comm comm = get16(fp);
                int size = get32(fp);

                mpi_functions.emplace([comm, size](MPIApplication &application) -> MPIOperation<void> {
                    if ((MPIRankIDType) size != application.communicator(comm).GroupSize()) {
                        throw std::runtime_error{"MPI_Comm_Size error"};
                    }
                    co_return;
                });
                int64_t interval = get_interval(cpu.start.sec, cpu.start.nsec, cpu.stop.sec, cpu.stop.nsec);
                mpi_functions.emplace([interval](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    co_await application.Compute(std::chrono::nanoseconds{interval});
                });
                break;
            }
            case DUMPI_Comm_rank: {
                dumpi_comm comm = get16(fp);
                int rank = get32(fp);

                mpi_functions.emplace([comm, rank](MPIApplication &application) -> MPIOperation<void> {
                    if ((MPIRankIDType) rank != application.communicator(comm).RankID()) {
                        throw std::runtime_error{"MPI_Comm_Rank error"};
                    }
                    co_return;
                });
                int64_t interval = get_interval(cpu.start.sec, cpu.start.nsec, cpu.stop.sec, cpu.stop.nsec);
                mpi_functions.emplace([interval](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    co_await application.Compute(std::chrono::nanoseconds{interval});
                });
                break;
            }
            case DUMPI_Irecv: {
                int count = get32(fp);
                dumpi_datatype datatype = get16(fp);
                dumpi_source source = get32(fp);
                get32(fp);  // tag 用不到
                dumpi_comm comm = get16(fp);
                dumpi_request request = get32(fp);

                mpi_functions.emplace([comm, datatype, source, request, count](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    application.requests[request] = type_mapping(datatype, [&c, source, count]<typename T>() {
                        return c.template Recv<std::vector<T>>(ns3::FakePacket, source, count);
                    });
                    co_return;
                });
                break;
            }
            case DUMPI_Isend: {
                int count = get32(fp);
                dumpi_datatype datatype = get16(fp);
                dumpi_dest dest = get32(fp);
                get32(fp);  // tag用不到
                dumpi_comm comm = get16(fp);
                dumpi_request request = get32(fp);

                mpi_functions.emplace([comm, datatype, dest, request, count](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    application.requests[request] = type_mapping(datatype, [&c, dest, count]<typename T>() {
                        return c.template Send<std::vector<T>>(ns3::FakePacket, dest, count);
                    });
                    co_return;
                });
                break;
            }
            case DUMPI_Send: {
                int count = get32(fp);
                dumpi_datatype datatype = get16(fp);
                dumpi_dest dest = get32(fp);
                get32(fp);  //tag用不到
                dumpi_comm comm = get16(fp);

                mpi_functions.emplace([comm, datatype, dest, count](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await type_mapping(datatype, [&c, dest, count]<typename T>() {
                        return c.template Send<std::vector<T>>(ns3::FakePacket, dest, count);
                    });
                });
                break;
            }
            case DUMPI_Recv: {
                int count = get32(fp);
                dumpi_datatype datatype = get16(fp);
                dumpi_source source = get32(fp);
                get32(fp);  //tag用不到
                dumpi_comm comm = get16(fp);
                get_statuses(fp, config_mask);

                mpi_functions.emplace([comm, datatype, source, count](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await type_mapping(datatype, [&c, source, count]<typename T>() {
                        return c.template Recv<std::vector<T>>(ns3::FakePacket, source, count);
                    });
                });
                break;
            }
            case DUMPI_Waitall: {
                get32(fp);
                std::vector<int32_t> requests = get32arr(fp);
                get_statuses(fp, config_mask);

                mpi_functions.emplace([requests](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    for (auto request: requests) {
                        if (request != NULL_REQUEST) {
                            co_await application.requests[request];
                        }
                    }
                });
                break;
            }
            case DUMPI_Waitsome: {
                get32(fp);
                std::vector<int32_t> requests = get32arr(fp);
                get32(fp);
                std::vector<int32_t> indices = get32arr(fp);
                get_statuses(fp, config_mask);

                mpi_functions.emplace([requests](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    for (auto request: requests) {
                        if (request != NULL_REQUEST) {
                            co_await application.requests[request];
                        }
                    }
                });
                break;
            }
            case DUMPI_Wait: {
                dumpi_request request = get32(fp);
                get_statuses(fp, config_mask);

                mpi_functions.emplace([request](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    co_await application.requests[request];
                });
                break;
            }
            case DUMPI_Barrier: {
                dumpi_comm comm = get16(fp);

                mpi_functions.emplace([comm](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await c.Barrier();
                });
                break;
            }
            case DUMPI_Wtime: { //该函数压根没读取二进制数据，可以忽略
                mpi_functions.emplace([](ns3::MPIApplication &application) -> ns3::MPIOperation<void> { co_return; });
                int64_t interval = get_interval(cpu.start.sec, cpu.start.nsec, cpu.stop.sec, cpu.stop.nsec);
                mpi_functions.emplace([interval](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    co_await application.Compute(std::chrono::nanoseconds{interval});
                });
                break;
            }
            case DUMPI_Allreduce: {
                int count = get32(fp);
                dumpi_datatype datatype = get16(fp);
                get8(fp);   //op用不到
                dumpi_comm comm = get16(fp);

                mpi_functions.emplace([comm, datatype, count](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await type_mapping(datatype, [&c, count]<typename T>() {
                        return c.template AllReduce<std::vector<T>>(ns3::FakePacket, count);
                    });
                });
                break;
            }
            case DUMPI_Reduce: {
                int count = get32(fp);
                dumpi_datatype datatype = get16(fp);
                get8(fp);   //op用不到
                int root = get32(fp);
                dumpi_comm comm = get16(fp);

                mpi_functions.emplace([comm, datatype, count, root](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await type_mapping(datatype, [&c, count, root]<typename T>() {
                        return c.template Reduce<std::vector<T>>(ns3::FakePacket, root, count);
                    });
                });
                break;
            }
            case DUMPI_Bcast: {
                int count = get32(fp);
                dumpi_datatype datatype = get16(fp);
                int root = get32(fp);
                dumpi_comm comm = get16(fp);

                mpi_functions.emplace([comm, datatype, count, root](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await type_mapping(datatype, [&c, count, root]<typename T>() {
                        return c.template Broadcast<std::vector<T>>(ns3::FakePacket, root, count);
                    });
                });
                break;
            }
            case DUMPI_Alltoall: {
                int send_count = get32(fp);
                dumpi_datatype send_type = get16(fp);
                int recv_count = get32(fp);
                dumpi_datatype recv_type = get16(fp);
                dumpi_comm comm = get16(fp);
                if(send_type != recv_type){
                    throw std::runtime_error{"MPI Alltoall send_type != recv_type"};
                }
                if(send_count != recv_count){
                    throw std::runtime_error{"MPI Alltoall send_count != recv_count"};
                }

                mpi_functions.emplace([comm, send_count, send_type, recv_count, recv_type](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await type_mapping(send_type, [&c, send_count, recv_count, recv_type]<typename T>() {
                        return type_mapping(recv_type, [&c, send_count, recv_count]<typename R>(){
                            return c.template AllToAll<std::vector<T>, std::vector<R>>(ns3::FakePacket, std::tuple{send_count}, std::tuple{recv_count});
                        });
                    });
                });
                break;
            }
            case DUMPI_Alltoallv: {
                get32(fp);  // comm_size用不上
                std::vector<int32_t> send_counts = get32arr(fp);
                get32arr(fp);   // send_displs用不上
                dumpi_datatype send_type = get16(fp);
                std::vector<int32_t> recv_counts = get32arr(fp);
                get32arr(fp);   // recv_displs用不上
                dumpi_datatype recv_type = get16(fp);
                dumpi_comm comm = get16(fp);
                if(send_type != recv_type){
                    throw std::runtime_error{"MPI Alltoallv send_type != recv_type"};
                }

                mpi_functions.emplace([comm, send_counts, send_type, recv_counts, recv_type](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    std::unordered_map<MPIRankIDType, std::tuple<int>> rank_send_counts;
                    std::unordered_map<MPIRankIDType, std::tuple<int>> rank_recv_counts;
                    uint32_t count = 0;
                    for(const auto& iter:(c.GroupMembers())){
                        rank_send_counts[iter] = send_counts[count];
                        rank_recv_counts[iter] = recv_counts[count];
                        count++;
                    }
                    co_await type_mapping(send_type, [&c, rank_send_counts, rank_recv_counts, recv_type]<typename T>() {
                        return type_mapping(recv_type, [&c, rank_send_counts, rank_recv_counts]<typename R>(){
                            return c.template AllToAll<std::vector<T>, std::vector<R>>(ns3::FakePacket, rank_send_counts, rank_recv_counts);
                        });
                    });
                });
                break;
            }
//            case DUMPI_Alltoallw: {
//                ns3::MPI_Alltoallw mpi_alltoallw;
//                int commsize = get32(fp);
//                int *sendcounts = new int;
//                int *senddispls = new int;
//                get32arr(fp, &(commsize), &(sendcounts));
//                get32arr(fp, &(commsize), &(senddispls));
//                dumpi_datatype *sendtypes = new dumpi_datatype;
//                get_datatype_arr(fp, commsize, sendtypes);
//                int *recvcounts = new int;
//                int *recvdispls = new int;
//                get32arr(fp, &(commsize), &(recvcounts));
//                get32arr(fp, &(commsize), &(recvdispls));
//                dumpi_datatype *recvtypes = new dumpi_datatype;
//                get_datatype_arr(fp, commsize, recvtypes);
//                mpi_alltoallw.set_commsize(commsize);
//                mpi_alltoallw.set_sendcounts(sendcounts);
//                mpi_alltoallw.set_senddispls(senddispls);
//                mpi_alltoallw.set_sendtypes(sendtypes);
//                mpi_alltoallw.set_recvcounts(recvcounts);
//                mpi_alltoallw.set_recvdispls(recvdispls);
//                mpi_alltoallw.set_recvtypes(recvtypes);
//                mpi_alltoallw.set_comm(get16(fp));
//                mpi_functions.emplace(std::move(mpi_alltoallw));
//                delete sendcounts;
//                delete senddispls;
//                delete sendtypes;
//                delete recvcounts;
//                delete recvdispls;
//                delete recvtypes;
//                break;
//            }
            case DUMPI_Rsend: {
                int count = get32(fp);
                dumpi_datatype datatype = get16(fp);
                dumpi_dest dest = get32(fp);
                get32(fp);  // tag用不上
                dumpi_comm comm = get16(fp);

                mpi_functions.emplace([comm, datatype, dest, count](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await type_mapping(datatype, [&c, dest, count]<typename T>() {
                        return c.template Send<std::vector<T>>(ns3::FakePacket, dest, count);
                    });
                });
                break;
            }
            case DUMPI_Allgather: {
                int send_count = get32(fp);
                dumpi_datatype send_type = get16(fp);
                int recv_count = get32(fp);
                dumpi_datatype recv_type = get16(fp);
                dumpi_comm comm = get16(fp);
                if(send_type != recv_type){
                    throw std::runtime_error{"MPI Allgather send_type != recv_type"};
                }
                if(send_count != recv_count){
                    throw std::runtime_error{"MPI Allgather send_count != recv_count"};
                }

                mpi_functions.emplace([comm, send_count, send_type](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await type_mapping(send_type, [&c, send_count]<typename T>() {
                        return c.template AllGather<std::vector<T>>(ns3::FakePacket, send_count);
                    });
                });
                break;
            }
            case DUMPI_Allgatherv: {
                get32(fp);  // comm_size用不到
                get32(fp);  // send_count用不上
                dumpi_datatype send_type = get16(fp);
                std::vector<int32_t> recv_counts = get32arr(fp);
                get32arr(fp);   // displs用不上
                dumpi_datatype recv_type = get16(fp);
                dumpi_comm comm = get16(fp);
                if(send_type != recv_type){
                    throw std::runtime_error{"MPI Allgatherv send_type != recv_type"};
                }

                mpi_functions.emplace([comm, send_type, recv_counts](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    std::unordered_map<MPIRankIDType, std::tuple<int>> rank_counts;
                    uint32_t count = 0;
                    for(const auto& iter:(c.GroupMembers())){
                        rank_counts[iter] = recv_counts[count++];
                    }

                    co_await type_mapping(send_type, [&c, rank_counts]<typename T>() {
                        return c.template AllGather<std::vector<T>>(ns3::FakePacket, rank_counts);
                    });
                });
                break;
            }
            case DUMPI_Gather: {
                int comm_rank = get32(fp);
                int send_count = get32(fp);
                dumpi_datatype send_type = get16(fp);
                int root = get32(fp);
                dumpi_comm comm = get16(fp);
                if(comm_rank == root){
                    get32(fp);  // recv_count用不上
                    dumpi_datatype recv_type = get16(fp);
                    if(send_type != recv_type){
                        throw std::runtime_error{"MPI Gather send_type != recv_type"};
                    }
                }

                mpi_functions.emplace([comm, root, send_count, send_type](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await type_mapping(send_type, [&c, root, send_count]<typename T>() {
                        return c.template Gather<std::vector<T>>(ns3::FakePacket, root, send_count);
                    });
                });
                break;
            }
            case DUMPI_Gatherv: {
                int comm_rank = get32(fp);
                get32(fp);  // comm_size用不上
                int send_count = get32(fp);
                dumpi_datatype send_type = get16(fp);
                int root = get32(fp);
                dumpi_comm comm = get16(fp);
                std::vector<int32_t> recv_counts;
                if(comm_rank == root){
                    recv_counts = get32arr(fp);
                    get32arr(fp);   // displs用不上
                    if(send_count != recv_counts[comm_rank]){
                        throw std::runtime_error{"MPI Gatherv send_count != recv_count"};
                    }
                }
                dumpi_datatype recv_type = get16(fp);
                if(send_type != recv_type){
                    throw std::runtime_error{"MPI Gatherv send_type != recv_type"};
                }

                mpi_functions.emplace([comm, comm_rank, root, send_type, send_count, recv_counts](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    if(c.RankID() != (MPIRankIDType)comm_rank){
                        throw std::runtime_error{"Gatherv parsed rankID != communicator.RankID()"};
                    }
                    std::unordered_map<MPIRankIDType, std::tuple<int>> rank_counts;
                    if(comm_rank == root){
                        uint32_t count = 0;
                        for(const auto& iter:(c.GroupMembers())){
                            rank_counts[iter] = recv_counts[count++];
                        }
                    }
                    else{
                        rank_counts[comm_rank] = send_count;
                    }

                    co_await type_mapping(send_type, [&c, root, rank_counts]<typename T>() {
                        return c.template Gather<std::vector<T>>(ns3::FakePacket, root, rank_counts);
                    });
                });
                break;
            }
            case DUMPI_Initialized: {
                int result = get32(fp);
                mpi_functions.emplace([result](MPIApplication &application) -> MPIOperation<void> {
                    if (result != static_cast<int>(application.Initialized())) {
                        throw std::runtime_error{"Application has been initialized before calling MPI_Init!!!"};
                    }
                    co_return;
                });

                int64_t interval = get_interval(cpu.start.sec, cpu.start.nsec, cpu.stop.sec, cpu.stop.nsec);    // 只在init函数进行初始化
                mpi_functions.emplace([interval](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    co_await application.Compute(std::chrono::nanoseconds{interval});
                });
                break;
            }
            case DUMPI_Testall: {
                get32(fp);  // count用不上
                std::vector<int32_t> requests = get32arr(fp);
                get32(fp);  // flag用不上
                get_statuses(fp, config_mask);

                mpi_functions.emplace([requests](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    for (auto request: requests) {
                        if (request != NULL_REQUEST) {
                            co_await application.requests[request];
                        }
                    }
                });
                break;
            }
            case DUMPI_Sendrecv: {
                int send_count = get32(fp);
                dumpi_datatype send_type = get16(fp);
                dumpi_dest dest = get32(fp);
                get32(fp);  //send_tag用不到
                int recv_count = get32(fp);
                dumpi_datatype recv_type = get16(fp);
                dumpi_source source = get32(fp);
                get32(fp);  //recv_tag用不到
                dumpi_comm comm = get16(fp);
                get_statuses(fp, config_mask);

                mpi_functions.emplace([comm, send_count, send_type, dest, recv_count, source, recv_type](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                      auto &c = application.communicator(comm);
                      co_await type_mapping(send_type, [&c, send_count, dest, recv_count, source, recv_type]<typename T>() {
                          return type_mapping(recv_type, [&c, send_count, dest, recv_count, source]<typename R>(){
                              return c.template SendRecv<std::vector<T>, std::vector<R>>(ns3::FakePacket, dest, source, std::tuple{send_count}, std::tuple{recv_count});
                          });
                      });
                });
                break;
            }
            case DUMPI_Sendrecv_replace: {
                int count = get32(fp);
                dumpi_datatype datatype = get16(fp);
                dumpi_dest dest = get32(fp);
                get32(fp);  // send_tag用不到
                dumpi_source source = get32(fp);
                get32(fp);  // recv_tag用不到
                dumpi_comm comm = get16(fp);
                get_statuses(fp, config_mask);

                mpi_functions.emplace([comm, count, datatype, dest, source](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await type_mapping(datatype, [&c, count, dest, source]<typename T>() {
                        return c.template SendRecv<std::vector<T>, std::vector<T>>(ns3::FakePacket, dest, source, std::tuple{count}, std::tuple{count});
                    });
                });
                break;
            }
            case DUMPI_Scatter: {
                int comm_rank = get32(fp);
                int recv_count = get32(fp);
                dumpi_datatype recv_type = get16(fp);
                int root = get32(fp);
                dumpi_comm comm = get16(fp);

                if(comm_rank == root){
                    get32(fp);  // send_count用不上
                    dumpi_datatype send_type = get16(fp);
                    if(send_type != recv_type){
                        throw std::runtime_error{"MPI Scatter send_type != recv_type"};
                    }
                }

                mpi_functions.emplace([comm, root, recv_count, recv_type](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    co_await type_mapping(recv_type, [&c, root, recv_count]<typename T>() {
                        return c.template Scatter<std::vector<T>>(ns3::FakePacket, root, recv_count);
                    });
                });
                break;
            }
            case DUMPI_Scatterv: {
                int comm_rank = get32(fp);
                get32(fp);  // comm_size用不上
                dumpi_datatype send_type = get16(fp);
                int recv_count = get32(fp);
                dumpi_datatype recv_type = get16(fp);
                int root = get32(fp);
                dumpi_comm comm = get16(fp);
                std::vector<int32_t> send_counts;
                if(send_type != recv_type){
                    throw std::runtime_error{"MPI Scatterv send_type != recv_type"};
                }

                if (comm_rank == root) {
                    send_counts = get32arr(fp);
                    get32arr(fp);   // displs用不上
                }

                mpi_functions.emplace([comm, comm_rank, root, recv_count, recv_type, send_counts](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    if(c.RankID() != (MPIRankIDType)comm_rank){
                        throw std::runtime_error{"Scatterv parsed rankID != communicator.RankID()"};
                    }
                    std::unordered_map<MPIRankIDType, std::tuple<int>> rank_counts;
                    if(comm_rank == root){
                        uint32_t count = 0;
                        for(const auto& iter:(c.GroupMembers())){
                            rank_counts[iter] = send_counts[count++];
                        }
                    }
                    else{
                        rank_counts[comm_rank] = recv_count;
                    }

                    co_await type_mapping(recv_type, [&c, root, rank_counts]<typename T>() {
                        return c.template Scatter<std::vector<T>>(ns3::FakePacket, root, rank_counts);
                    });
                });
                break;
            }
            case DUMPI_Reduce_scatter: {
                get32(fp);  // comm_size用不上
                std::vector<int32_t> recv_counts = get32arr(fp);
                dumpi_datatype datatype = get16(fp);
                get8(fp);   // op用不上
                dumpi_comm comm = get16(fp);

                mpi_functions.emplace([comm, recv_counts, datatype](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    auto &c = application.communicator(comm);
                    std::unordered_map<MPIRankIDType, std::tuple<int>> rank_counts;
                    uint32_t count = 0;
                    for(const auto& iter:(c.GroupMembers())){
                        rank_counts[iter] = recv_counts[count++];
                    }

                    co_await type_mapping(datatype, [&c, rank_counts]<typename T>() {
                        return c.template ReduceScatter<std::vector<T>>(ns3::FakePacket, rank_counts);
                    });
                });
                break;
            }
//            case DUMPI_Group_free: {
//                ns3::MPI_Group_Free mpi_group_free;
//                mpi_group_free.set_group(get16(fp));
//                mpi_functions.emplace(std::move(mpi_group_free));
//                break;
//            }
//            case DUMPI_Comm_group: {
//                ns3::MPI_Comm_Group mpi_comm_group;
//                mpi_comm_group.set_comm(get16(fp));
//                mpi_comm_group.set_group(get16(fp));
//                mpi_functions.emplace(std::move(mpi_comm_group));
//                break;
//            }
//            case DUMPI_Group_incl: {
//                ns3::MPI_Group_Incl mpi_group_incl;
//                mpi_group_incl.set_group(get16(fp));
//                int count = get32(fp);
//                mpi_group_incl.set_count(count);
//                int *ranks = new int;
//                get32arr(fp, &(count), &(ranks));
//                mpi_group_incl.set_ranks(ranks);
//                mpi_group_incl.set_newgroup(get16(fp));
//                mpi_functions.emplace(std::move(mpi_group_incl));
//                delete ranks;
//                break;
//            }
//            case DUMPI_Comm_create: {
//                ns3::MPI_Comm_Create mpi_comm_create;
//                mpi_comm_create.set_oldcomm(get16(fp));
//                mpi_comm_create.set_group(get16(fp));
//                mpi_comm_create.set_newcomm(get16(fp));
//                mpi_functions.emplace(std::move(mpi_comm_create));
//                break;
//            }
//            case DUMPI_Type_free: {
//                ns3::MPI_Type_Free mpi_type_free;
//                mpi_type_free.set_datatype(get16(fp));
//                mpi_functions.emplace(std::move(mpi_type_free));
//                break;
//            }
//            case DUMPI_Type_commit: {
//                ns3::MPI_Type_Commit mpi_type_commit;
//                mpi_type_commit.set_datatype(get16(fp));
//                mpi_functions.emplace(std::move(mpi_type_commit));
//                break;
//            }
//            case DUMPI_Type_vector: {
//                ns3::MPI_Type_Vector mpi_type_vector;
//                mpi_type_vector.set_count(get32(fp));
//                mpi_type_vector.set_blocklength(get32(fp));
//                mpi_type_vector.set_stride(get32(fp));
//                mpi_type_vector.set_oldtype(get16(fp));
//                mpi_type_vector.set_newtype(get16(fp));
//                mpi_functions.emplace(std::move(mpi_type_vector));
//                break;
//            }
//            case DUMPI_Rsend_init: {
//                ns3::MPI_Rsend_Init mpi_rsend_init;
//                mpi_rsend_init.set_count(get32(fp));
//                mpi_rsend_init.set_datatype(get16(fp));
//                mpi_rsend_init.set_dest(get32(fp));
//                mpi_rsend_init.set_tag(get32(fp));
//                mpi_rsend_init.set_comm(get16(fp));
//                mpi_rsend_init.set_request(get32(fp));
//                mpi_functions.emplace(std::move(mpi_rsend_init));
//                break;
//            }
//            case DUMPI_Op_free: {
//                ns3::MPI_Op_Free mpi_op_free;
//                mpi_op_free.set_op(get8(fp));
//                mpi_functions.emplace(std::move(mpi_op_free));
//                break;
//            }
//            case DUMPI_Op_create: {
//                ns3::MPI_Op_Create mpi_op_create;
//                mpi_op_create.set_commute(get32(fp));
//                mpi_op_create.set_op(get8(fp));
//                mpi_functions.emplace(std::move(mpi_op_create));
//                break;
//            }
//            case DUMPI_Comm_free: {
//                ns3::MPI_Comm_Free mpi_comm_free;
//                mpi_comm_free.set_comm(get16(fp));
//                mpi_functions.emplace(std::move(mpi_comm_free));
//                break;
//            }
//            case DUMPI_Comm_free_keyval: {
//                ns3::MPI_Comm_Free_Keyval mpi_comm_free_keyval;
//                mpi_comm_free_keyval.set_keyval(get16(fp));
//                mpi_functions.emplace(std::move(mpi_comm_free_keyval));
//                break;
//            }
//            case DUMPI_Comm_dup: {
//                ns3::MPI_Comm_Dup mpi_comm_dup;
//                mpi_comm_dup.set_oldcomm(get16(fp));
//                mpi_comm_dup.set_newcomm(get16(fp));
//                mpi_functions.emplace(std::move(mpi_comm_dup));
//                break;
//            }
//            case DUMPI_Comm_split: {
//                ns3::MPI_Comm_Split mpi_comm_split;
//                mpi_comm_split.set_oldcomm(get16(fp));
//                mpi_comm_split.set_color(get32(fp));
//                mpi_comm_split.set_key(get32(fp));
//                mpi_comm_split.set_newcomm(get16(fp));
//                mpi_functions.emplace(std::move(mpi_comm_split));
//                break;
//            }
//            case DUMPI_Pack_size: {
//                ns3::MPI_Pack_Size mpi_pack_size;
//                mpi_pack_size.set_incount(get32(fp));
//                mpi_pack_size.set_datatype(get16(fp));
//                mpi_pack_size.set_comm(get16(fp));
//                mpi_pack_size.set_size(get32(fp));
//                mpi_functions.emplace(std::move(mpi_pack_size));
//                break;
//            }
//            case DUMPI_Pack: {
//                ns3::MPI_Pack mpi_pack;
//                mpi_pack.set_incount(get32(fp));
//                mpi_pack.set_datatype(get16(fp));
//                mpi_pack.set_outcount(get32(fp));
//                pack_position position;
//                position.in = get32(fp);
//                position.out = get32(fp);
//                mpi_pack.set_position(position);
//                mpi_pack.set_comm(get16(fp));
//                mpi_functions.emplace(std::move(mpi_pack));
//                break;
//            }
//            case DUMPI_Unpack: {
//                ns3::MPI_Unpack mpi_unpack;
//                mpi_unpack.set_incount(get32(fp));
//                pack_position position;
//                position.in = get32(fp);
//                position.out = get32(fp);
//                mpi_unpack.set_position(position);
//                mpi_unpack.set_outcount(get32(fp));
//                mpi_unpack.set_datatype(get16(fp));
//                mpi_unpack.set_comm(get16(fp));
//                mpi_functions.emplace(std::move(mpi_unpack));
//                break;
//            }
//            case DUMPI_Cart_sub: {
//                ns3::MPI_Cart_Sub mpi_cart_sub;
//                int ndim = get32(fp);
//                mpi_cart_sub.set_ndim(ndim);
//                mpi_cart_sub.set_oldcomm(get16(fp));
//                int *remain_dims = new int;
//                get32arr(fp, &(ndim), &(remain_dims));
//                mpi_cart_sub.set_remain_dims(remain_dims);
//                mpi_cart_sub.set_newcomm(get16(fp));
//                mpi_functions.emplace(std::move(mpi_cart_sub));
//                delete remain_dims;
//                break;
//            }
//            case DUMPI_Cart_create: {
//                ns3::MPI_Cart_Create mpi_cart_create;
//                mpi_cart_create.set_oldcomm(get16(fp));
//                int ndim = get32(fp);
//                int *dims = new int;
//                int *periods = new int;
//                get32arr(fp, &(ndim), &(dims));
//                get32arr(fp, &(ndim), &(periods));
//                mpi_cart_create.set_ndim(ndim);
//                mpi_cart_create.set_dims(dims);
//                mpi_cart_create.set_periods(periods);
//                mpi_cart_create.set_reorder(get32(fp));
//                mpi_cart_create.set_newcomm(get16(fp));
//                mpi_functions.emplace(std::move(mpi_cart_create));
//                delete dims;
//                delete periods;
//                break;
//            }
//            case DUMPI_Cart_coords: {
//                ns3::MPI_Cart_Coords mpi_cart_coords;
//                int ndim = get32(fp);
//                mpi_cart_coords.set_ndim(ndim);
//                mpi_cart_coords.set_comm(get16(fp));
//                mpi_cart_coords.set_rank(get32(fp));
//                mpi_cart_coords.set_maxdims(get32(fp));
//                int *coords = new int;
//                get32arr(fp, &(ndim), &(coords));
//                mpi_functions.emplace(std::move(mpi_cart_coords));
//                delete coords;
//                break;
//            }
//            case DUMPI_Cart_rank: {
//                ns3::MPI_Cart_Rank mpi_cart_rank;
//                int ndim = get32(fp);
//                mpi_cart_rank.set_ndim(ndim);
//                mpi_cart_rank.set_comm(get16(fp));
//                int *coords = new int;
//                get32arr(fp, &(ndim), &(coords));
//                mpi_cart_rank.set_coords(coords);
//                mpi_cart_rank.set_rank(get32(fp));
//                mpi_functions.emplace(std::move(mpi_cart_rank));
//                delete coords;
//                break;
//            }
            case DUMPI_Get_count: {
                get_statuses(fp, config_mask);
                get16(fp);
                get32(fp);

                int64_t interval = get_interval(cpu.start.sec, cpu.start.nsec, cpu.stop.sec, cpu.stop.nsec);
                mpi_functions.emplace([interval](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    co_await application.Compute(std::chrono::nanoseconds{interval});
                });
                break;
            }
            case DUMPI_Attr_get: {
                get16(fp);
                get32(fp);
                get32(fp);

                int64_t interval = get_interval(cpu.start.sec, cpu.start.nsec, cpu.stop.sec, cpu.stop.nsec);
                mpi_functions.emplace([interval](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    co_await application.Compute(std::chrono::nanoseconds{interval});
                });
                break;
            }
            case DUMPI_Alloc_mem: {
                get32(fp);  // size用不上
                get16(fp);  // info用不上

                int64_t interval = get_interval(cpu.start.sec, cpu.start.nsec, cpu.stop.sec, cpu.stop.nsec);
                mpi_functions.emplace([interval](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    co_await application.Compute(std::chrono::nanoseconds{interval});
                });
                break;
            }
            case DUMPI_Free_mem: {
                int64_t interval = get_interval(cpu.start.sec, cpu.start.nsec, cpu.stop.sec, cpu.stop.nsec);
                mpi_functions.emplace([interval](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    co_await application.Compute(std::chrono::nanoseconds{interval});
                });
                break;
            }
            case DUMPI_Get_version: {
                get32(fp);  // version用不上
                get32(fp);  // subversion用不上

                int64_t interval = get_interval(cpu.start.sec, cpu.start.nsec, cpu.stop.sec, cpu.stop.nsec);
                mpi_functions.emplace([interval](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    co_await application.Compute(std::chrono::nanoseconds{interval});
                });
                break;
            }
//            case DUMPI_Type_indexed: {
//                ns3::MPI_Type_Indexed mpi_type_indexed;
//                int count = get32(fp);
//                mpi_type_indexed.set_count(count);
//                int *lengths = new int;
//                int *indices = new int;
//                get32arr(fp, &(count), &(lengths));
//                get32arr(fp, &(count), &(indices));
//                mpi_type_indexed.set_lengths(lengths);
//                mpi_type_indexed.set_indices(indices);
//                mpi_type_indexed.set_oldtype(get16(fp));
//                mpi_type_indexed.set_newtype(get16(fp));
//                mpi_functions.emplace(std::move(mpi_type_indexed));
//                delete lengths;
//                delete indices;
//                break;
//            }
            case DUMPI_Finalize: {
                mpi_functions.emplace([](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                    co_return application.Finalize();
                });
                break;
            }
            default: {
                throw std::runtime_error{"No such mpi function!!!"};
            }
        }

        // printf("returning at walltime %d.%09d, cputime %d.%09d seconds in thread %d\n",
        //        wall.stop.sec, wall.stop.nsec, cpu.stop.sec, cpu.stop.nsec, thread);

        mpi_function = find_next_function(fp);
        if (mpi_function < DUMPI_END_OF_STREAM) {
            // 插入两个相邻MPI函数之间的阻塞时间
            int32_t cur_stop_sec = cpu.stop.sec;
            int32_t cur_stop_nsec = cpu.stop.nsec;
            config_mask = start_read(fp, &cpu, &wall, cpu_time_bias, wall_time_bias, &thread);
            int64_t interval = get_interval(cur_stop_sec, cur_stop_nsec, cpu.start.sec, cpu.start.nsec);
            mpi_functions.emplace([interval](ns3::MPIApplication &application) -> ns3::MPIOperation<void> {
                co_await application.Compute(std::chrono::nanoseconds{interval});
            });
        }
    }

    fp.close();
    return mpi_functions;
}

std::vector<std::queue<ns3::MPIFunction>> ns3::parse_traces(std::filesystem::path trace_dir) {
    std::vector<std::filesystem::path> trace_file_names;
    for (auto &p: std::filesystem::directory_iterator(trace_dir)) {
        trace_file_names.push_back(p.path());
    }
    std::sort(trace_file_names.begin(), trace_file_names.end());
    std::vector<std::queue<ns3::MPIFunction>> q;
    for (const auto &path: trace_file_names) {
        q.push_back(ns3::parse_trace(path));
    }
    return q;
}