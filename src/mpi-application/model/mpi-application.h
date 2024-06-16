#ifndef NS3_MPI_APPLICATION_H
#define NS3_MPI_APPLICATION_H

#include <chrono>
#include <coroutine>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <random>
#include <unordered_map>

#include "ns3/application.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"

#include "mpi-communicator.h"
#include "mpi-operation.h"

namespace ns3 {
    using MPIRequestIDType = uint64_t;

    static const constinit MPIRequestIDType NULL_REQUEST = 1;

    class MPIApplication;

    using MPIFunction = std::function<MPIOperation<void>(MPIApplication &)>;

    class MPIApplication : public Application {
    public:
        enum class Status {
            INITIAL,
            WORKING,
            FINALIZED,
        };
    private:
        bool running = false;
        Status status = Status::INITIAL;
        MPIRankIDType rankID;
        std::map<MPIRankIDType, Address> addresses;
        std::map<Address, MPIRankIDType> ranks;
        std::queue<MPIFunction> functions;
        std::shared_ptr<std::mt19937> randomEngine;
        std::unordered_map<MPICommunicatorIDType, MPICommunicator> communicators;
    public:
        std::unordered_map<MPIRequestIDType, MPIOperation<void>> requests;

        MPIApplication(
                MPIRankIDType rankID,
                std::map<MPIRankIDType, Address> &&addresses,
                std::map<Address, MPIRankIDType> &&ranks,
                std::queue<std::function<MPIOperation<void>(MPIApplication &)>> &&functions) noexcept;

        MPIApplication(
                MPIRankIDType rankID,
                std::map<MPIRankIDType, Address> &&addresses,
                std::map<Address, MPIRankIDType> &&ranks,
                std::queue<std::function<MPIOperation<void>(MPIApplication &)>> &&functions,
                std::mt19937::result_type seed) noexcept;

        MPIApplication(
                MPIRankIDType rankID,
                const std::map<MPIRankIDType, Address> &addresses,
                const std::map<Address, MPIRankIDType> &ranks,
                std::queue<std::function<MPIOperation<void>(MPIApplication &)>> &&functions) noexcept;

        MPIApplication(
                MPIRankIDType rankID,
                const std::map<MPIRankIDType, Address> &addresses,
                const std::map<Address, MPIRankIDType> &ranks,
                std::queue<std::function<MPIOperation<void>(MPIApplication &)>> &&functions,
                std::mt19937::result_type seed) noexcept;

        MPIApplication(const MPIApplication &application) = delete;

        MPIApplication(MPIApplication &&application) noexcept = default;

        MPIApplication &operator=(const MPIApplication &application) = delete;

        MPIApplication &operator=(MPIApplication &&application) noexcept = default;

        void StartApplication() override;

        void StopApplication() override;

        MPIOperation<void> run();

        MPIOperation<void> Initialize(size_t mtu = 1492);

        void Finalize();

        void Block() noexcept;

        void Unblock() noexcept;

        template<typename R, typename P>
        MPIOperation<void> Compute(const std::chrono::duration<R, P> &duration) {
            auto o = makeMPIOperation<void>();
            Simulator::Schedule(convert(duration), [o]() { o.terminate(); });
            co_await o;
        }

        MPICommunicator &communicator(const MPICommunicatorIDType id);

        bool Initialized() const noexcept;

        bool Finalized() const noexcept;

        ~MPIApplication() override = default;
    };
}

#endif // NS3_MPI_APPLICATION_H
