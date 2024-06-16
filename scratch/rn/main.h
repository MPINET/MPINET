#ifndef NS3_RN_SIMULATOR_MAIN_H
#define NS3_RN_SIMULATOR_MAIN_H

#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>

#include <unordered_map>

constinit static const bool RespondToLinkChanges = false;

using OCSLayerID = std::size_t;
using GroupID = std::size_t;
using SwitchID = std::size_t;
using ServerID = std::size_t;
using LinkID = std::size_t;
using NS3NodeID = uint32_t;
using NS3NetDevice = ns3::Ptr<ns3::NetDevice>;
using NS3Ipv4 = ns3::Ptr<ns3::Ipv4>;
using NS3Ipv4L3Protocol = ns3::Ptr<ns3::Ipv4L3Protocol>;

struct LinkInfo {
    NS3NetDevice device1;
    NS3NetDevice device2;
    NS3Ipv4 ipv41;
    NS3Ipv4 ipv42;
    ns3::Ipv4Address address1;
    ns3::Ipv4Address address2;
    NS3Ipv4L3Protocol ipv41l3protocol;
    NS3Ipv4L3Protocol ipv42l3protocol;

    bool operator==(const LinkInfo &) const = default;
};

struct PortInfo {
    NS3NetDevice device;
    ns3::Ipv4Address address;
};

template<>
struct std::hash<LinkInfo> {
    std::size_t operator()(const LinkInfo &linkInfo) const noexcept {
        return std::hash<NS3NetDevice>{}(linkInfo.device1) ^ std::hash<NS3NetDevice>{}(linkInfo.device2);
    }
};

using ServerMap = std::unordered_map<NS3NodeID, PortInfo>;
using BackgroundLinkMap = std::unordered_map<NS3NodeID, std::unordered_map<NS3NodeID, LinkInfo>>;
using ReconfigurableLinkMap = std::unordered_map<OCSLayerID, std::unordered_map<GroupID, std::unordered_map<GroupID, LinkInfo>>>;
using ReconfigurableLinkMapByServer = std::unordered_map<
    OCSLayerID,
    std::unordered_map<ServerID, std::unordered_map<ServerID, LinkInfo>>>; // 这里的OCSLayerID为逻辑OCSLayerID

using BlockedPairState = std::unordered_map<NS3NodeID, std::unordered_map<NS3NodeID, bool>>;
using Address2Onoffapplication = std::unordered_map<ns3::Ipv4Address, ns3::Ptr<ns3::OnOffApplication >>;
using LocalDrainCallback = std::function<void(NS3NodeID, ns3::Address)>;
using GroupLinkNumberMap = std::unordered_map<GroupID, std::unordered_map<GroupID, int>>;

#endif //NS3_RN_SIMULATOR_MAIN_H
