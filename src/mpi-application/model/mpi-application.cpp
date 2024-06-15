//
// Created by Ricardo Evans on 2023/6/29.
//

#include <vector>
#include <unordered_map>

#include "ns3/tcp-socket.h"
#include "ns3/tcp-socket-factory.h"

#include "mpi-application.h"
#include "mpi-util.h"

ns3::MPIApplication::MPIApplication(ns3::MPIRankIDType rankID, std::map<ns3::MPIRankIDType, Address> &&addresses, std::map<Address, ns3::MPIRankIDType> &&ranks, std::queue<std::function<MPIOperation<void>(MPIApplication &)>> &&functions) noexcept:
        MPIApplication(rankID, std::move(addresses), std::move(ranks), std::move(functions), std::mt19937::default_seed ^ rankID) {}

ns3::MPIApplication::MPIApplication(ns3::MPIRankIDType rankID, std::map<ns3::MPIRankIDType, Address> &&addresses, std::map<Address, ns3::MPIRankIDType> &&ranks, std::queue<MPIFunction> &&functions, std::mt19937::result_type seed) noexcept:
        rankID(rankID),
        addresses(std::move(addresses)),
        ranks(std::move(ranks)),
        functions(std::move(functions)),
        randomEngine(std::make_shared<std::mt19937>(seed)) {}

ns3::MPIApplication::MPIApplication(ns3::MPIRankIDType rankID, const std::map<ns3::MPIRankIDType, Address> &addresses, const std::map<Address, ns3::MPIRankIDType> &ranks, std::queue<std::function<MPIOperation<void>(MPIApplication &)>> &&functions) noexcept:
        MPIApplication(rankID, addresses, ranks, std::move(functions), std::mt19937::default_seed ^ rankID) {}

ns3::MPIApplication::MPIApplication(ns3::MPIRankIDType rankID, const std::map<ns3::MPIRankIDType, Address> &addresses, const std::map<Address, ns3::MPIRankIDType> &ranks, std::queue<std::function<MPIOperation<void>(MPIApplication &)>> &&functions, std::mt19937::result_type seed) noexcept:
        rankID(rankID),
        addresses(addresses),
        ranks(ranks),
        functions(std::move(functions)),
        randomEngine(std::make_shared<std::mt19937>(seed)) {}

void ns3::MPIApplication::StartApplication() {
    running = true;
    run();
}

void ns3::MPIApplication::StopApplication() {
    running = false;
}

ns3::MPIOperation<void> ns3::MPIApplication::run() {
    std::cout << "mpi application of rank " << rankID << " total functions: " << functions.size() << std::endl;
    auto start = ns3::Now();
    while (!functions.empty()) {
        if (!running) {
            break;
        }
        co_await functions.front()(*this);
        functions.pop();
        std::cout << "mpi application of rank " << rankID << " remaining functions: " << functions.size() << " now time: " << ns3::Now() << std::endl;
    }
    auto end = ns3::Now();
    std::cout << "mpi application of rank " << rankID << " start time: " << start << ", end time: " << end << std::endl;
    running = false;
}

ns3::MPIOperation<void> ns3::MPIApplication::Initialize(size_t mtu) {
    if (status != Status::INITIAL) {
        throw std::runtime_error("MPIApplication::Init() should only be called once");
    }
    auto cache_limit = mtu * 100;
    auto &self = addresses[rankID];
    MPISocket listener{GetNode(), TcpSocketFactory::GetTypeId(), cache_limit};
    listener.bind(self);
    std::vector<MPIOperation<void>> operations;
    std::unordered_map<MPIRankIDType, MPISocket> selfSockets;
    std::unordered_map<MPIRankIDType, MPISocket> worldSockets;
    for (auto &[rank, address]: addresses) {
        if (rank < rankID) {
            operations.push_back(listener.accept().then([&worldSockets, this](auto d) {
                auto &[s, a, e] = d;
                if (e != NS3Error::ERROR_NOTERROR) {
                    throw std::runtime_error("error when accepting connection from other ranks");
                }
                auto ip = retrieveIPAddress(a);
                NS_ASSERT_MSG(ranks.contains(ip), "rank not found");
                worldSockets[ranks[ip]] = std::move(s);
            }));
        }
        if (rank > rankID) {
            worldSockets[rank] = MPISocket{GetNode(), TcpSocketFactory::GetTypeId(), cache_limit};
            operations.push_back(worldSockets[rank].connect(address).then([](auto e) {
                if (e != NS3Error::ERROR_NOTERROR) {
                    throw std::runtime_error("error when connecting to other ranks");
                }
            }));
        }
    }
    for (auto &o: operations) {
        co_await o;
    }
    operations.clear();
    worldSockets[rankID] = MPISocket{cache_limit}; // loopback
    selfSockets[rankID] = MPISocket{cache_limit}; // loopback
    listener.close();
    NS_ASSERT_MSG(selfSockets.size() == 1, "self sockets size is not correct");
    NS_ASSERT_MSG(worldSockets.size() == addresses.size(), "world sockets size is not correct");
    communicators.emplace(std::piecewise_construct, std::forward_as_tuple(NULL_COMMUNICATOR), std::forward_as_tuple());
    communicators.emplace(std::piecewise_construct, std::forward_as_tuple(WORLD_COMMUNICATOR), std::forward_as_tuple(rankID, randomEngine, std::move(worldSockets)));
    communicators.emplace(std::piecewise_construct, std::forward_as_tuple(SELF_COMMUNICATOR), std::forward_as_tuple(rankID, randomEngine, std::move(selfSockets)));
    status = Status::WORKING;
}

void ns3::MPIApplication::Finalize() {
    if (status != Status::WORKING) {
        throw std::runtime_error("MPIApplication::Finalize() should only be called after MPIApplication::Init()");
    }
    for (auto &[_, communicator]: communicators) {
        communicator.Close();
    }
    status = Status::FINALIZED;
}

void ns3::MPIApplication::Block() noexcept {
    std::ranges::for_each(communicators | std::ranges::views::values, &MPICommunicator::Block);
}

void ns3::MPIApplication::Unblock() noexcept {
    std::ranges::for_each(communicators | std::ranges::views::values, &MPICommunicator::Unblock);
}

ns3::MPICommunicator &ns3::MPIApplication::communicator(const ns3::MPICommunicatorIDType id) {
    if (!Initialized()) {
        throw std::domain_error("MPIApplication::communicator can only be called after initialized");
    }
    return communicators[id];
}

bool ns3::MPIApplication::Initialized() const noexcept {
    return status == Status::WORKING;
}

bool ns3::MPIApplication::Finalized() const noexcept {
    return status == Status::FINALIZED;
}
