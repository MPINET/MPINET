//
// Created by Ricardo Evans on 2023/6/18.
//

#ifndef NS3_MPI_APPLICATION_PROTOCOL_TRAIT_H
#define NS3_MPI_APPLICATION_PROTOCOL_TRAIT_H

#include <cstdint>
#include <concepts>
#include <vector>

#include "ns3/packet.h"

#include "mpi-exception.h"
#include "mpi-operation.h"
#include "mpi-socket.h"
#include "mpi-util.h"

namespace ns3 {
    using MPIRankIDType = uint64_t;

    enum class MPIOperator {
        SUM,
        PRODUCT,
        MAX,
        MIN,
        LOGICAL_AND,
        BITWISE_AND,
        LOGICAL_OR,
        BITWISE_OR,
        LOGICAL_XOR,
        BITWISE_XOR,
    };

    struct MPIRawPacket {
    };

    struct MPIFakePacket {
    };

    template<typename T>
    struct MPIObjectReader;

    template<typename T>
    struct MPIObjectWriter;

    template<MPIOperator O, typename T>
    struct MPIOperatorImplementation;

    template<typename T>
    concept MPIReadable=requires(MPIObjectReader<T> reader){
        { reader.operator()(std::declval<MPISocket &>()) };
    };

    template<typename T>
    concept MPIWritable=requires(MPIObjectWriter<T> writer){
        { writer.operator()(std::declval<MPISocket &>(), std::declval<T>()) };
    };

    template<typename T>
    concept MPIObject = MPIReadable<T> and MPIWritable<T>;

    template<typename T, typename ...U>
    concept MPIFakeReadable=requires(MPIObjectReader<T> reader){
        { reader(std::declval<MPISocket &>(), std::declval<MPIFakePacket>(), std::declval<U>()...) };
    };

    template<typename T, typename ...U>
    concept MPIFakeWritable=requires(MPIObjectWriter<T> writer){
        { writer(std::declval<MPISocket &>(), std::declval<MPIFakePacket>(), std::declval<U>()...) };
    };

    template<typename T, typename ...U>
    concept MPIFakeObject = MPIFakeReadable<T, U...> and MPIFakeWritable<T, U...>;

    template<typename T>
    concept MPIAddable=requires(T t1, T t2){
        { t1 + t2 } -> std::same_as<T>;
        { T::AdditionUnit } -> std::same_as<T>;
    };

    template<typename T>
    concept MPIMultiplicative=requires(T t1, T t2){
        { t1 * t2 } -> std::same_as<T>;
        { T::MultiplicationUnit } -> std::same_as<T>;
    };

    template<MPIOperator O, typename T, typename ...P>
    concept MPIOperatorApplicable=requires(MPIOperatorImplementation<O, T> o){
        { o(std::declval<std::vector<T>>(), std::declval<P>()...) } -> std::convertible_to<T>;
    };
}
#endif //NS3_MPI_APPLICATION_PROTOCOL_TRAIT_H
