//
// Created by Ricardo Evans on 2023/6/20.
//

#ifndef NS3_MPI_APPLICATION_PROTOCOL_H
#define NS3_MPI_APPLICATION_PROTOCOL_H

#include <algorithm>
#include <concepts>
#include <numeric>
#include <ranges>

#include "mpi-protocol-trait.h"

#include "ns3/packet.h"
#include "ns3/socket.h"

namespace ns3 {
    using NS3Packet = Ptr<Packet>;
    using NS3Error = Socket::SocketErrno;

    constinit const MPIRawPacket RawPacket{};

    constinit const MPIFakePacket FakePacket{};

    template<typename T> requires std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, uint128_t> || std::is_same_v<T, int128_t>
    struct MPIObjectReader<T> {
        MPIOperation<void> operator()(MPISocket &socket, MPIFakePacket) const {
            co_await read(socket, size);
        }

        MPIOperation<T> operator()(MPISocket &socket) const {
            NS3Packet result = co_await read(socket, size);
            T data;
            result->CopyData(reinterpret_cast<uint8_t *>(&data), size);
            co_return std::move(data);
        }

    private:
        static constexpr const std::size_t size = sizeof(T);

        MPIOperation<NS3Packet> read(MPISocket &socket, std::size_t s) const {
            auto [packet, error] = co_await socket.receive(s);
            if (error != NS3Error::ERROR_NOTERROR) {
                throw MPISocketException{"Parse " + std::string{typeid(T).name()} + " failed, reason: " + format(error)};
            }
            co_return packet;
        }
    };

    template<typename T> requires std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, uint128_t> || std::is_same_v<T, int128_t>
    struct MPIObjectWriter<T> {
        MPIOperation<void> operator()(MPISocket &socket, const T &t) const {
            co_await write(socket, Create<Packet>(reinterpret_cast<const uint8_t *>(&t), size));
        }

        MPIOperation<void> operator()(MPISocket &socket, MPIFakePacket) const {
            co_await write(socket, Create<Packet>(size));
        }

    private:
        static constexpr const std::size_t size = sizeof(T);

        MPIOperation<void> write(MPISocket &socket, NS3Packet packet) const {
            auto [sent, error] = co_await socket.send(packet);
            if (error != NS3Error::ERROR_NOTERROR) {
                throw MPISocketException{"Deparse " + std::string{typeid(T).name()} + " failed, reason: " + format(error)};
            }
        }
    };

    struct FakeDataPacket {
    };

    template<>
    struct MPIObjectReader<FakeDataPacket> {
        MPIOperation<void> operator()(MPISocket &socket, MPIFakePacket, std::size_t packet_size) const {
            auto [packet, error] = co_await socket.receive(packet_size);
            if (error != NS3Error::ERROR_NOTERROR) {
                throw MPISocketException{std::string{"Read fake data packet failed, reason: "} + format(error)};
            }
        }
    };

    template<>
    struct MPIObjectWriter<FakeDataPacket> {
        MPIOperation<void> operator()(MPISocket &socket, MPIFakePacket, std::size_t packet_size) const {
            auto [sent, error] = co_await socket.send(Create<Packet>(packet_size));
            if (error != NS3Error::ERROR_NOTERROR) {
                throw MPISocketException{std::string{"Write fake data packet failed, reason: "} + format(error)};
            }
        }
    };

    template<typename T>
    struct MPIObjectReader<std::vector<T>> {
        MPIOperation<std::vector<T>> operator()(MPISocket &socket) const requires MPIObject<T> {
            std::size_t count = co_await MPIObjectReader<std::size_t>{}(socket);
            std::vector<T> result;
            MPIObjectReader<T> reader;
            for (std::size_t i = 0; i < count; ++i) {
                result.push_back(std::move(co_await reader(socket)));
            }
            co_return result;
        }

        template<typename ...U>
        MPIOperation<void> operator()(MPISocket &socket, MPIFakePacket p, std::size_t size, U &&...u) const requires MPIFakeObject<T, U...> {
            co_await MPIObjectReader<std::size_t>{}(socket, p);
            MPIObjectReader<T> reader;
            while (size-- > 0) {
                co_await reader(socket, p, std::forward<U>(u)...);
            }
        }
    };

    template<typename T>
    struct MPIObjectWriter<std::vector<T>> {
        MPIOperation<void> operator()(MPISocket &socket, const std::vector<T> &vector) const requires MPIObject<T> {
            co_await MPIObjectWriter<std::size_t>{}(socket, vector.size());
            MPIObjectWriter<T> writer;
            for (auto &t: vector) {
                co_await writer(socket, t);
            }
        }

