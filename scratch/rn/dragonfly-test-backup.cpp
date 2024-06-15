//
// Created by Ricardo Evans on 2023/10/4.
//

#include "routing/ipv4-ugal-routing-helper.h"
#include "utility.h"

#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor.h"
#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/mpi-application-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>

using namespace std;
using namespace ns3;

const constinit static bool RespondToLinkChanges = false;

using OCSLayerID = std::size_t;
using GroupID = std::size_t;
using SwitchID = std::size_t;
using ServerID = std::size_t;
using LinkID = std::size_t;
using NS3NodeID = uint32_t;
using NS3NetDevice = ns3::Ptr<ns3::NetDevice>;
using NS3Ipv4 = ns3::Ptr<ns3::Ipv4>;
using NS3Ipv4L3Protocol = ns3::Ptr<ns3::Ipv4L3Protocol>;

struct LinkInfo
{
    NS3NetDevice device1;
    NS3NetDevice device2;
    NS3Ipv4 ipv41;
    NS3Ipv4 ipv42;
    ns3::Ipv4Address address1;
    ns3::Ipv4Address address2;
    NS3Ipv4L3Protocol ipv41l3protocol;
    NS3Ipv4L3Protocol ipv42l3protocol;

    bool operator==(const LinkInfo&) const = default;
};

struct PortInfo
{
    NS3NetDevice device;
    ns3::Ipv4Address address;
};

template <>
struct std::hash<LinkInfo>
{
    std::size_t operator()(const LinkInfo& linkInfo) const noexcept
    {
        return std::hash<NS3NetDevice>{}(linkInfo.device1) ^
               std::hash<NS3NetDevice>{}(linkInfo.device2);
    }
};

using ServerMap = std::unordered_map<NS3NodeID, PortInfo>;
using BackgroundLinkMap = std::unordered_map<NS3NodeID, std::unordered_map<NS3NodeID, LinkInfo>>;
using ReconfigurableLinkMap =
    std::unordered_map<OCSLayerID,
                       std::unordered_map<GroupID, std::unordered_map<GroupID, LinkInfo>>>;

ServerMap serverMap;
BackgroundLinkMap backgroundLinkMap;
ReconfigurableLinkMap reconfigurableLinkMap;

std::string tracePath = "";

float start_time = 1.5;
float stop_time = 1.5006;

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
long long maxBytes = 20000000;

double threshold = 0.000000005;

using GroupLinkNumberMap = std::unordered_map<GroupID, std::unordered_map<GroupID, int>>;
GroupLinkNumberMap groupLinkNumberMap;

std::vector<std::vector<float>> traffic_matrix;
std::vector<std::vector<int>> topology_matrix;
std::vector<std::vector<float>> AVG;
std::vector<std::vector<GroupID>> hottest_pair;
std::vector<std::vector<std::size_t>> change_pair;
std::vector<std::size_t> choosed_pair;
std::vector<std::vector<uint32_t>> neighbor_end_pair;
std::vector<std::vector<uint32_t>> adverse_end_pair;
std::vector<std::vector<uint32_t>> traffic_pattern_end_pair; // [0]: reciever [1]: sender

using BlockedPairState = std::unordered_map<NS3NodeID, std::unordered_map<NS3NodeID, bool>>;
BlockedPairState blockedPairState;

using Address2Onoffapplication = std::unordered_map<ns3::Ipv4Address, Ptr<OnOffApplication>>;
Address2Onoffapplication address2Onoffapplication;

using LocalDrainCallback = std::function<void(NS3NodeID, Address)>;

void
link_reconfiguration(ns3::Ipv4UGALRoutingHelper& ugalRoutingHelper)
{
    vector<GroupID> groupLinkDown;
    vector<GroupID> groupLinkUp;
    // find add link
    auto tor1Ipv4 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].ipv41;
    auto device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].device1;
    auto tor2Ipv4 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].ipv42;
    auto device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].device2;
    tor1Ipv4->SetUp(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetUp(tor2Ipv4->GetInterfaceForDevice(device2));
    tor1Ipv4 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].device1;
    tor2Ipv4 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].device2;
    tor1Ipv4->SetUp(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetUp(tor2Ipv4->GetInterfaceForDevice(device2));

    groupLinkUp.push_back(choosed_pair[0]);
    groupLinkUp.push_back(choosed_pair[1]);
    groupLinkUp.push_back(choosed_pair[2]);
    groupLinkUp.push_back(choosed_pair[3]);

    // find delete link
    auto min_g0_g2 = min(choosed_pair[0], choosed_pair[2]);
    auto max_g0_g2 = max(choosed_pair[0], choosed_pair[2]);
    tor1Ipv4 = reconfigurableLinkMap[choosed_pair[4]][min_g0_g2][max_g0_g2].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][min_g0_g2][max_g0_g2].device1;
    tor2Ipv4 = reconfigurableLinkMap[choosed_pair[4]][min_g0_g2][max_g0_g2].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][min_g0_g2][max_g0_g2].device2;
    tor1Ipv4->SetDown(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetDown(tor2Ipv4->GetInterfaceForDevice(device2));

    std::cout << "down node11: " << device1->GetNode()->GetId() << std::endl;
    std::cout << "down node12: " << device2->GetNode()->GetId() << std::endl;

    auto min_g1_g3 = min(choosed_pair[1], choosed_pair[3]);
    auto max_g1_g3 = max(choosed_pair[1], choosed_pair[3]);
    tor1Ipv4 = reconfigurableLinkMap[choosed_pair[4]][min_g1_g3][max_g1_g3].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][min_g1_g3][max_g1_g3].device1;
    tor2Ipv4 = reconfigurableLinkMap[choosed_pair[4]][min_g1_g3][max_g1_g3].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][min_g1_g3][max_g1_g3].device2;
    tor1Ipv4->SetDown(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetDown(tor2Ipv4->GetInterfaceForDevice(device2));

    std::cout << "down node21: " << device1->GetNode()->GetId() << std::endl;
    std::cout << "down node22: " << device2->GetNode()->GetId() << std::endl;

    groupLinkDown.push_back(choosed_pair[0]);
    groupLinkDown.push_back(choosed_pair[2]);
    groupLinkDown.push_back(choosed_pair[1]);
    groupLinkDown.push_back(choosed_pair[3]);

    std::cout << "groupLinkUp: " << groupLinkUp[0] << " " << groupLinkUp[1] << " " << groupLinkUp[2]
              << " " << groupLinkUp[3] << endl;
    std::cout << "groupLinkDown: " << groupLinkDown[0] << " " << groupLinkDown[1] << " "
              << groupLinkDown[2] << " " << groupLinkDown[3] << endl;
    groupLinkNumberMap[groupLinkUp[0]][groupLinkUp[1]]++;
    groupLinkNumberMap[groupLinkUp[2]][groupLinkUp[3]]++;
    groupLinkNumberMap[groupLinkUp[1]][groupLinkUp[0]]++;
    groupLinkNumberMap[groupLinkUp[3]][groupLinkUp[2]]++;
    groupLinkNumberMap[groupLinkDown[0]][groupLinkDown[1]]--;
    groupLinkNumberMap[groupLinkDown[2]][groupLinkDown[3]]--;
    groupLinkNumberMap[groupLinkDown[1]][groupLinkDown[0]]--;
    groupLinkNumberMap[groupLinkDown[3]][groupLinkDown[2]]--;

    Ipv4GlobalRoutingHelper::RecomputeRoutingTables();

    if (ugal)
    {
        ugalRoutingHelper.NotifyLinkChanges();
    }
}

