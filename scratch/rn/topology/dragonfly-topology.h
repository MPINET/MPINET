#ifndef DRAGONFLY_TOPOLOGY_H
#define DRAGONFLY_TOPOLOGY_H

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

class DragonflyTopology
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
    std::size_t p = 2;
    std::size_t a = 4;
    std::size_t h = 2;
    std::size_t g = 0;
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
    // initialize topology use (parameter to initialize topology)
    DragonflyTopology(std::size_t p,
                      std::size_t a,
                      std::size_t h,
                      std::size_t g,
                      std::string bandwidth,
                      std::string delay,
                      std::size_t ocs,
                      bool ugal,
                      bool flowRouting,
                      double congestionMonitorPeriod,
                      bool enable_reconfiguration,
                      float reconfiguration_timestep,
                      float stop_time,
                      bool is_adversial,
                      bool ecmp,
                      std::string app_bd,
                      double bias,
                      int reconfiguration_count,
                      bool only_reconfiguration,
                      bool MUSE);

    void link_reconfiguration(std::vector<std::size_t>& to_choose_pair);
    void link_reconfiguration_for_MUSE(std::vector<std::size_t>& to_choose_pair);
    void Direct_change(std::vector<std::size_t>& to_choose_pair);
    void Direct_change_for_MUSE(std::vector<std::size_t>& to_choose_pair);
    void activate_one_level(OCSLayerID layer);
    void incremental_change();
    void incremental_change_for_MUSE();
    void alarm_local_drain(NS3NodeID srcID, Address dstadd);
    void unblock_flows();
    void Local_draining(std::vector<std::size_t>& to_choose_pair);
    void Local_draining_for_MUSE(std::vector<std::size_t>& to_choose_pair);
    int block_flows(Ptr<Node> node, uint32_t ipv4ifIndex, Ptr<Node> peer);
    bool check_nodes_between_group(NS3NodeID nodeid1, NS3NodeID nodeid2);
    bool check_link_status(NS3NetDevice device1, NS3NetDevice device2);
    void inter_group_hop_trigger(Ptr<const NetDevice> device1, Ptr<const NetDevice> device2);

    void change_all_link_delay(string delay);
    void change_all_link_bandwidth(string bandwidth);

  // for network
  protected:
    std::unordered_set<mip::Edge> reconfigurableEdges;
    std::unordered_map<std::string, std::unordered_set<mip::Edge>> conflictEdges;
    std::unordered_map<std::string, std::unordered_set<mip::Edge>> synchronousEdges;
    mip::TrafficPattern trafficPattern;

  public:
    mip::TrafficPattern getTrafficPattern(){return trafficPattern;}
    void initializeNetwork();

  protected:
    bool cmp_change_pair(const std::vector<std::size_t>& a, const std::vector<std::size_t>& b);
    void set_direct_change(Ptr<Ipv4> ipv4, bool value);
    void change_linkdevice_state(NS3NetDevice device1, NS3NetDevice device2, bool state);
};

#endif // DRAGONFLY_TOPOLOGY_H