        template<typename ...U>
        MPIOperation<void> operator()(MPISocket &socket, MPIFakePacket p, std::size_t count, U &&...u) const requires MPIFakeObject<T, U...> {
            co_await MPIObjectWriter<std::size_t>{}(socket, count);
            MPIObjectWriter<T> writer;
            while (count-- > 0) {
                co_await writer(socket, p, std::forward<U>(u)...);
            }
        }
    };

    template<typename T> requires std::is_integral_v<T> || std::is_floating_point_v<T>
    struct MPIOperatorImplementation<MPIOperator::SUM, T> {
        template<std::ranges::range R>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), T{0}, std::plus{});
        }
    };

    template<MPIAddable T>
    struct MPIOperatorImplementation<MPIOperator::SUM, T> {
        template<std::ranges::range R>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), T::AdditionUnit, std::plus{});
        }
    };

    template<typename T>
    struct MPIOperatorImplementation<MPIOperator::SUM, T> {
        template<std::ranges::range R, typename O, typename U=T>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r, U &&unit, O &&o) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), std::forward<U>(unit), std::forward<O>(o));
        }
    };

    template<typename T> requires std::is_integral_v<T> || std::is_floating_point_v<T>
    struct MPIOperatorImplementation<MPIOperator::PRODUCT, T> {
        template<std::ranges::range R>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), T{1}, std::multiplies{});
        }
    };

    template<MPIMultiplicative T>
    struct MPIOperatorImplementation<MPIOperator::PRODUCT, T> {
        template<std::ranges::range R>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), T::MultiplicationUnit, std::multiplies{});
        }
    };

    template<typename T>
    struct MPIOperatorImplementation<MPIOperator::PRODUCT, T> {
        template<std::ranges::range R, typename O, typename U=T>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r, U &&unit, O &&o) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), std::forward<U>(unit), std::forward<O>(o));
        }
    };

    template<typename T>
    struct MPIOperatorImplementation<MPIOperator::MAX, T> {
        template<std::ranges::range R, typename C=std::ranges::less, typename P=std::identity>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r, C &&comparator = {}, P &&projectile = {}) const {
            return *std::ranges::max_element(std::forward<R>(r), std::forward<C>(comparator), std::forward<P>(projectile));
        }
    };

    template<typename T>
    struct MPIOperatorImplementation<MPIOperator::MIN, T> {
        template<std::ranges::range R, typename C=std::ranges::less, typename P=std::identity>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r, C &&comparator = {}, P &&projectile = {}) const {
            return *std::ranges::min_element(std::forward<R>(r), std::forward<C>(comparator), std::forward<P>(projectile));
        }
    };

    template<typename T> requires std::is_integral_v<T>
    struct MPIOperatorImplementation<MPIOperator::BITWISE_AND, T> {
        template<std::ranges::range R>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), ~T{0}, std::bit_and{});
        }
    };

    template<typename T> requires std::is_integral_v<T>
    struct MPIOperatorImplementation<MPIOperator::BITWISE_OR, T> {
        template<std::ranges::range R>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), T{0}, std::bit_or{});
        }
    };

    template<typename T> requires std::is_integral_v<T>
    struct MPIOperatorImplementation<MPIOperator::BITWISE_XOR, T> {
        template<std::ranges::range R>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), T{0}, std::bit_xor{});
        }
    };

    template<typename T> requires std::is_integral_v<T>
    struct MPIOperatorImplementation<MPIOperator::LOGICAL_AND, T> {
        template<std::ranges::range R>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), ~T{0}, std::logical_and{});
        }
    };

    template<typename T> requires std::is_integral_v<T>
    struct MPIOperatorImplementation<MPIOperator::LOGICAL_OR, T> {
        template<std::ranges::range R>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), T{0}, std::logical_or{});
        }
    };

    template<typename T> requires std::is_integral_v<T>
    struct MPIOperatorImplementation<MPIOperator::LOGICAL_XOR, T> {
        template<std::ranges::range R>
        requires std::is_same_v<std::ranges::range_value_t<R>, T>
        T operator()(R &&r) const {
            return std::accumulate(std::ranges::begin(r), std::ranges::end(r), T{0}, std::bit_xor{});
        }
    };
}

#endif //NS3_MPI_APPLICATION_PROTOCOL_H
