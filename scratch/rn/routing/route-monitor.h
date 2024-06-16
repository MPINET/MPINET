#ifndef NS3_ROUTE_MONITOR_H
#define NS3_ROUTE_MONITOR_H

#include <functional>
#include <tuple>

#include <ns3/internet-module.h>

#include "ipv4-routing-utility.h"

namespace ns3 {

    template<typename T>
    concept HeaderConcept=std::is_base_of_v<Header, T>;

    template<typename ...Headers>
    using RouteMonitorCallback=std::function<void(Ptr<const NetDevice>, Ptr<const NetDevice>, const Headers &...)>;

    // Place this first in the list routing
    template<HeaderConcept ...Headers>
    class RouteMonitor : public Ipv4RoutingProtocol {
    private:
        RouteMonitorCallback<Headers...> callback;

        template<typename HeaderTuple, std::size_t...I>
        static void PeekHeader(Ptr<const Packet> p, HeaderTuple &headers, std::index_sequence<I...>) {
            (p->PeekHeader(std::get<I>(headers)), ...);
        }
    public:
        explicit RouteMonitor(RouteMonitorCallback<Headers...> callback) : callback(std::move(callback)) {}

        Ptr<Ipv4Route> RouteOutput(Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr) override {
            return nullptr;
        }

        bool RouteInput(Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev, const UnicastForwardCallback &ucb, const MulticastForwardCallback &mcb, const LocalDeliverCallback &lcb, const ErrorCallback &ecb) override {
            if (callback) {
                std::tuple<Headers...> headers;
                PeekHeader(p, headers, std::make_index_sequence<sizeof...(Headers)>());
                std::apply(callback, std::tuple_cat(std::make_tuple(RemoteDevice(idev), idev), headers));
            }
            return false;
        }

        void NotifyInterfaceUp(uint32_t interface) override {}

        void NotifyInterfaceDown(uint32_t interface) override {}

        void NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) override {}

        void NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) override {}

        void SetIpv4(Ptr<Ipv4> ipv4) override {}

        void PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const override {}

        ~RouteMonitor() override = default;
    };
}

#endif //NS3_ROUTE_MONITOR_H
