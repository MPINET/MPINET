#ifndef FATTREE_TOPOLOGY_H
#define FATTREE_TOPOLOGY_H

#include "../intergroup-cost.h"
#include "../main.h"
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

class FattreeTopology 
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
    // parameter to initialize topology
    std::size_t k = 4; // k-port switch

    std::size_t number_of_pod = k;
    std::size_t number_of_aggre_switches_per_pod = k / 2;
    std::size_t number_of_edge_switches_per_pod = k / 2;
    std::size_t number_of_servers_per_edge_switch = (k / 2);
    std::size_t number_of_servers_per_pod =
        number_of_servers_per_edge_switch * number_of_edge_switches_per_pod;
    std::size_t number_of_core_switches = (k / 2) * (k / 2);
    std::size_t number_of_aggre_switches = number_of_aggre_switches_per_pod * number_of_pod;
    std::size_t number_of_edge_switches = number_of_edge_switches_per_pod * number_of_pod;
    std::size_t number_of_servers = number_of_servers_per_pod * number_of_pod;

    // reconfigurable map between core and aggregation (exactly not change)
    ReconfigurableLinkMap top_reconfigurableLinkMap;

    // topology has switches, servers, and allNodes
    ns3::NodeContainer core_switches;
    ns3::NodeContainer aggre_switches;
    ns3::NodeContainer edge_switches;

    // map switch node id to aggre/edge/core switch index
    SwitchID2Index aggreSwitchID2Index;
    SwitchID2Index edgeSwitchID2Index;
    SwitchID2Index coreSwitchID2Index;

  public:
    // initialize topology use (parameter to initialize topology)
    FattreeTopology(std::size_t k,
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
                    bool only_reconfiguration);

    // void activate_one_level(int level);
    // void link_reconfiguration(std::vector<std::size_t>& to_choose_pair);
    // void Direct_change(std::vector<std::size_t>& to_choose_pair);
    // void incremental_change();
    // void alarm_local_drain(NS3NodeID srcID, Address dstadd);
    // void unblock_flows();
    // void Local_draining(std::vector<std::size_t>& to_choose_pair);
    // int block_flows(Ptr<Node> node, uint32_t ipv4ifIndex, Ptr<Node> peer);
    bool check_nodes_between_group(NS3NodeID nodeid1, NS3NodeID nodeid2);
    bool check_link_status(NS3NetDevice device1, NS3NetDevice device2);
    void inter_group_hop_trigger(Ptr<const NetDevice> device1, Ptr<const NetDevice> device2);

    void change_all_link_delay(string delay);
    void change_all_link_bandwidth(string bandwidth);
};

#endif // FATTREE_TOPOLOGY_H