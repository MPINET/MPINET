#ifndef NS3_IPV4_UGAL_ROUTING_HELPER_H
#define NS3_IPV4_UGAL_ROUTING_HELPER_H

#include "ipv4-ugal-routing.h"

#include "ns3/applications-module.h"
#include "congestion-monitor-application.h"
#include <ns3/internet-module.h>

#include <memory>
#include <unordered_map>

namespace ns3 {
    // used to create Ipv4UGALRouting objects during installing InternetStack
    template<typename T=double>
    class Ipv4UGALRoutingHelper : public Ipv4RoutingHelper {
    private:
        using NodeID = uint32_t;
        using NS3NetDevice = Ptr<NetDevice>;
        using CongestionMap = std::unordered_map<NodeID, std::unordered_map<NodeID, T>>;
        using LinkMap = std::unordered_map<NodeID, std::unordered_map<NodeID, std::pair<Ipv4Address, Ipv4Address>>>;
        using AddressMap = std::unordered_map<Ipv4Address, NodeID>;

        bool usingAbsoluteQueueLength; // whether to use absolute queue length as the cost
        bool flowRouting; // whether to use flow routing
        bool ecmp; // whether to use ECMP
        bool respondToLinkChanges; // whether to respond to link changes
        T bias; // bias for the cost of choosing the path with minimum congestion
        std::function<T(NodeID, NodeID)> initialCongestion; // initial congestion function
        std::function<bool(NS3NetDevice, NS3NetDevice)> linkEnabled; // link enabled function

        std::shared_ptr<CongestionMap> congestion; // reversed congestion map: current node -> previous hop -> congestion from previous hop to current node
        std::shared_ptr<LinkMap> links; // link map: current node -> next hop -> the addresses connecting current node and next hop
        std::shared_ptr<AddressMap> addresses; // address map: address -> node id

    public:
        static double UnitInitialCongestion(NodeID, NodeID) {
            return 1.0;
        }

        Ipv4UGALRoutingHelper(bool usingAbsoluteQueueLength = true, bool flowRouting = false, bool ecmp = false, bool respondToLinkChanges = false, T bias = {}, std::function<T(NodeID, NodeID)> initialCongestion = UnitInitialCongestion, std::function<bool(NS3NetDevice, NS3NetDevice)> linkEnabled = isLinkEnabled) : usingAbsoluteQueueLength(usingAbsoluteQueueLength), flowRouting(flowRouting), ecmp(ecmp), respondToLinkChanges(respondToLinkChanges), bias(bias), initialCongestion(std::move(initialCongestion)), linkEnabled(std::move(linkEnabled)), congestion(std::make_shared<CongestionMap>()), links(std::make_shared<LinkMap>()), addresses(std::make_shared<AddressMap>()) {}

        Ipv4UGALRoutingHelper *Copy() const override {
            return new Ipv4UGALRoutingHelper(*this);
        }

        Ptr<Ipv4RoutingProtocol> Create(Ptr<Node> node) const override {
            std::function<void()> callback;
            if (respondToLinkChanges) {
                callback = std::bind(&Ipv4UGALRoutingHelper::NotifyLinkChanges, this);
            }
            auto routing = CreateObject<Ipv4UGALRouting<T>>(flowRouting, ecmp, bias, callback, congestion, links, addresses);
            return routing;
        }

        static bool isLinkEnabled(NS3NetDevice local, NS3NetDevice remote) {
            auto localIpv4= local->GetNode()->GetObject<Ipv4>();
            auto interface = localIpv4->GetInterfaceForDevice(local);
            auto remoteIpv4 = remote->GetNode()->GetObject<Ipv4>();
            auto remoteInterface = remoteIpv4->GetInterfaceForDevice(remote);
            return localIpv4->IsUp(interface) && localIpv4->IsForwarding(interface) && remoteIpv4->IsUp(remoteInterface) && remoteIpv4->IsForwarding(remoteInterface);
        }

        // must be called after all nodes are installed with InternetStack
        void NotifyLinkChanges() const {
            for (auto i = NodeList::Begin(); i < NodeList::End(); ++i) {
                auto localNode = *i;
                auto localNodeId = localNode->GetId();
                auto localIpv4 = localNode->GetObject<Ipv4>();
                for (uint32_t interface = 0; interface < localIpv4->GetNInterfaces(); ++interface) {
                    if (localIpv4->GetNAddresses(interface) <= 0) {
                        continue;
                    }
                    auto localAddress = localIpv4->GetAddress(interface, 0).GetLocal();
                    (*addresses)[localAddress] = localNodeId;
                    auto local = localIpv4->GetNetDevice(interface);
                    if (DynamicCast<LoopbackNetDevice>(local)) {
                        continue;
                    }
                    if (local->IsPointToPoint()) {
                        auto remote = RemoteDevice(local);
                        auto remoteNode = remote->GetNode();
                        auto remoteNodeId = remoteNode->GetId();
                        auto remoteIpv4 = remoteNode->GetObject<Ipv4>();
                        auto remoteInterface = remoteIpv4->GetInterfaceForDevice(remote);
                        if (remoteInterface < 0 || remoteIpv4->GetNAddresses(remoteInterface) <= 0) {
                            continue;
                        }
                        auto remoteAddress = remoteIpv4->GetAddress(remoteInterface, 0).GetLocal();
                        (*links)[localNodeId][remoteNodeId] = {localAddress, remoteAddress};
                        // remove congestion info for disabled links, add congestion info for new links
                        if (linkEnabled(local, remote)) {
                            if (!congestion->contains(localNodeId) || !congestion->at(localNodeId).contains(remoteNodeId)) {
                                (*congestion)[localNodeId][remoteNodeId] = initialCongestion(localNodeId, remoteNodeId);
                            }
                            if (!congestion->contains(remoteNodeId) || !congestion->at(remoteNodeId).contains(localNodeId)) {
                                (*congestion)[remoteNodeId][localNodeId] = initialCongestion(remoteNodeId, localNodeId);
                            }
                        } else {
                            if (congestion->contains(localNodeId) && congestion->at(localNodeId).contains(remoteNodeId)) {
                                congestion->at(localNodeId).erase(remoteNodeId);
                            }
                            if (congestion->contains(remoteNodeId) && congestion->at(remoteNodeId).contains(localNodeId)) {
                                congestion->at(remoteNodeId).erase(localNodeId);
                            }
                        }
                    } else if (local->IsMulticast() || local->IsBroadcast()) {
                    }
                }
            }
        }

        template<typename=void> requires std::is_convertible_v<T, double>
        ApplicationContainer InstallCongestionMonitorApplication(const NodeContainer &nodes, const Time &period) const {
            ApplicationContainer applications;
            auto callback = [c = this->congestion, a = this->addresses](Ipv4Address local, Ipv4Address remote, double occupancy) {
                auto localNodeId = (*a).at(local);
                auto remoteNodeId = (*a).at(remote);
                (*c)[remoteNodeId][localNodeId] = occupancy; // reversed congestion map
            };
            for (auto i = nodes.Begin(); i < nodes.End(); ++i) {
                auto node = *i;
                auto application = CreateObject<CongestionMonitorApplication>(usingAbsoluteQueueLength, period, callback);
                node->AddApplication(application);
                applications.Add(application);
            }
            return applications;
        }

        ~Ipv4UGALRoutingHelper() override = default;
    };
}

#endif //NS3_IPV4_UGAL_ROUTING_HELPER_H
