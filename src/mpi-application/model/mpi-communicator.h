#ifndef NS3_MPI_APPLICATION_COMMUNICATOR_H
#define NS3_MPI_APPLICATION_COMMUNICATOR_H

#include <coroutine>
#include <memory>
#include <numeric>
#include <random>
#include <ranges>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>

#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"

#include "mpi-exception.h"
#include "mpi-socket.h"
#include "mpi-protocol.h"
#include "mpi-util.h"

namespace ns3 {
    using MPICommunicatorIDType = std::uint64_t;
    static const constinit
    MPICommunicatorIDType ERROR_COMMUNICATOR = 0;
    static const constinit
    MPICommunicatorIDType NULL_COMMUNICATOR = 1;
    static const constinit
    MPICommunicatorIDType WORLD_COMMUNICATOR = 2;
    static const constinit
    MPICommunicatorIDType SELF_COMMUNICATOR = 3;

    /**
     * @brief MPICommunicator is the model of the communicator concept in MPI operations, you should do most of communications via this.
     */
    class MPICommunicator {
    private:
        using NS3Packet = Ptr<Packet>;
        using NS3Error = Socket::SocketErrno;

        MPIRankIDType rankID;
        std::shared_ptr<std::mt19937> randomEngine;
        std::vector<MPIRankIDType> ranks;
        std::unordered_map<MPIRankIDType, MPISocket> sockets;
        std::uniform_int_distribution<MPIRankIDType> voteGenerator{0, std::numeric_limits<MPIRankIDType>::max()};

        MPIOperation<void> templateTest();

    public:
        MPICommunicator() noexcept = default;

        MPICommunicator(MPIRankIDType rankID, const std::shared_ptr<std::mt19937> &randomEngine, std::unordered_map<MPIRankIDType, MPISocket> &&sockets) noexcept;

        MPICommunicator(MPICommunicator &&) noexcept = default;

        MPICommunicator(const MPICommunicator &) = delete;

        MPICommunicator &operator=(MPICommunicator &&) noexcept = default;

        MPICommunicator &operator=(const MPICommunicator &) = delete;

        MPIOperation<void> Send(MPIRawPacket, MPIRankIDType rank, NS3Packet packet);

        MPIOperation<void> Send(MPIFakePacket, MPIRankIDType rank, std::size_t size);

        template<typename T, MPIWritable T_=std::decay_t<T>>
        MPIOperation<void> Send(MPIRankIDType rank, T &&data) {
            co_await MPIObjectWriter<T_>{}(sockets[rank], std::forward<T>(data));
        }

        template<typename T, typename ...U>
        requires MPIFakeWritable<T, std::decay_t<U>...>
        MPIOperation<void> Send(MPIFakePacket p, MPIRankIDType rank, U &&... u) {
            co_await MPIObjectWriter<T>{}(sockets[rank], p, std::forward<U>(u)...);
        }

        MPIOperation<NS3Packet> Recv(MPIRawPacket, MPIRankIDType rank, std::size_t size);

        MPIOperation<void> Recv(MPIFakePacket, MPIRankIDType rank, std::size_t size);

        template<MPIReadable T>
        MPIOperation<T> Recv(MPIRankIDType rank) {
            co_return std::move(co_await MPIObjectReader<T>{}(sockets[rank]));
        }

        template<typename T, typename ...U>
        requires MPIFakeReadable<T, std::decay_t<U>...>
        MPIOperation<void> Recv(MPIFakePacket p, MPIRankIDType rank, U &&...u) {
            co_await MPIObjectReader<T>{}(sockets[rank], p, std::forward<U>(u)...);
        }

        template<typename S, typename R=std::decay_t<S>>
        requires MPIWritable<std::decay_t<S>> and MPIReadable<std::decay_t<R>>
        MPIOperation<R> SendRecv(MPIRankIDType destination, S &&data, MPIRankIDType source) {
            auto oS = Send(destination, std::forward<S>(data));
            auto oR = Recv<R>(source);
            co_await oS;
            co_return co_await oR;
        }

