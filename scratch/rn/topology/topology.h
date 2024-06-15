#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "../intergroup-cost.h"
#include "../main.h"
#include "../mip-model/network.h"
#include "../routing/ipv4-ugal-routing-helper.h"
#include "../routing/route-monitor-helper.h"
#include "../utility.h"

#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/mpi-application-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>

#include <optional>

using namespace std;
using namespace ns3;

class Topology
{
  public:
    using NS3NetDevice = Ptr<NetDevice>;
    using LinkDeviceState =
        std::unordered_map<NS3NetDevice, std::unordered_map<NS3NetDevice, bool>>;
    using SwitchID2Index = std::unordered_map<NS3NodeID, std::size_t>;

    ServerMap serverMap;
    BackgroundLinkMap backgroundLinkMap;
    ReconfigurableLinkMap reconfigurableLinkMap;
    ReconfigurableLinkMapByServer reconfigurableLinkMapByServer;

    float start_time = 1.5;
    float stop_time = 1.5006;

    // parameter to initialize topology
    std::string bandwidth = "10Gbps";
    std::string delay = "1us";
    std::size_t ocs = 0;
    bool ugal = false;
    bool flowRouting = false;
    double congestionMonitorPeriod = 0.0001;
    bool enable_reconfiguration = false;
    float reconfiguration_timestep = 0.0002;
    bool is_adversial = false;
    bool ecmp = false;
    std::string app_bd = "10Gbps";
    double bias = 0.0;
    int reconfiguration_count = 100;
    double threshold = 0.00000001;
    bool only_reconfiguration = false;
    bool MUSE = false;

    uint64_t totalHops = 0;

    GroupLinkNumberMap groupLinkNumberMap;
    SwitchID2Index switchID2Index;
    LinkDeviceState linkDeviceState;

    std::vector<std::vector<float>> traffic_matrix;
    std::vector<std::vector<int>> topology_matrix;
    std::vector<std::vector<float>> AVG;
    std::vector<std::vector<GroupID>> hottest_pair;
    std::vector<std::vector<std::size_t>>
        change_pair; //  std::size_t : group index ; are candidate choosed_pairs
    std::vector<std::size_t> choosed_pair;

    std::vector<std::vector<uint32_t>>
        neighbor_end_pair; // [0]: reciever index(not ID) [1]: sender index(not ID)
    std::vector<std::vector<uint32_t>>
        adverse_end_pair; // [0]: reciever index(not ID) [1]: sender index(not ID)
    std::vector<std::vector<uint32_t>>
        traffic_pattern_end_pair; // [0]: reciever index(not ID) [1]: sender index(not ID)

    BlockedPairState blockedPairState; // for local draining [srcID][dstID] = whether drained

    Address2Onoffapplication address2Onoffapplication;

    ns3::NodeContainer switches;
    ns3::NodeContainer servers;
    ns3::NodeContainer allNodes;

    ns3::InternetStackHelper internetStackHelper;
    optional<ns3::Ipv4UGALRoutingHelper<>> ugalRoutingHelper;
    optional<ns3::Ipv4UGALRoutingHelper<IntergroupCost>> ugalRoutingHelperForECMPUgal;
    ns3::Ipv4GlobalRoutingHelper globalRoutingHelper;
    ns3::ApplicationContainer congestionMonitorApplications;
    optional<ns3::RouteMonitorHelper<>> routeMonitorHelper;

  public:
    // for network
  protected:
    std::unordered_set<mip::Edge> reconfigurableEdges;
    std::unordered_map<std::string, std::unordered_set<mip::Edge>> conflictEdges;
    std::unordered_map<std::string, std::unordered_set<mip::Edge>> synchronousEdges;
    mip::TrafficPattern trafficPattern;
};

#endif // TOPOLOGY_H
