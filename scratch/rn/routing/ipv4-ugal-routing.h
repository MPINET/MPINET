#ifndef NS3_IPV4_UGAL_ROUTING_H
#define NS3_IPV4_UGAL_ROUTING_H

#include <compare>
#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <ns3/internet-module.h>

#include "ipv4-routing-utility.h"

namespace ns3 {
    // actual routing protocol
    template<typename T=double>
    class Ipv4UGALRouting : public Ipv4RoutingProtocol {
    private:
        using NodeID = uint32_t;
        using CongestionMap = std::unordered_map<NodeID, std::unordered_map<NodeID, T>>;
        using LinkMap = std::unordered_map<NodeID, std::unordered_map<NodeID, std::pair<Ipv4Address, Ipv4Address>>>;
        using AddressMap = std::unordered_map<Ipv4Address, NodeID>;

        struct RouteInfo {
            NodeID current;
            NodeID nextHop;
            std::size_t hops;
            T cost;
        };

        struct MinHops {
            bool operator()(const RouteInfo &lhs, const RouteInfo &rhs) const {
                return lhs.hops > rhs.hops;
            }
        };

        struct MinCongestion {
            bool operator()(const RouteInfo &lhs, const RouteInfo &rhs) const {
                return lhs.cost > rhs.cost;
            }
        };

        bool flowRouting; // whether to use flow routing
        bool ecmp; // whether to use ECMP
        T bias; // bias for the cost of choosing the path with minimum congestion
        std::mt19937 generator{};
        std::function<void()> linkChangeCallback; // callback function to be called when link changes, can be empty
        std::shared_ptr<CongestionMap> congestion; // reversed congestion map: current node -> previous hop -> congestion from previous hop to current node
        std::shared_ptr<LinkMap> links; // link map: current node -> next hop -> the addresses connecting current node and next hop
        std::shared_ptr<AddressMap> addresses; // address map: address -> node id

        std::unordered_map<Ipv4Flow, Ptr<Ipv4Route>> outputFlows;
        std::unordered_map<Ipv4Flow, Ptr<Ipv4Route>> inputUnicastFlows;
        std::unordered_map<Ipv4Flow, Ptr<Ipv4MulticastRoute>> inputMulticastFlows;
        std::unordered_map<Ipv4Flow, uint32_t> inputLocalFlows;

        Ptr<Ipv4> m_ipv4;

        // dijkstra algorithm
        Ptr<Ipv4Route> findRoute(Ipv4Address destination, Ptr<const NetDevice> dev) {
            auto interface = m_ipv4->GetInterfaceForDevice(dev);
            NS_ASSERT_MSG(interface >= 0, "Invalid interface");
            auto currentNode = dev->GetNode()->GetId();
            NS_ASSERT_MSG(addresses->contains(destination), "Unknown destination address: " << destination);
            auto destinationNode = (*addresses)[destination];
            int32_t outputInterface;
            if (currentNode == destinationNode) {
                outputInterface = m_ipv4->GetInterfaceForAddress(destination);
                NS_ASSERT_MSG(outputInterface >= 0, "Invalid interface");
            } else {
                std::optional<RouteInfo> route = std::nullopt;
                if (ecmp) {
                    auto mcRoutes = minCongestionECMPRoute(currentNode, destinationNode);
                    auto mhRoutes = minHopsECMPRoute(currentNode, destinationNode);
                    std::optional<RouteInfo> mcRoute;
                    std::ranges::sample(mcRoutes, OptionalInserter{mcRoute}, 1, generator);
                    std::optional<RouteInfo> mhRoute;
                    std::ranges::sample(mhRoutes, OptionalInserter(mhRoute), 1, generator);
                    if (mcRoute && mhRoute) {
                        route = mcRoute->cost + bias < mhRoute->cost ? mcRoute : mhRoute;
                    }
                } else {
                    auto mcRoute = minCongestionRoute(currentNode, destinationNode);
                    auto mhRoute = minHopsRoute(currentNode, destinationNode);
                    if (mcRoute && mhRoute) {
                        route = mcRoute->cost + bias < mhRoute->cost ? mcRoute : mhRoute;
                    }
                }
                if (!route) {
                    return nullptr;
                }
                auto [outputAddress, nextHopAddress] = (*links)[route->current][route->nextHop];
                outputInterface = m_ipv4->GetInterfaceForAddress(outputAddress);
            }
            auto ipv4Route = Create<Ipv4Route>();
            ipv4Route->SetDestination(destination);
            ipv4Route->SetGateway(Ipv4Address::GetZero());
            ipv4Route->SetSource(m_ipv4->SourceAddressSelection(outputInterface, ipv4Route->GetDestination()));
            ipv4Route->SetOutputDevice(m_ipv4->GetNetDevice(outputInterface));
            return ipv4Route;
        }