        template<typename S, typename R, typename ...US, typename ...UR>
        requires MPIFakeWritable<S, US...> and MPIFakeReadable<R, UR...>
        MPIOperation<void> SendRecv(MPIFakePacket p, MPIRankIDType destination, MPIRankIDType source, const std::tuple<US...> &uS, const std::tuple<UR...> &uR) {
            auto oS = std::apply([this, &p, &destination](auto &&...args) { return this->Send<S>(p, destination, std::forward<decltype(args)>(args)...); }, uS);
            auto oR = std::apply([this, &p, &source](auto &&...args) { return this->Recv<R>(p, source, std::forward<decltype(args)>(args)...); }, uR);
            co_await oS;
            co_await oR;
        }

        template<typename T, MPIObject T_=std::decay_t<T>>
        MPIOperation<std::unordered_map<MPIRankIDType, T_>> Gather(MPIRankIDType root, T &&data) {
            std::unordered_map<MPIRankIDType, T_> result;
            auto o = Send(root, std::forward<T>(data));
            if (rankID == root) {
                std::unordered_map<MPIRankIDType, MPIOperation<T_>> operations;
                for (auto rank: sockets | std::ranges::views::keys) {
                    operations[rank] = Recv<T_>(rank);
                }
                for (auto &[rank, operation]: operations) {
                    if constexpr (std::is_move_assignable_v<T_>) {
                        result[rank] = std::move(co_await operation);
                    } else {
                        result[rank] = co_await operation;
                    }
                }
            }
            co_await o;
            co_return result;
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, std::decay_t<U>...>
        MPIOperation<void> Gather(MPIFakePacket p, MPIRankIDType root, U &&...u) {
            std::unordered_map<MPIRankIDType, T> result;
            auto o = Send<T>(p, root, u...);
            if (rankID == root) {
                std::vector<MPIOperation<void>> operations;
                for (auto rank: sockets | std::ranges::views::keys) {
                    operations.push_back(Recv<T>(p, rank, u...));
                }
                for (auto &operation: operations) {
                    co_await operation;
                }
            }
            co_await o;
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, U...>
        MPIOperation<void> Gather(MPIFakePacket p, MPIRankIDType root, const std::unordered_map<MPIRankIDType, std::tuple<U...>> &u) {
            std::unordered_map<MPIRankIDType, T> result;
            auto o = std::apply([this, &p, &root](auto &&...args) { return this->Send<T>(p, root, std::forward<decltype(args)>(args)...); }, u.at(rankID));
            if (rankID == root) {
                std::vector<MPIOperation<void>> operations;
                for (auto rank: sockets | std::ranges::views::keys) {
                    operations.push_back(std::apply([this, &p, &rank](auto &&...args) { return this->Recv<T>(p, rank, std::forward<decltype(args)>(args)...); }, u.at(rank)));
                }
                for (auto &operation: operations) {
                    co_await operation;
                }
            }
            co_await o;
        }

        template<typename T, MPIObject T_=std::decay_t<T>>
        MPIOperation<std::unordered_map<MPIRankIDType, T_>> AllGather(T &&data) {
            std::unordered_map<MPIRankIDType, MPIOperation<std::unordered_map<MPIRankIDType, T_>>> operations;
            for (auto rank: sockets | std::ranges::views::keys) {
                operations[rank] = Gather(rank, data);
            }
            for (auto &operation: operations | std::ranges::views::values) {
                co_await operation;
            }
            co_return co_await operations[rankID];
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, std::decay_t<U>...>
        MPIOperation<void> AllGather(MPIFakePacket p, U &&...u) {
            std::vector<MPIOperation<void>> operations;
            for (auto rank: sockets | std::ranges::views::keys) {
                operations.push_back(Gather<T>(p, rank, u...));
            }
            for (auto &operation: operations) {
                co_await operation;
            }
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, U...>
        MPIOperation<void> AllGather(MPIFakePacket p, const std::unordered_map<MPIRankIDType, std::tuple<U...>> &u) {
            std::vector<MPIOperation<void>> operations;
            for (auto rank: sockets | std::ranges::views::keys) {
                operations.push_back(Gather<T>(p, rank, u));
            }
            for (auto &operation: operations) {
                co_await operation;
            }
        }

        template<MPIObject T>
        MPIOperation<T> Scatter(MPIRankIDType root, const std::unordered_map<MPIRankIDType, T> &data) {
            auto o = Recv<T>(root);
            if (rankID == root) {
                std::vector<MPIOperation<void>> operations;
                for (auto rank: sockets | std::ranges::views::keys) {
                    operations.push_back(Send(rank, data.at(rank)));
                }
                for (auto &operation: operations) {
                    co_await operation;
                }
            }
            co_return co_await o;
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, std::decay_t<U>...>
        MPIOperation<void> Scatter(MPIFakePacket p, MPIRankIDType root, U &&...u) {
            auto o = Recv<T>(p, root, u...);
            if (rankID == root) {
                std::vector<MPIOperation<void>> operations;
                for (auto rank: sockets | std::ranges::views::keys) {
                    operations.push_back(Send<T>(p, rank, u...));
                }
                for (auto &operation: operations) {
                    co_await operation;
                }
            }
            co_await o;
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, U...>
        MPIOperation<void> Scatter(MPIFakePacket p, MPIRankIDType root, const std::unordered_map<MPIRankIDType, std::tuple<U...>> &u) {
            auto o = std::apply([this, &p, &root](auto &&...args) { return this->Recv<T>(p, root, std::forward<decltype(args)>(args)...); }, u.at(rankID));
            if (rankID == root) {
                std::vector<MPIOperation<void>> operations;
                for (auto rank: sockets | std::ranges::views::keys) {
                    operations.push_back(std::apply([this, &p, &rank](auto &&...args) { return this->Send<T>(p, rank, std::forward<decltype(args)>(args)...); }, u.at(rank)));
                }
                for (auto &operation: operations) {
                    co_await operation;
                }
            }
            co_await o;
        }

        template<MPIObject T>
        MPIOperation<T> Broadcast(MPIRankIDType root, const std::optional<T> &data) {
            auto o = Recv<T>(root);
            if (rankID == root) {
                std::vector<MPIOperation<void>> operations;
                for (auto rank: sockets | std::ranges::views::keys) {
                    operations.push_back(Send(rank, data.value()));
                }
                for (auto &operation: operations) {
                    co_await operation;
                }
            }
            co_return co_await o;
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, std::decay_t<U>...>
        MPIOperation<void> Broadcast(MPIFakePacket p, MPIRankIDType root, U &&...u) {
            auto o = Recv<T>(p, root, u...);
            if (rankID == root) {
                std::vector<MPIOperation<void>> operations;
                for (auto rank: sockets | std::ranges::views::keys) {
                    operations.push_back(Send<T>(p, rank, u...));
                }
                for (auto &operation: operations) {
                    co_await operation;
                }
            }
            co_await o;
        }

        MPIOperation<void> Barrier();

        template<MPIOperator O, typename T, typename ...U>
        requires MPIObject<std::decay_t<T>> && MPIOperatorApplicable<O, std::decay_t<T>, U...>
        MPIOperation<std::optional<std::decay_t<T>>> Reduce(MPIRankIDType root, T &&data, U &&... u) {
            using T_ = std::decay_t<T>;
            std::unordered_map<MPIRankIDType, T_> result = co_await Gather(root, std::forward<T>(data));
            if (rankID == root) {
                co_return MPIOperatorImplementation<O, T_>{}(result | std::views::values, std::forward<U>(u)...);
            }
            co_return std::nullopt;
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, std::decay_t<U>...>
        MPIOperation<void> Reduce(MPIFakePacket p, MPIRankIDType root, U &&...u) {
            co_await Gather<T>(p, root, std::forward<U>(u)...);
        }

        template<MPIOperator O, MPIObject T, typename ...P>
        requires MPIOperatorApplicable<O, T, P...>
        MPIOperation<T> ReduceScatter(const std::unordered_map<MPIRankIDType, T> &data, P &&...p) {
            std::unordered_map<MPIRankIDType, MPIOperation<T>> operations;
            for (auto &[rank, d]: data) {
                operations[rank] = Reduce<O>(rank, d, p...).then([](auto &&o) { return o.value(); });
            }
            for (auto &o: operations | std::ranges::views::values) {
                co_await o;
            }
            co_return co_await operations[rankID];
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, U...>
        MPIOperation<void> ReduceScatter(MPIFakePacket p, const std::unordered_map<MPIRankIDType, std::tuple<U...>> &u) {
            std::vector<MPIOperation<void>> operations;
            for (auto &[rank, u_]: u) {
                operations.push_back(std::apply([this, &p, &rank](auto &&...args) { return this->Reduce<T>(p, rank, std::forward<decltype(args)>(args)...); }, u_));
            }
            for (auto &o: operations) {
                co_await o;
            }
        }

        template<typename T, MPIObject T_ = std::decay_t<T>>
        MPIOperation<MPIRankIDType> Elect(T &&votes) {
            std::unordered_map<MPIRankIDType, MPIOperation<std::unordered_map<MPIRankIDType, T_>>> operations;
            for (auto rank: sockets | std::ranges::views::keys) {
                operations[rank] = Gather(rank, std::forward<T>(votes));
            }
            for (auto &[_, operation]: operations) {
                co_await operation;
            }
            co_return std::ranges::max_element(operations[rankID].result(), [](auto &p1, auto &p2) { return p1.second == p2.second ? p1.first < p2.first : p1.second < p2.second; })->first;
        }

        template<MPIOperator O, typename T, typename ...U>
        requires MPIObject<std::decay_t<T>> && MPIOperatorApplicable<O, T, U...>
        MPIOperation<std::decay_t<T>> AllReduce(T &&data, U &&... u) {
            MPIRankIDType root = co_await Elect(voteGenerator(*randomEngine));
            auto result = co_await Reduce<O, T>(root, std::forward<T>(data), std::forward<U>(u)...);
            co_return std::move(co_await Broadcast(root, std::move(result)));
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, std::decay_t<U>...>
        MPIOperation<void> AllReduce(MPIFakePacket p, U &&...u) {
            MPIRankIDType root = co_await Elect(voteGenerator(*randomEngine));
            co_await Reduce<T>(p, root, u...);
            co_await Broadcast<T>(p, root, u...);
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<std::vector<T>, std::size_t, std::decay_t<U>...>
        MPIOperation<void> RingAllReduce(MPIFakePacket p, std::size_t size, U &&...u) {
            auto partition = size / GroupSize();
            auto index = std::count_if(ranks.begin(), ranks.end(), [this](const auto &item) { return item < rankID; });
            auto send_target = ranks[(index + 1) % GroupSize()];
            auto receive_target = ranks[(index + GroupSize() - 1) % GroupSize()];
            for (std::size_t i = 0; i < GroupSize() - 1; ++i) {
                auto send_partition_index = (index + GroupSize() - i) % GroupSize();
                auto receive_partition_index = (index + GroupSize() - i - 1) % GroupSize();
                auto send_offset = partition * send_partition_index;
                auto send_partition_size = std::min(partition, size - send_offset);
                auto receive_offset = partition * receive_partition_index;
                auto receive_partition_size = std::min(partition, size - receive_offset);
                auto send = Send<std::vector<T>>(p, send_target, send_partition_size, u...);
                auto receive = Recv<std::vector<T>>(p, receive_target, receive_partition_size, u...);
                co_await send;
                co_await receive;
            }
            std::unordered_map<MPIRankIDType, std::tuple<std::size_t, std::decay_t<U>...>> parameter_map{};
            for (std::size_t i = 0; i < GroupSize(); ++i) {
                auto rank = ranks[i];
                auto complete_index = (i + 1) % GroupSize();
                auto complete_offset = partition * complete_index;
                auto complete_size = std::min(partition, size - complete_offset);
                parameter_map[rank] = std::make_tuple(complete_size, u...);
            }
            co_await AllGather<std::vector<T>>(p, parameter_map);
        }

        template<MPIWritable S, MPIReadable R>
        MPIOperation<std::unordered_map<MPIRankIDType, R>> AllToAll(const std::unordered_map<MPIRankIDType, S> &data) {
            std::vector<MPIOperation<void>> sendOperations;
            std::unordered_map<MPIRankIDType, MPIOperation<R>> recvOperations;
            for (auto &[rank, s]: data) {
                sendOperations.push_back(Send(rank, s));
                recvOperations[rank] = Recv<R>(rank);
            }
            std::unordered_map<MPIRankIDType, R> result;
            for (auto &o: sendOperations) {
                co_await o;
            }
            for (auto &[rank, o]: recvOperations) {
                result[rank] = std::move(co_await o);
            }
            co_return result;
        }

        template<MPIObject T>
        MPIOperation<std::unordered_map<MPIRankIDType, T>> AllToAll(const std::unordered_map<MPIRankIDType, T> &data) {
            return AllToAll<T, T>(data);
        }

        template<typename S, typename R, typename ...US, typename ...UR>
        requires MPIFakeWritable<S, US...> and MPIFakeReadable<R, UR...>
        MPIOperation<void> AllToAll(MPIFakePacket p, const std::unordered_map<MPIRankIDType, std::tuple<US...>> &uS, const std::unordered_map<MPIRankIDType, std::tuple<UR...>> &uR) {
            std::vector<MPIOperation<void>> operations;
            for (auto &[rank, u]: uS) {
                operations.push_back(std::apply([this, &p, &rank](auto &&...args) { return this->Send<S>(p, rank, std::forward<decltype(args)>(args)...); }, u));
            }
            for (auto &[rank, u]: uR) {
                operations.push_back(std::apply([this, &p, &rank](auto &&...args) { return this->Recv<R>(p, rank, std::forward<decltype(args)>(args)...); }, u));
            }
            for (auto &o: operations) {
                co_await o;
            }
        }

        template<typename S, typename R, typename ...US, typename ...UR>
        requires MPIFakeWritable<S, US...> and MPIFakeReadable<R, UR...>
        MPIOperation<void> AllToAll(MPIFakePacket p, const std::tuple<US...> &uS, const std::tuple<UR...> &uR) {
            std::unordered_map<MPIRankIDType, std::tuple<US...>> uSMap;
            std::unordered_map<MPIRankIDType, std::tuple<UR...>> uRMap;
            for (auto rank: sockets | std::ranges::views::keys) {
                uSMap[rank] = uS;
                uRMap[rank] = uR;
            }
            return AllToAll<S, R>(p, uSMap, uRMap);
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, U...>
        MPIOperation<void> AllToAll(MPIFakePacket p, const std::unordered_map<MPIRankIDType, std::tuple<U...>> &uS, const std::unordered_map<MPIRankIDType, std::tuple<U...>> &uR) {
            return AllToAll<T, T>(p, uS, uR);
        }

        template<typename T, typename ...U>
        requires MPIFakeObject<T, U...>
        MPIOperation<void> AllToAll(MPIFakePacket p, const std::tuple<U...> &uS, const std::tuple<U...> &uR) {
            std::unordered_map<MPIRankIDType, std::tuple<U...>> uSMap;
            std::unordered_map<MPIRankIDType, std::tuple<U...>> uRMap;
            for (auto rank: sockets | std::ranges::views::keys) {
                uSMap[rank] = uS;
                uRMap[rank] = uR;
            }
            return AllToAll<T, T>(p, uSMap, uRMap);
        }

        void Block() noexcept;

        void Unblock() noexcept;

        std::size_t TxBytes() const noexcept;

        std::size_t RxBytes() const noexcept;

        void Close() noexcept;

        MPIRankIDType RankID() const noexcept;

        std::set<MPIRankIDType> GroupMembers() const noexcept;

        inline std::size_t GroupSize() const noexcept {
            return sockets.size();
        }

        virtual ~MPICommunicator() = default;
    };
}

#endif // NS3_MPI_APPLICATION_COMMUNICATOR_H