static void
set_direct_change(Ptr<Ipv4> ipv4, bool value)
{
    ipv4->local_drain(value);
}

static void
Direct_change(ns3::Ipv4UGALRoutingHelper& ugalRoutingHelper)
{
    std::cout << "Direct_change " << change_pair.size() << std::endl;
    // delete link[3][1]
    auto Ipv41 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[3]][choosed_pair[1]].ipv41;
    auto device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[3]][choosed_pair[1]].device1;
    auto ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    set_direct_change(Ipv41, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), set_direct_change, Ipv41, false);
    Ipv41->SetDown(ipv4ifIndex1);

    auto Ipv42 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[3]][choosed_pair[1]].ipv42;
    auto device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[3]][choosed_pair[1]].device2;
    auto ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    set_direct_change(Ipv42, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), set_direct_change, Ipv42, false);
    Ipv42->SetDown(ipv4ifIndex2);

    // delete link[0][2]
    Ipv41 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].device1;
    ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    set_direct_change(Ipv41, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), set_direct_change, Ipv41, false);
    Ipv41->SetDown(ipv4ifIndex1);

    Ipv42 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].device2;
    ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    set_direct_change(Ipv42, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), set_direct_change, Ipv42, false);
    Ipv42->SetDown(ipv4ifIndex2);

    // add link[0][1]
    Ipv41 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].device1;
    ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    Ipv41->SetUp(ipv4ifIndex1);
    // Simulator::Schedule (Seconds (threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv41,
    // ipv4ifIndex1);

    Ipv42 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].device2;
    ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    Ipv42->SetUp(ipv4ifIndex2);
    // Simulator::Schedule (Seconds (threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv42,
    // ipv4ifIndex2);

    // add link[2][3]
    Ipv41 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].device1;
    ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    Ipv41->SetUp(ipv4ifIndex1);
    // Simulator::Schedule (Seconds (threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv41,
    // ipv4ifIndex1);

    Ipv42 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].device2;
    ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    Ipv42->SetUp(ipv4ifIndex2);
    // Simulator::Schedule (Seconds (threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv42,
    // ipv4ifIndex2);

    Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &Ipv4GlobalRoutingHelper::RecomputeRoutingTables);

    if (ugal)
    {
        ugalRoutingHelper.NotifyLinkChanges();
        Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                            &ns3::Ipv4UGALRoutingHelper::NotifyLinkChanges,
                            &ugalRoutingHelper);
    }
}

// //local draining
// void alarm_local_drain(NS3NodeID srcID, Address dstadd);
// static int
// block_flows(Ptr<Ipv4L3Protocol> ipv4l3, uint32_t ipv4ifIndex)
// {
//     //get all src-dst pair from interface
//     Ptr<Ipv4Interface> interface = ipv4l3->GetInterface(ipv4ifIndex);
//     auto nflow = interface->packet_src_record.size();
//     std::cout << "here!" << interface->packet_src_record_repetitive.size() << std::endl;

//     if(interface->packet_src_record_repetitive.size() != 0){
//         // find src-dst pair to block
//         for(auto& pair : interface->packet_src_record){
//             auto srcAddress = pair[0];
//             auto dstAddress = pair[1];
//             std::cout << "srcAddress: " << srcAddress << " dstAddress: " << dstAddress <<
//             std::endl; NS3NodeID srcID = 0; NS3NodeID dstID = 0;

//             for(auto& nodeID2Port : serverMap){
//                 if(nodeID2Port.second.address == srcAddress){
//                     srcID = nodeID2Port.first;
//                 }
//                 if(nodeID2Port.second.address == dstAddress){
//                     dstID = nodeID2Port.first;
//                 }
//             }
//             std::cout << "srcID: " << srcID << " dstID: " << dstID << std::endl;

//             blockedPairState[srcID][dstID] = false;

//             auto sendApp = address2Onoffapplication[srcAddress];
//             auto socket = sendApp->GetSocket();
//             socket->block(alarm_local_drain);
//             // std::cout << socket->GetNode()->GetId() << "blocked! " << socket->m_local_blocked
//             << std::endl;
//         }
//     }
//     return nflow;
// }

// static void
// unblock_flows()
// {
//     // find src-dst pair to unblock
//     for(auto& srcDstPair : blockedPairState){
//         auto srcID = srcDstPair.first;
//         auto srcAddress = serverMap[srcID].address;

//         auto sendApp = address2Onoffapplication[srcAddress];
//         auto socket = sendApp->GetSocket();
//         socket->unblock();
//     }
// }

// static void
// Local_draining(std::vector<std::size_t>& to_choose_pair, ns3::Ipv4UGALRoutingHelper&
// ugalRoutingHelper)
// {
//     // debug info
//     std::cout << "local draining " << to_choose_pair.size() << std::endl;

//     // ensure previous local draining is done
//     for(auto& kv1 : blockedPairState){
//         for(auto& kv2 : kv1.second){
//             if(kv2.second == false){
//                 std::cout << "ERROR: Previous local draining is not done." << std::endl;
//                 return;
//             }
//         }
//     }

//     // clear  blockedPairState
//     blockedPairState.clear();
//     choosed_pair = to_choose_pair;