    public:
        Ipv4UGALRouting(bool flowRouting, bool ecmp, T bias, const std::function<void()> &linkChangeCallback, const std::shared_ptr<CongestionMap> &congestion, const std::shared_ptr<std::unordered_map<NodeID, std::unordered_map<NodeID, std::pair<Ipv4Address, Ipv4Address>>>> &links, const std::shared_ptr<std::unordered_map<Ipv4Address, NodeID>> &addresses) : flowRouting(flowRouting), ecmp(ecmp), bias(bias), linkChangeCallback(linkChangeCallback), congestion(congestion), links(links), addresses(addresses) {}

        Ptr<Ipv4Route> RouteOutput(Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr) override {
            if (!m_ipv4) {
                sockerr = Socket::ERROR_AFNOSUPPORT;
                return nullptr;
            }
            auto source = header.GetSource();
            auto destination = header.GetDestination();
            if (flowRouting && outputFlows.contains({source, destination})) {
                sockerr = Socket::ERROR_NOTERROR;
                return outputFlows[{source, destination}];
            }
            Ptr <Ipv4Route> route = nullptr;
            if (oif != nullptr) {
                route = findRoute(destination, oif);
            } else if (m_ipv4->GetNInterfaces() > 0) {
                route = findRoute(destination, m_ipv4->GetNetDevice(0));
                if (route->GetOutputDevice()->IsLinkUp()) {
                    oif = route->GetOutputDevice();
                }
            }
            if (!route) {
                sockerr = Socket::ERROR_NOROUTETOHOST;
                return nullptr;
            }
            if (oif != route->GetOutputDevice() || !oif->IsLinkUp()) {
                sockerr = Socket::ERROR_NOROUTETOHOST;
                return nullptr;
            }
            if (flowRouting) {
                outputFlows[{source, destination}] = route;
            }
            sockerr = Socket::ERROR_NOTERROR;
            return route;
        }

        bool RouteInput(Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev, const UnicastForwardCallback &ucb, const MulticastForwardCallback &mcb, const LocalDeliverCallback &lcb, const ErrorCallback &ecb) override {
            auto source = header.GetSource();
            auto destination = header.GetDestination();
            auto interface = m_ipv4->GetInterfaceForDevice(idev);
            if (flowRouting && inputLocalFlows.contains({source, destination})) {
                if (!lcb.IsNull()) {
                    lcb(p, header, inputLocalFlows[{source, destination}]);
                }
                return true;
            }
            if (m_ipv4->IsDestinationAddress(destination, interface)) {
                if (!lcb.IsNull()) {
                    lcb(p, header, interface);
                }
                return true;
            }
            if (!m_ipv4->IsForwarding(interface)) {
                if (!ecb.IsNull()) {
                    ecb(p, header, Socket::ERROR_NOROUTETOHOST);
                }
                return true;
            }
            if (flowRouting && inputUnicastFlows.contains({source, destination})) {
                if (!ucb.IsNull()) {
                    ucb(inputUnicastFlows[{source, destination}], p, header);
                }
                return true;
            }
            auto route = findRoute(destination, idev);
            if (route) {
                if (flowRouting) {
                    inputUnicastFlows[{source, destination}] = route;
                }
                if (!ucb.IsNull()) {
                    ucb(route, p, header);
                }
                return true;
            }
            return false;
        }

        std::optional<RouteInfo> minCongestionRoute(NodeID current, NodeID destination) const {
            std::unordered_map<NodeID, RouteInfo> routes;
            std::priority_queue<RouteInfo, std::vector<RouteInfo>, MinCongestion> queue;
            queue.push({destination, destination, 0, {}});
            while (!queue.empty()) {
                auto route = std::move(queue.top());
                queue.pop();
                if (current == route.current) {
                    return route;
                }
                if (routes.contains(route.current)) {
                    continue;
                }
                routes[route.current] = route;
                for (const auto &[previousHop, cost]: (*congestion)[route.current]) {
                    queue.push({previousHop, route.current, route.hops + 1, route.cost + cost});
                }
            }
            return std::nullopt;
        }

