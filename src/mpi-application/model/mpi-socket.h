//
// Created by Ricardo Evans on 2023/6/7.
//

#ifndef NS3_MPI_APPLICATION_SOCKET_H
#define NS3_MPI_APPLICATION_SOCKET_H

#include <deque>
#include <functional>
#include <tuple>

#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/type-id.h"

#include "mpi-operation.h"

namespace ns3 {

    class MPISocket {
    private:
        using NS3Socket = Ptr<Socket>;
        using NS3Packet = Ptr<Packet>;
        using NS3Node = Ptr<Node>;
        using NS3Error = Socket::SocketErrno;
        using AcceptOperation = MPIOperation<std::tuple<MPISocket, Address, NS3Error>>;
        using AcceptOperationQueue = std::deque<AcceptOperation>;
        using ConnectOperation = MPIOperation<NS3Error>;
        using ConnectOperationQueue = std::deque<ConnectOperation>;
        using SendOperation = MPIOperation<std::tuple<std::size_t, NS3Error>>;
        using SendOperationQueue = std::deque<SendOperation>;
        using ReceiveOperation = MPIOperation<std::tuple<NS3Packet, NS3Error>>;
        using ReceiveOperationQueue = std::deque<ReceiveOperation>;

        NS3Socket socket;
        bool blocked = false;
        bool connected = false;
        bool listening = false;
        bool closed = false;
        AcceptOperationQueue pendingAccept;
        ConnectOperationQueue pendingConnect;
        SendOperationQueue pendingSend;
        ReceiveOperationQueue pendingReceive;

        NS3Packet cache;
        size_t cacheLimit = 212992;

        size_t tx_size = 0;
        size_t rx_size = 0;

        void registerCallbacks() noexcept;

        void clearCallbacks() noexcept;

        void onAccept(NS3Socket s, const Address &remoteAddress);

        void onConnect(NS3Error error);

        void onSend();

        void onReceive();

        void onClose(NS3Error error);

    public:
        /**
         * Loopback socket
         */
        MPISocket(size_t cacheLimit = 212992) noexcept;

        explicit MPISocket(const NS3Node &node, TypeId typeId, size_t cacheLimit = 212992) noexcept;

        explicit MPISocket(const NS3Socket &socket, size_t cacheLimit = 212992) noexcept;

        MPISocket(MPISocket &&);

        MPISocket(const MPISocket &) = delete;

        MPISocket &operator=(MPISocket &&);

        MPISocket &operator=(const MPISocket &) = delete;

        AcceptOperation accept();

        NS3Error bind(const Address &address) noexcept;

        ConnectOperation connect(const Address &address) noexcept;

        SendOperation send(NS3Packet packet) noexcept;

        ReceiveOperation receive(std::size_t size = 0) noexcept;

        NS3Error close() noexcept;

        NS3Error closeSend() noexcept;

        NS3Error closeReceive() noexcept;

        size_t txBytes() const noexcept;

        size_t rxBytes() const noexcept;

        void block() noexcept;

        void unblock() noexcept;

        constexpr inline bool isBlocked() const noexcept {
            return blocked;
        }

        constexpr inline bool isClosed() const noexcept {
            return closed;
        }

        constexpr inline bool isConnected() const noexcept {
            return connected;
        }

        constexpr inline bool isListening() const noexcept {
            return listening;
        }

        virtual ~MPISocket() noexcept;
    };
}

#endif //NS3_MPI_APPLICATION_SOCKET_H