//     // build new blockedPairState
//     // // to-delete link1
//     auto Ipv41 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].ipv41;
//     auto device1 =
//     reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].device1; auto
//     ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1); auto ipv41L3Protocol =
//     reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].ipv41l3protocol;
//     auto nflow_1 = block_flows(ipv41L3Protocol, ipv4ifIndex1);

//     // // to-delete link1
//     auto Ipv42 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[1]][choosed_pair[3]].ipv41;
//     auto device2 =
//     reconfigurableLinkMap[choosed_pair[4]][choosed_pair[1]][choosed_pair[3]].device1; auto
//     ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2); auto ipv42L3Protocol =
//     reconfigurableLinkMap[choosed_pair[4]][choosed_pair[1]][choosed_pair[3]].ipv41l3protocol;
//     auto nflow_2 = block_flows(ipv42L3Protocol, ipv4ifIndex2);

//     if(nflow_1 == 0 && nflow_2 == 0){
//         link_reconfiguration(ugalRoutingHelper);
//     }
// }

// void
// alarm_local_drain(NS3NodeID srcID, Address dstadd)
// {
//     cout << "alarm_local_drain: " << srcID << " " << dstadd << endl;
//     // NS3NodeID dstID = 0;
//     // for(auto& node : serverMap){
//     //     if(node.second.address == dstadd){
//     //         dstID = node.first;
//     //     }
//     // }

//     // check if all pairs local drained
//     bool allLocalDrained = true;
//     for (auto& outerPair : blockedPairState) {
//         if(outerPair.first == srcID){
//             for(auto& innerPair : outerPair.second){
//                 innerPair.second = true;
//             }
//             continue;
//         }
//         for (auto& innerPair : outerPair.second) {
//             if(innerPair.second == false){
//                 allLocalDrained = false;
//                 break;
//             }
//         }
//     }

//     std::cout << "allLocalDrained: " << allLocalDrained << endl;
//     if(allLocalDrained == true)
//     {
//         ns3::Ipv4UGALRoutingHelper ugalRoutingHelper; //?
//         link_reconfiguration(ugalRoutingHelper);
//         unblock_flows();
//         blockedPairState.clear();
//     }
// }

// acticate one level
void
acticate_one_level(OCSLayerID layer)
{
    // std::cout << "acticate_one_level" << endl;
    vector<bool> choose_group(g, 0);
    GroupID choose_g_1 = 0;
    GroupID choose_g_2 = 0;
    vector<GroupID> count_choose;
    for (GroupID i = 0; i < g; i++)
    {
        count_choose.push_back(i);
    }
    // int count = 0;
    while (count_choose.size() > 1)
    {
        GroupID index1 = rand() % count_choose.size();
        choose_g_1 = count_choose[index1];
        // std::cout << "choose_g_1 :" << choose_g_1 << endl;
        GroupID index2 = rand() % count_choose.size();
        choose_g_2 = count_choose[index2];
        // std::cout << "choose_g_2 :" << choose_g_2 << endl;
        if (choose_g_2 == choose_g_1)
            continue;
        if (choose_g_1 > choose_g_2)
            swap(choose_g_1, choose_g_2);

        auto ipv41 = reconfigurableLinkMap[layer][choose_g_1][choose_g_2].ipv41;
        auto device1 = reconfigurableLinkMap[layer][choose_g_1][choose_g_2].device1;
        auto ipv41ifIndex = ipv41->GetInterfaceForDevice(device1);
        ipv41->SetUp(ipv41ifIndex);

        auto ipv42 = reconfigurableLinkMap[layer][choose_g_1][choose_g_2].ipv42;
        auto device2 = reconfigurableLinkMap[layer][choose_g_1][choose_g_2].device2;
        auto ipv42ifIndex = ipv42->GetInterfaceForDevice(device2);
        ipv42->SetUp(ipv42ifIndex);

        groupLinkNumberMap[choose_g_1][choose_g_2]++;
        groupLinkNumberMap[choose_g_2][choose_g_1]++;

        if (index1 < index2)
        {
            int tmp = index1;
            index1 = index2;
            index2 = tmp;
        }
        // std::cout << "erase :" << count_choose[index1] << endl;
        count_choose.erase(count_choose.begin() + index1);
        // std::cout << "erase :" << count_choose[index2] << endl;
        count_choose.erase(count_choose.begin() + index2);
    }
}

static bool
cmp_change_pair(vector<std::size_t>& a, vector<std::size_t>& b)
{
    return AVG[a[2]][a[3]] + AVG[a[3]][a[2]] + AVG[a[0]][a[1]] + AVG[a[1]][a[0]] -
               (AVG[a[0]][a[2]] + AVG[a[2]][a[0]]) - (AVG[a[1]][a[3]] + AVG[a[3]][a[1]]) >
           AVG[b[2]][b[3]] + AVG[b[3]][b[2]] + AVG[b[0]][b[1]] + AVG[b[1]][b[0]] -
               (AVG[b[0]][b[2]] + AVG[b[2]][b[0]]) - (AVG[b[1]][b[3]] + AVG[b[3]][b[1]]);
}