        std::vector<RouteInfo> minCongestionECMPRoute(NodeID current, NodeID destination) const {
            std::unordered_map<NodeID, RouteInfo> routes;
            std::priority_queue<RouteInfo, std::vector<RouteInfo>, MinCongestion> queue;
            queue.push({destination, destination, 0, {}});
            std::optional<T> minimumCost = std::nullopt;
            std::vector<RouteInfo> results{};
            while (!queue.empty()) {
                auto route = std::move(queue.top());
                queue.pop();
                if (minimumCost && route.cost > *minimumCost) {
                    break;
                }
                if (current == route.current) {
                    minimumCost = route.cost;
                    results.emplace_back(std::move(route));
                }
                if (routes.contains(route.current)) {
                    continue;
                }
                routes[route.current] = route;
                for (const auto &[previousHop, cost]: (*congestion)[route.current]) {
                    queue.push({previousHop, route.current, route.hops + 1, route.cost + cost});
                }
            }
            return results;
        }

        std::optional<RouteInfo> minHopsRoute(NodeID current, NodeID destination) const {
            std::unordered_map<NodeID, RouteInfo> routes;
            std::priority_queue<RouteInfo, std::vector<RouteInfo>, MinHops> queue;
            queue.push({destination, destination, 0, {}});
            while (!queue.empty()) {
                auto route = std::move(queue.top());
                queue.pop();
                if (current == route.current) {
                    return route;
                }
                if (routes.contains(route.current)) {
                    continue;
                }
                routes[route.current] = route;
                for (const auto &[previousHop, cost]: (*congestion)[route.current]) {
                    queue.push({previousHop, route.current, route.hops + 1, route.cost + cost});
                }
            }
            return std::nullopt;
        }

        std::vector<RouteInfo> minHopsECMPRoute(NodeID current, NodeID destination) const {
            std::unordered_map<NodeID, RouteInfo> routes;
            std::priority_queue<RouteInfo, std::vector<RouteInfo>, MinHops> queue;
            queue.push({destination, destination, 0, {}});
            auto minimumHops = std::numeric_limits<decltype(std::declval<RouteInfo>().hops)>::max();
            std::vector<RouteInfo> results{};
            while (!queue.empty()) {
                auto route = std::move(queue.top());
                queue.pop();
                if (route.hops > minimumHops) {
                    break;
                }
                if (current == route.current) {
                    minimumHops = route.hops;
                    results.emplace_back(std::move(route));
                }
                if (routes.contains(route.current)) {
                    continue;
                }
                routes[route.current] = route;
                for (const auto &[previousHop, cost]: (*congestion)[route.current]) {
                    queue.push({previousHop, route.current, route.hops + 1, route.cost + cost});
                }
            }
            return results;
        }

        void NotifyInterfaceUp(uint32_t interface) override {
            if (linkChangeCallback) {
                linkChangeCallback();
            }
            Reset();
        }

        void NotifyInterfaceDown(uint32_t interface) override {
            if (linkChangeCallback) {
                linkChangeCallback();
            }
            Reset();
        }

        void NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) override {
            if (linkChangeCallback) {
                linkChangeCallback();
            }
            Reset();
        }

        void NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) override {
            if (linkChangeCallback) {
                linkChangeCallback();
            }
            Reset();
        }

        void SetIpv4(Ptr<Ipv4> ipv4) override {
            m_ipv4 = ipv4;
            if (linkChangeCallback) {
                linkChangeCallback();
            }
            Reset();
        }

        void PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const override {
            auto &s = *stream->GetStream();
            if (!flowRouting) {
                s << "Flow routing is disabled" << std::endl;
                return;
            }
            s << "Cached flows: " << std::endl;
            s << "Source\tDestination\tRoute" << std::endl;
            s << "Output flows: " << std::endl;
            for (auto &[flow, route]: outputFlows) {
                s << flow.src << "\t" << flow.dst << "\t" << route << std::endl;
            }
            s << "Input unicast flows: " << std::endl;
            for (auto &[flow, route]: inputUnicastFlows) {
                s << flow.src << "\t" << flow.dst << "\t" << route << std::endl;
            }
            s << "Input multicast flows: " << std::endl;
            for (auto &[flow, route]: inputMulticastFlows) {
                s << flow.src << "\t" << flow.dst << "\t" << route << std::endl;
            }
            s << "Input local flows: " << std::endl;
            for (auto &[flow, i]: inputLocalFlows) {
                s << flow.src << "\t" << flow.dst << "\t" << i << std::endl;
            }
        }

        ~Ipv4UGALRouting() override = default;

        void Reset() {
            outputFlows.clear();
            inputUnicastFlows.clear();
            inputMulticastFlows.clear();
            inputLocalFlows.clear();
        }
    };
}

#endif //NS3_IPV4_UGAL_ROUTING_H