// incremental_change
static void
incremental_change(ns3::Ipv4UGALRoutingHelper& ugalRoutingHelper)
{
    std::cout << "incremental_change" << endl;
    traffic_matrix = vector<vector<float>>(g, vector<float>(g, 0.0));
    topology_matrix = vector<vector<int>>(g, vector<int>(g, 0));
    AVG = vector<vector<float>>(g, vector<float>(g, 0.0));
    float target = 0;
    hottest_pair = {};
    change_pair = {};
    // get traffic matirx : from ip interface
    //  for(int i = 0; i < number_of_end; i++){
    //      Ptr<Node> n = nodes.Get(i / p);
    //      uint32_t ipv4ifIndex = i % p + 1; //?
    //      Ptr<Ipv4L3Protocol> ipv4l3 = n->GetObject<Ipv4L3Protocol> ();
    //      std::cout << "index " << i / p << " "<< i % p + 1 << endl;
    //      Ptr<Ipv4Interface> interface = ipv4l3->GetInterface(ipv4ifIndex);
    //      std::cout << i << " " << ipv4ifIndex <<" " << interface->GetAddress(ipv4ifIndex) <<
    //      endl; for(uint32_t i = 0; i < interface->packet_src_record.size(); i ++){
    //          uint32_t index = interface->packet_src_record[i].first.Get();
    //          index = (index & 0b1111111111111111) >> 8;
    //          if(index >= 0 && index < uint32_t(number_of_end))
    //              traffic_matrix[index][i] += 1.0;
    //      }
    //  }

    // get traffic matirx : from setting
    for (vector<uint32_t>& tp : traffic_pattern_end_pair)
    {
        traffic_matrix[tp[0] / (a * p)][tp[1] / (a * p)] += 1.0;
        traffic_matrix[tp[1] / (a * p)][tp[0] / (a * p)] += 1.0;
    }

    // get topology matrix
    for (GroupID i = 0; i < g; i++)
    {
        for (GroupID j = 0; j < g; j++)
        {
            topology_matrix[i][j] = groupLinkNumberMap[i][j];
            topology_matrix[j][i] = groupLinkNumberMap[j][i];
        }
    }

    // get AVG
    for (GroupID i = 0; i < g; i++)
    {
        for (GroupID j = 0; j < g; j++)
        {
            if (i == j)
            {
                continue;
            }
            if (topology_matrix[i][j] != 0)
            {
                AVG[i][j] = (float(traffic_matrix[i][j])) / (float(topology_matrix[i][j]));
            }
            else
            {
                std::cout << "no link?" << endl;
                AVG[i][j] = (float(traffic_matrix[i][j]));
            }
        }
    }

    // find hottest and target
    for (GroupID i = 0; i < g; i++)
    {
        for (GroupID j = 0; j < g; j++)
        {
            if (i == j)
                continue;
            if (AVG[i][j] + AVG[j][i] > target)
            {
                hottest_pair.clear();
                hottest_pair.push_back({i, j});
                target = AVG[i][j] + AVG[j][i];
            }
            else if (AVG[i][j] + AVG[j][i] == target)
            {
                hottest_pair.push_back({i, j});
            }
        }
    }
    int hottest = target;
    // std::cout << "hottest size: " << hottest_pair.size() << " " << hottest_pair[0][0] << " " <<
    // hottest_pair[0][1] << endl;

    // find targetreturn
    target = 0;
    for (uint32_t i = 0; i < hottest_pair.size(); i++)
    {
        for (OCSLayerID layer = 0; layer < ocs; layer++)
        {
            for (GroupID k = 0; k < g; k++)
            {
                for (GroupID l = k + 1; l < g; l++)
                {
                    auto g_index1 = min(hottest_pair[i][0], hottest_pair[i][1]);
                    auto g_index2 = max(hottest_pair[i][0], hottest_pair[i][1]);
                    auto g_index3 = min(k, l);
                    auto g_index4 = max(k, l);
                    // std::cout << g_index1 << " " << g_index2 << " " << g_index3 << " " <<
                    // g_index4 << endl;
                    if ((g_index1 == g_index3 || g_index2 == g_index4) ||
                        (g_index1 == g_index4 || g_index2 == g_index3))
                        continue;
                    if (hottest + AVG[g_index3][g_index4] + AVG[g_index4][g_index3] -
                            (AVG[g_index1][g_index3] + AVG[g_index3][g_index1]) -
                            (AVG[g_index2][g_index4] + AVG[g_index4][g_index2]) >
                        0)
                    {
                        auto ipv41 = reconfigurableLinkMap[layer][g_index1][g_index2].ipv41;
                        auto device1 = reconfigurableLinkMap[layer][g_index1][g_index2].device1;
                        auto ipv41ifIndex = ipv41->GetInterfaceForDevice(device1);
                        auto ipv42 = reconfigurableLinkMap[layer][g_index3][g_index4].ipv41;
                        auto device2 = reconfigurableLinkMap[layer][g_index3][g_index4].device1;
                        auto ipv42ifIndex = ipv42->GetInterfaceForDevice(device2);
                        auto ipv43 = reconfigurableLinkMap[layer][min(g_index1, g_index3)]
                                                          [max(g_index1, g_index3)]
                                                              .ipv41;
                        auto device3 = reconfigurableLinkMap[layer][min(g_index1, g_index3)]
                                                            [max(g_index1, g_index3)]
                                                                .device1;
                        auto ipv43ifIndex = ipv43->GetInterfaceForDevice(device3);
                        auto ipv44 = reconfigurableLinkMap[layer][min(g_index2, g_index4)]
                                                          [max(g_index2, g_index4)]
                                                              .ipv41;
                        auto device4 = reconfigurableLinkMap[layer][min(g_index2, g_index4)]
                                                            [max(g_index2, g_index4)]
                                                                .device1;
                        auto ipv44ifIndex = ipv44->GetInterfaceForDevice(device4);

                        if (!ipv41->IsUp(ipv41ifIndex) && !ipv42->IsUp(ipv42ifIndex) &&
                            ipv43->IsUp(ipv43ifIndex) && ipv44->IsUp(ipv44ifIndex))
                        {
                            change_pair.push_back({g_index1, g_index2, g_index3, g_index4, layer});
                            target = hottest + AVG[g_index3][g_index4] + AVG[g_index4][g_index3] -
                                     (AVG[g_index1][g_index3] + AVG[g_index3][g_index1]) -
                                     (AVG[g_index2][g_index4] + AVG[g_index4][g_index2]);
                        }
                    }
                    // std::cout << hottest_pair[i][0] << " " <<  hottest_pair[i][1] << " " << k <<
                    // " " << l << " " << hottest + AVG[k][l] + AVG[l][k] -
                    // (AVG[hottest_pair[i][0]][k] + AVG[k][hottest_pair[i][0]]) -
                    // (AVG[hottest_pair[i][1]][l] + AVG[l][hottest_pair[i][1]]) << endl;
                    if (hottest + AVG[g_index3][g_index4] + AVG[g_index4][g_index3] -
                            (AVG[g_index1][g_index4] + AVG[g_index4][g_index1]) -
                            (AVG[g_index2][g_index3] + AVG[g_index3][g_index2]) >
                        0)
                    {
                        auto ipv41 = reconfigurableLinkMap[layer][g_index1][g_index2].ipv41;
                        auto device1 = reconfigurableLinkMap[layer][g_index1][g_index2].device1;
                        auto ipv41ifIndex = ipv41->GetInterfaceForDevice(device1);
                        auto ipv42 = reconfigurableLinkMap[layer][g_index3][g_index4].ipv41;
                        auto device2 = reconfigurableLinkMap[layer][g_index3][g_index4].device1;
                        auto ipv42ifIndex = ipv42->GetInterfaceForDevice(device2);
                        auto ipv43 = reconfigurableLinkMap[layer][min(g_index1, g_index4)]
                                                          [max(g_index1, g_index4)]
                                                              .ipv41;
                        auto device3 = reconfigurableLinkMap[layer][min(g_index1, g_index4)]
                                                            [max(g_index1, g_index4)]
                                                                .device1;
                        auto ipv43ifIndex = ipv43->GetInterfaceForDevice(device3);
                        auto ipv44 = reconfigurableLinkMap[layer][min(g_index2, g_index3)]
                                                          [max(g_index2, g_index3)]
                                                              .ipv41;
                        auto device4 = reconfigurableLinkMap[layer][min(g_index2, g_index3)]
                                                            [max(g_index2, g_index3)]
                                                                .device1;
                        auto ipv44ifIndex = ipv44->GetInterfaceForDevice(device4);

                        if (!ipv41->IsUp(ipv41ifIndex) && !ipv42->IsUp(ipv42ifIndex) &&
                            ipv43->IsUp(ipv43ifIndex) && ipv44->IsUp(ipv44ifIndex))
                        {
                            target = hottest + AVG[g_index3][g_index4] + AVG[g_index4][g_index3] -
                                     (AVG[g_index1][g_index4] + AVG[g_index4][g_index1]) -
                                     (AVG[g_index2][g_index3] + AVG[g_index3][g_index2]);
                            change_pair.push_back({g_index1, g_index2, g_index4, g_index3, layer});
                        }
                    }
                    // std::cout << hottest_pair[i][0] << " " <<  hottest_pair[i][1] << " " << k <<
                    // " " << l << " " << hottest + AVG[k][l] + AVG[l][k] -
                    // (AVG[hottest_pair[i][1]][k] + AVG[k][hottest_pair[i][1]]) -
                    // (AVG[hottest_pair[i][0]][l] + AVG[l][hottest_pair[i][0]]) << endl;
                }
            }
        }
    }

    sort(change_pair.begin(), change_pair.end(), cmp_change_pair);
    // std::cout << "change: !!" <<endl;
    // for(uint32_t i = 0; i < change_pair.size();i ++){
    //     auto& t = change_pair[i];
    //     std::cout << "change: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << " " <<
    //     AVG[t[0]][t[1]] + AVG[t[1]][t[0]] - (AVG[t[0]][t[2]] + AVG[t[2]][t[0]]) -
    //     (AVG[t[1]][t[3]] + AVG[t[3]][t[1]]) << endl;
    // }

    int num = change_pair.size();
    for (int choose_i = 0; choose_i < num; choose_i++)
    {
        choosed_pair = change_pair[choose_i];
        std::cout << "choose: " << choosed_pair[0] << " " << choosed_pair[1] << " "
                  << choosed_pair[2] << " " << choosed_pair[3] << endl;
        Direct_change(ugalRoutingHelper);
        // link_reconfiguration(ugalRoutingHelper);
        // Local_draining(change_pair[choose_i], ugalRoutingHelper);

        break;
    }

    // std::cout << "end increment" << endl;
}

// local_draining

int
main(int argc, char** argv)
{
    // ns3::LogComponentEnable("Ipv4UGALRouting", ns3::LOG_LEVEL_ALL);
    // ns3::LogComponentEnable("CongestionMonitorApplication", ns3::LOG_LEVEL_ALL);
    // ns3::LogComponentEnable("Ipv4UGALRoutingHelper", ns3::LOG_LEVEL_ALL);
    // ns3::LogComponentEnableAll(ns3::LOG_LEVEL_WARN);
    // ns3::LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
    ns3::LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    // set default ttl
    ns3::Config::SetDefault("ns3::Ipv4L3Protocol::DefaultTtl", StringValue("128"));

    // set dctcp (to reset red attribute)
    ns3::Config::SetDefault("ns3::TcpSocketBase::UseEcn", StringValue("On"));
    ns3::Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpDctcp::GetTypeId()));
    ns3::Config::SetDefault("ns3::RedQueueDisc::UseEcn", BooleanValue(true));
    ns3::Config::SetDefault("ns3::RedQueueDisc::QW", DoubleValue(1.0));
    ns3::Config::SetDefault("ns3::RedQueueDisc::MinTh", DoubleValue(16));
    ns3::Config::SetDefault("ns3::RedQueueDisc::MaxTh", DoubleValue(16));

    ns3::CommandLine cmd;
    cmd.Usage("simulations of HPC applications in reconfigurable network");
    cmd.AddNonOption("trace-path", "the path containing binary dumpi traces", tracePath);
    cmd.AddValue("p",
                 "the dragonfly parameter p, represents the number of servers per TOR switch",
                 p);
    cmd.AddValue("a",
                 "the dragonfly parameter a, represents the number of TOR switches per group ",
                 a);
    cmd.AddValue(
        "h",
        "the dragonfly parameter h, represents the count of inter-group links per TOR switch",
        h);
    cmd.AddValue("g",
                 "the dragonfly parameter g, represents the number of groups, setting to zero "
                 "means using balanced dragonfly (g = a * h + 1)",
                 g);
    cmd.AddValue("bandwidth", "the bandwidth of the links in the topology", bandwidth);
    cmd.AddValue("delay", "the delay of the links in the topology", delay);
    cmd.AddValue("ocs",
                 "the ocs count used in the reconfigurable topology, each ocs is connected to a "
                 "TOR in every group",
                 ocs);
    cmd.AddValue("ugal", "whether to use ugal routing", ugal);
    cmd.AddValue("flow-routing", "whether to use flow routing", flowRouting);
    cmd.AddValue("congestion-monitor-period",
                 "the period of congestion monitor in seconds",
                 congestionMonitorPeriod);
    cmd.AddValue("enable_reconfig", "whether to enable reconfiguration", enable_reconfiguration);
    cmd.AddValue("reconfig_time",
                 "the period of reconfiguration in seconds",
                 reconfiguration_timestep);
    cmd.AddValue("stop_time", "the time flow stop to generate", stop_time);
    cmd.AddValue("is_ad", "traffic pattern is adversial", is_adversial);
    cmd.AddValue("ecmp", "enable ecmp", ecmp);
    cmd.AddValue("app_bd", "app input speed", app_bd);
    cmd.AddValue("bias", "bias for ugal", bias);
    cmd.AddValue("reconfiguration_count", "the count of reconfiguration", reconfiguration_count);
    cmd.Parse(argc, argv);

    if (ecmp)
    {
        Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",
                           BooleanValue(true)); // 启用多路径路由
    }

    // the rate and packet size of sending
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(1024));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(app_bd));

    if (g <= 0)
    { // using balanced dragonfly according to the definitions of options
        g = a * h + 1;
    }
    NS_ASSERT_MSG((a * h) % (g - 1) == 0, "invalid group count");
    auto switchCount = g * a;
    auto serverCount = switchCount * p;
    auto backgroundLinkCount =
        switchCount * (a + h - 1) +
        serverCount *
            2; // server-tor links + links in every group + links between groups (simplex links)
    auto reconfigurableLinkCount = ocs * g * (g - 1); // ocs links between groups (simplex links)

    ns3::Time::SetResolution(ns3::Time::NS);

    ns3::NodeContainer switches;
    ns3::NodeContainer servers;
    ns3::NodeContainer allNodes;

    switches.Create(switchCount);
    servers.Create(serverCount);
    allNodes.Add(switches);
    allNodes.Add(servers);

    ns3::InternetStackHelper internetStackHelper;
    ns3::Ipv4UGALRoutingHelper ugalRoutingHelper(true, flowRouting, ecmp, RespondToLinkChanges, bias);
    ns3::Ipv4GlobalRoutingHelper globalRoutingHelper;
    ns3::ApplicationContainer congestionMonitorApplications;

    if (ugal)
    {
        internetStackHelper.SetRoutingHelper(ugalRoutingHelper);
        congestionMonitorApplications = ugalRoutingHelper.InstallCongestionMonitorApplication(
            allNodes,
            ns3::Seconds(congestionMonitorPeriod));
    }
    else
    {
        internetStackHelper.SetRoutingHelper(globalRoutingHelper);
    }
    internetStackHelper.Install(allNodes);

    ns3::PointToPointHelper p2pHelper;
    p2pHelper.SetDeviceAttribute("DataRate", ns3::StringValue(bandwidth));
    p2pHelper.SetChannelAttribute("Delay", ns3::StringValue(delay));

    ns3::Ipv4AddressHelper ipv4AddressHelper{"10.0.0.0", "255.255.255.0"};
    // server-tor links
    for (SwitchID switch_index = 0; switch_index < switchCount; ++switch_index)
    {
        auto tor = switches.Get(switch_index);
        for (ServerID server_index = 0; server_index < p; ++server_index)
        {
            auto server = servers.Get(switch_index * p + server_index);
            auto torIpv4 = tor->GetObject<ns3::Ipv4>();
            auto serverIpv4 = server->GetObject<ns3::Ipv4>();
            auto torIpv4L3Protocol = tor->GetObject<ns3::Ipv4L3Protocol>();
            auto serverIpv4L3Protocol = server->GetObject<ns3::Ipv4L3Protocol>();
            auto devices = p2pHelper.Install(tor, server);
            auto [torDevice, serverDevice] = ContainerPattern<2>(devices);
            auto addresses = ipv4AddressHelper.Assign(devices);
            auto [torAddress, serverAddress] =
                ContainerPattern<2>(addresses,
                                    [](const ns3::Ipv4InterfaceContainer& c, std::size_t offset) {
                                        return c.GetAddress(offset);
                                    });
            serverMap[server->GetId()] = {serverDevice, serverAddress};
            backgroundLinkMap[tor->GetId()][server->GetId()] = {torDevice,
                                                                serverDevice,
                                                                torIpv4,
                                                                serverIpv4,
                                                                torAddress,
                                                                serverAddress,
                                                                torIpv4L3Protocol,
                                                                serverIpv4L3Protocol};
            backgroundLinkMap[server->GetId()][tor->GetId()] = {serverDevice,
                                                                torDevice,
                                                                serverIpv4,
                                                                torIpv4,
                                                                serverAddress,
                                                                torAddress,
                                                                serverIpv4L3Protocol,
                                                                torIpv4L3Protocol};
            ipv4AddressHelper.NewNetwork();
        }
    }
    // links in every group
    for (GroupID group_index = 0; group_index < g; ++group_index)
    {
        for (SwitchID switch_index1 = 0; switch_index1 < a; ++switch_index1)
        {
            auto tor1 = switches.Get(group_index * a + switch_index1);
            for (SwitchID switch_index2 = switch_index1 + 1; switch_index2 < a; ++switch_index2)
            {
                auto tor2 = switches.Get(group_index * a + switch_index2);
                auto tor1Ipv4 = tor1->GetObject<ns3::Ipv4>();
                auto tor2Ipv4 = tor2->GetObject<ns3::Ipv4>();
                auto tor1Ipv4L3Protocol = tor1->GetObject<ns3::Ipv4L3Protocol>();
                auto tor2Ipv4L3Protocol = tor2->GetObject<ns3::Ipv4L3Protocol>();
                auto devices = p2pHelper.Install(tor1, tor2);
                auto [tor1Device, tor2Device] = ContainerPattern<2>(devices);
                auto addresses = ipv4AddressHelper.Assign(devices);
                auto [tor1Address, tor2Address] =
                    ContainerPattern<2>(addresses,
                                        [](const ns3::Ipv4InterfaceContainer& c,
                                           std::size_t offset) { return c.GetAddress(offset); });
                backgroundLinkMap[tor1->GetId()][tor2->GetId()] = {tor1Device,
                                                                   tor2Device,
                                                                   tor1Ipv4,
                                                                   tor2Ipv4,
                                                                   tor1Address,
                                                                   tor2Address,
                                                                   tor1Ipv4L3Protocol,
                                                                   tor2Ipv4L3Protocol};
                backgroundLinkMap[tor2->GetId()][tor1->GetId()] = {tor2Device,
                                                                   tor1Device,
                                                                   tor2Ipv4,
                                                                   tor1Ipv4,
                                                                   tor2Address,
                                                                   tor1Address,
                                                                   tor2Ipv4L3Protocol,
                                                                   tor1Ipv4L3Protocol};
                ipv4AddressHelper.NewNetwork();
            }
        }
    }
    // links between groups, every group connect to groups whose ID is less than its
    if (!enable_reconfiguration)
    {
        for (GroupID group_index1 = 0; group_index1 < g; ++group_index1)
        {
            for (SwitchID switch_index1 = 0; switch_index1 < a; ++switch_index1)
            {
                auto tor1 = switches.Get(group_index1 * a + switch_index1);
                for (LinkID link_index = 0; link_index < h; ++link_index)
                {
                    auto offset = (switch_index1 * h + link_index) % (g - 1) + 1;
                    auto group_index2 = (g + group_index1 - offset) % g;
                    if (group_index2 >= group_index1)
                    {
                        continue;
                    }
                    auto switch_index2 = a - switch_index1 - 1; // symmetric switch placement
                    auto tor2 = switches.Get(group_index2 * a + switch_index2);
                    auto tor1Ipv4 = tor1->GetObject<ns3::Ipv4>();
                    auto tor2Ipv4 = tor2->GetObject<ns3::Ipv4>();
                    auto tor1Ipv4L3Protocol = tor1->GetObject<ns3::Ipv4L3Protocol>();
                    auto tor2Ipv4L3Protocol = tor2->GetObject<ns3::Ipv4L3Protocol>();
                    auto devices = p2pHelper.Install(tor1, tor2);
                    auto [tor1Device, tor2Device] = ContainerPattern<2>(devices);
                    auto addresses = ipv4AddressHelper.Assign(devices);
                    auto [tor1Address, tor2Address] = ContainerPattern<2>(
                        addresses,
                        [](const ns3::Ipv4InterfaceContainer& c, std::size_t offset) {
                            return c.GetAddress(offset);
                        });
                    backgroundLinkMap[tor1->GetId()][tor2->GetId()] = {tor1Device,
                                                                       tor2Device,
                                                                       tor1Ipv4,
                                                                       tor2Ipv4,
                                                                       tor1Address,
                                                                       tor2Address,
                                                                       tor1Ipv4L3Protocol,
                                                                       tor2Ipv4L3Protocol};
                    backgroundLinkMap[tor2->GetId()][tor1->GetId()] = {tor2Device,
                                                                       tor1Device,
                                                                       tor2Ipv4,
                                                                       tor1Ipv4,
                                                                       tor2Address,
                                                                       tor1Address,
                                                                       tor2Ipv4L3Protocol,
                                                                       tor1Ipv4L3Protocol};
                    ipv4AddressHelper.NewNetwork();
                }
            }
        }
    }

    for (auto iterator = servers.Begin(); iterator < servers.End(); ++iterator)
    {
        auto node = *iterator;
        NS_ASSERT_MSG(
            node->GetNDevices() == 2,
            "network devices of server are not installed correctly"); // with an additional loopback
                                                                      // device
    }

    // for (auto iterator = switches.Begin(); iterator < switches.End(); ++iterator) {
    //     auto node = *iterator;
    //     NS_ASSERT_MSG(node->GetNDevices() == p + a + h, "network devices of tor are not installed
    //     correctly"); // with an additional loopback device
    // }

    NS_ASSERT_MSG(serverMap.size() == serverCount, "conflict server ports");
    auto backgroundLinkCountRange =
        backgroundLinkMap | std::views::values |
        std::views::transform(&std::unordered_map<NS3NodeID, LinkInfo>::size);
    NS_ASSERT_MSG(std::accumulate(backgroundLinkCountRange.begin(),
                                  backgroundLinkCountRange.end(),
                                  (std::size_t)0) <= backgroundLinkCount,
                  "conflict background links");

    // reconfigurable links, disabled by default
    for (OCSLayerID layer = 0; layer < ocs; ++layer)
    {
        auto switch_index = layer % a;
        for (GroupID group_index1 = 0; group_index1 < g; ++group_index1)
        {
            for (GroupID group_index2 = group_index1 + 1; group_index2 < g; ++group_index2)
            {
                auto tor1 = switches.Get(group_index1 * a + switch_index);
                auto tor2 = switches.Get(group_index2 * a + switch_index);
                auto devices = p2pHelper.Install(tor1, tor2);
                auto addresses = ipv4AddressHelper.Assign(devices);
                auto [tor1Device, tor2Device] = ContainerPattern<2>(devices);
                auto tor1Ipv4 = tor1->GetObject<ns3::Ipv4>();
                tor1Ipv4->SetDown(tor1Ipv4->GetInterfaceForDevice(tor1Device));
                auto tor2Ipv4 = tor2->GetObject<ns3::Ipv4>();
                tor2Ipv4->SetDown(tor2Ipv4->GetInterfaceForDevice(tor2Device));
                auto [tor1Address, tor2Address] =
                    ContainerPattern<2>(addresses,
                                        [](const ns3::Ipv4InterfaceContainer& c,
                                           std::size_t offset) { return c.GetAddress(offset); });
                auto tor1Ipv4L3Protocol = tor1->GetObject<ns3::Ipv4L3Protocol>();
                auto tor2Ipv4L3Protocol = tor2->GetObject<ns3::Ipv4L3Protocol>();
                reconfigurableLinkMap[layer][group_index1][group_index2] = {tor1Device,
                                                                            tor2Device,
                                                                            tor1Ipv4,
                                                                            tor2Ipv4,
                                                                            tor1Address,
                                                                            tor2Address,
                                                                            tor1Ipv4L3Protocol,
                                                                            tor2Ipv4L3Protocol};
                reconfigurableLinkMap[layer][group_index2][group_index1] = {tor2Device,
                                                                            tor1Device,
                                                                            tor2Ipv4,
                                                                            tor1Ipv4,
                                                                            tor2Address,
                                                                            tor1Address,
                                                                            tor2Ipv4L3Protocol,
                                                                            tor1Ipv4L3Protocol};
                ipv4AddressHelper.NewNetwork();
            }
        }
    }

    auto reconfigurableLinkCountRange =
        reconfigurableLinkMap | std::views::values | std::views::join | std::views::values |
        std::views::transform(&std::unordered_map<GroupID, LinkInfo>::size);
    NS_ASSERT_MSG(std::accumulate(reconfigurableLinkCountRange.begin(),
                                  reconfigurableLinkCountRange.end(),
                                  (std::size_t)0) >= reconfigurableLinkCount,
                  "conflict reconfigurable links");

    // initialize GroupLinkNumberMap
    for (GroupID i = 0; i < g; i++)
    {
        for (LinkID j = 0; j < g; j++)
        {
            if (i != j)
            {
                groupLinkNumberMap[i][j] = 1;
            }
        }
    }

    // neighbor pattern
    for (uint32_t i = 0; i < g * a * p; i++)
    {
        uint32_t j = (int)(i + a * p) % (int)(g * a * p);
        neighbor_end_pair.push_back({i, (uint32_t)j});
    }

    // ad pattern
    for (uint32_t i = 0; i < g * a * p; i++)
    {
        uint32_t j = (i + a * p) % (2 * a * p) + (int)(i / (2 * a * p)) * 2 * a * p;
        if (j >= g * a * p)
        {
            continue;
        }
        adverse_end_pair.push_back({i, (uint32_t)j});
    }

    if (is_adversial)
    {
        cout << "is_adversial" << endl;
        traffic_pattern_end_pair = adverse_end_pair;
    }
    else
    {
        cout << "is_neighbor" << endl;
        traffic_pattern_end_pair = neighbor_end_pair;
    }

    uint32_t port = 50000;
    ns3::ApplicationContainer sinkApp;
    ns3::Address sinkLocalAddress(ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), port));
    ns3::PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);

    // add server
    for (uint32_t i = 0; i < traffic_pattern_end_pair.size(); i++)
    {
        // std::cout << traffic_pattern_end_pair[i][0] << " " << traffic_pattern_end_pair[i][1] <<
        // endl;
        auto app = sinkHelper.Install(servers.Get(traffic_pattern_end_pair[i][0]));
        // auto address = serverMap[servers.Get(traffic_pattern_end_pair[i][0])->GetId()].address;
        // address2Onoffapplication[address] = app.Get(0);
        sinkApp.Add(app);
    }

    // set server app start&stop time
    sinkApp.Start(ns3::Seconds(start_time));
    sinkApp.Stop(ns3::Seconds(stop_time));

    // set client app
    ns3::OnOffHelper clientHelper("ns3::TcpSocketFactory", ns3::Address());
    clientHelper.SetAttribute("OnTime",
                              ns3::StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper.SetAttribute("OffTime",
                              ns3::StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    clientHelper.SetAttribute("MaxBytes", UintegerValue(maxBytes));
    // can add different clientapp here(eg.clientApp1,clientApp2) to set different start&stop time
    ns3::ApplicationContainer clientApps;

    // add client
    ns3::AddressValue remoteAddress;
    for (uint32_t i = 0; i < traffic_pattern_end_pair.size(); i++)
    {
        // std::cout <<"a "<< traffic_pattern_end_pair[i][0] <<" "<< traffic_pattern_end_pair[i][1]
        // <<" "<< end_interfaces[traffic_pattern_end_pair[i][0]].GetAddress(1) << " ";
        auto peer = servers.Get(traffic_pattern_end_pair[i][0])->GetId();
        remoteAddress = ns3::AddressValue(
            ns3::InetSocketAddress(serverMap[peer].address, port)); // server ip and port
        clientHelper.SetAttribute("Remote", remoteAddress);
        // std::cout <<"b "<< traffic_pattern_end_pair[i][1] << " " <<
        // end_interfaces[traffic_pattern_end_pair[i][1]].GetAddress(1) << endl;
        auto clientApp = clientHelper.Install((servers.Get(traffic_pattern_end_pair[i][1])));
        auto address = serverMap[servers.Get(traffic_pattern_end_pair[i][1])->GetId()].address;
        address2Onoffapplication[address] = DynamicCast<OnOffApplication>(clientApp.Get(0));
        clientApps.Add(clientApp);
    }

    // set the same kind client app start&stop time
    clientApps.Start(ns3::Seconds(start_time));
    clientApps.Stop(ns3::Seconds(stop_time));
    congestionMonitorApplications.Stop(ns3::Seconds(stop_time));

    // uint16_t port = 9999;
    // std::map<ns3::MPIRankIDType, ns3::Address> addresses;
    // std::map<ns3::Address, ns3::MPIRankIDType> ranks;
    // auto traces = ns3::parse_traces(tracePath);
    // NS_ASSERT_MSG(traces.size() <= serverCount, "topology size is not enough");

    // for (ns3::MPIRankIDType rankID = 0; rankID < traces.size(); ++rankID) {
    //     auto node = servers.Get(rankID);
    //     auto ip = serverMap[node->GetId()].address;
    //     addresses[rankID] = ns3::InetSocketAddress{ip, port};
    //     ranks[ip] = rankID;
    // }

    // std::size_t remaining = traces.size();
    // ns3::ApplicationContainer applications;
    // for (auto &trace: traces) {
    //     trace.emplace([&remaining, &congestionMonitorApplications](auto &) ->
    //     ns3::MPIOperation<void> {
    //         --remaining;
    //         if (remaining <= 0) {
    //             congestionMonitorApplications.Stop(ns3::Now());
    //         }
    //         co_return;
    //     });
    //     ns3::MPIRankIDType rankID = applications.GetN();
    //     auto node = servers.Get(rankID);
    //     auto app = ns3::Create<ns3::MPIApplication>(rankID, addresses, ranks, std::move(trace));
    //     node->AddApplication(app);
    //     applications.Add(app);
    // }

    for (OCSLayerID layer = 0; layer < ocs; layer++)
    {
        acticate_one_level(layer);
    }

    if (enable_reconfiguration)
    {
        for (float time = start_time + reconfiguration_timestep;
             time <= start_time + reconfiguration_timestep * reconfiguration_count;
             time += reconfiguration_timestep)
        {
            ns3::Simulator::Schedule(Seconds(time), incremental_change, ugalRoutingHelper);
        }
    }

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    ns3::Simulator::Schedule(Seconds(1.0), &ns3::Ipv4GlobalRoutingHelper::RecomputeRoutingTables);

    ugalRoutingHelper.NotifyLinkChanges();
    congestionMonitorApplications.Start(ns3::Seconds(start_time));
    // applications.Start(ns3::Seconds(0));
    Simulator::Stop(Seconds(stop_time + 10));
    std::cout << "Run..." << endl;
    ns3::Simulator::Run();
    std::cout << "Run done." << endl;
    flowMonitor->SerializeToXmlFile("flow.xml", true, true);
    ns3::Simulator::Destroy();
    std::cout << "Done." << endl;
    return 0;
}
