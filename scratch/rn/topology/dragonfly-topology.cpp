#include "dragonfly-topology.h"

#include "../main.h"

using namespace ns3;
using namespace std;

DragonflyTopology::DragonflyTopology(std::size_t p,
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
                                     bool MUSE)
    : p(p),
      a(a),
      h(h),
      g(g),
      bandwidth(bandwidth),
      delay(delay),
      ocs(ocs),
      ugal(ugal),
      flowRouting(flowRouting),
      congestionMonitorPeriod(congestionMonitorPeriod),
      enable_reconfiguration(enable_reconfiguration),
      is_adversial(is_adversial),
      ecmp(ecmp),
      app_bd(app_bd),
      bias(bias),
      reconfiguration_count(reconfiguration_count),
      only_reconfiguration(only_reconfiguration)
{
    this->stop_time = stop_time;
    this->reconfiguration_timestep = reconfiguration_timestep;
    this->MUSE = MUSE;

    std::function<bool(NS3NetDevice, NS3NetDevice)> linkEnabled = [this](NS3NetDevice device1,
                                                                         NS3NetDevice device2) {
        return this->check_link_status(device1, device2);
    };

    if (ugal && ecmp)
    {
        std::function<IntergroupCost(ns3::NS3NodeID, ns3::NS3NodeID)> initalCongestion =
            [this](NS3NodeID nodeid1, NS3NodeID nodeid2) {
                return this->check_nodes_between_group(nodeid1, nodeid2) ? IntergroupCost{1, 0}
                                                                         : IntergroupCost{0, 1};
            };

        ugalRoutingHelperForECMPUgal = Ipv4UGALRoutingHelper<IntergroupCost>(true,
                                                                             flowRouting,
                                                                             ecmp,
                                                                             RespondToLinkChanges,
                                                                             {},
                                                                             initalCongestion,
                                                                             linkEnabled);
    }
    else
    {
        ugalRoutingHelper = Ipv4UGALRoutingHelper<>(true,
                                                    flowRouting,
                                                    ecmp,
                                                    RespondToLinkChanges,
                                                    bias,
                                                    Ipv4UGALRoutingHelper<>::UnitInitialCongestion,
                                                    linkEnabled);
    }

    std::function<void(Ptr<const NetDevice>, Ptr<const NetDevice>)> routeMoniterCallback =
        [this](Ptr<const NetDevice> device1, Ptr<const NetDevice> device2) {
            this->inter_group_hop_trigger(device1, device2);
        };

    routeMonitorHelper = RouteMonitorHelper<>(routeMoniterCallback);
}

void 
DragonflyTopology::change_all_link_delay(string delay){
    Config::Set("/ChannelList/*/$ns3::PointToPointChannel/Delay", StringValue(delay));
}

void 
DragonflyTopology::change_all_link_bandwidth(string bandwidth){
    Config::Set("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/DataRate", StringValue(bandwidth) );
}

void
DragonflyTopology::inter_group_hop_trigger(Ptr<const NetDevice> device1,
                                           Ptr<const NetDevice> device2)
{
    auto nodeId1 = device1->GetNode()->GetId();
    auto nodeId2 = device2->GetNode()->GetId();
    if (check_nodes_between_group(nodeId1, nodeId2))
    {
        totalHops++;
    }
}

bool
DragonflyTopology::check_nodes_between_group(NS3NodeID nodeid1, NS3NodeID nodeid2)
{
    // end id should be excluded
    if (switchID2Index.find(nodeid1) == switchID2Index.end() ||
        switchID2Index.find(nodeid2) == switchID2Index.end())
    {
        return false;
    }

    auto nodeIndex1 = switchID2Index[nodeid1];
    auto nodeIndex2 = switchID2Index[nodeid2];
    if ((nodeIndex1 / a) != (nodeIndex2 / a))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool
DragonflyTopology::check_link_status(NS3NetDevice device1, NS3NetDevice device2)
{
    return linkDeviceState[device1][device2];
}

void
DragonflyTopology::link_reconfiguration(std::vector<std::size_t>& to_choose_pair)
{
    choosed_pair = to_choose_pair;

    std::vector<GroupID> groupLinkDown;
    std::vector<GroupID> groupLinkUp;
    // find add link
    auto tor1Ipv4 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].ipv41;
    auto device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].device1;
    auto tor2Ipv4 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].ipv42;
    auto device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].device2;
    tor1Ipv4->SetUp(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetUp(tor2Ipv4->GetInterfaceForDevice(device2));
    change_linkdevice_state(device1, device2, true);

    tor1Ipv4 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].device1;
    tor2Ipv4 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].device2;
    tor1Ipv4->SetUp(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetUp(tor2Ipv4->GetInterfaceForDevice(device2));
    change_linkdevice_state(device1, device2, true);

    groupLinkUp.push_back(choosed_pair[0]);
    groupLinkUp.push_back(choosed_pair[1]);
    groupLinkUp.push_back(choosed_pair[2]);
    groupLinkUp.push_back(choosed_pair[3]);

    // find delete link
    auto min_g0_g2 = std::min(choosed_pair[0], choosed_pair[2]);
    auto max_g0_g2 = std::max(choosed_pair[0], choosed_pair[2]);
    tor1Ipv4 = reconfigurableLinkMap[choosed_pair[4]][min_g0_g2][max_g0_g2].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][min_g0_g2][max_g0_g2].device1;
    tor2Ipv4 = reconfigurableLinkMap[choosed_pair[4]][min_g0_g2][max_g0_g2].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][min_g0_g2][max_g0_g2].device2;
    tor1Ipv4->SetDown(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetDown(tor2Ipv4->GetInterfaceForDevice(device2));
    change_linkdevice_state(device1, device2, false);

    auto min_g1_g3 = std::min(choosed_pair[1], choosed_pair[3]);
    auto max_g1_g3 = std::max(choosed_pair[1], choosed_pair[3]);
    tor1Ipv4 = reconfigurableLinkMap[choosed_pair[4]][min_g1_g3][max_g1_g3].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][min_g1_g3][max_g1_g3].device1;
    tor2Ipv4 = reconfigurableLinkMap[choosed_pair[4]][min_g1_g3][max_g1_g3].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][min_g1_g3][max_g1_g3].device2;
    tor1Ipv4->SetDown(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetDown(tor2Ipv4->GetInterfaceForDevice(device2));
    change_linkdevice_state(device1, device2, false);

    groupLinkDown.push_back(choosed_pair[0]);
    groupLinkDown.push_back(choosed_pair[2]);
    groupLinkDown.push_back(choosed_pair[1]);
    groupLinkDown.push_back(choosed_pair[3]);

    std::cout << Simulator::Now() << " groupLinkUp: " << groupLinkUp[0] << " " << groupLinkUp[1]
              << " " << groupLinkUp[2] << " " << groupLinkUp[3] << std::endl;
    std::cout << Simulator::Now() << " groupLinkDown: " << groupLinkDown[0] << " "
              << groupLinkDown[1] << " " << groupLinkDown[2] << " " << groupLinkDown[3]
              << std::endl;

    Ipv4GlobalRoutingHelper::RecomputeRoutingTables();

    if (ugal && ecmp)
    {
        ugalRoutingHelperForECMPUgal->NotifyLinkChanges();
    }
    else if (ugal)
    {
        ugalRoutingHelper->NotifyLinkChanges();
    }
}

void
DragonflyTopology::link_reconfiguration_for_MUSE(std::vector<std::size_t>& to_choose_pair)
{
    choosed_pair = to_choose_pair;

    std::vector<GroupID> groupLinkDown;
    std::vector<GroupID> groupLinkUp;
    // find add link
    auto tor1Ipv4 = backgroundLinkMap[choosed_pair[0]][choosed_pair[1]].ipv41;
    auto device1 = backgroundLinkMap[choosed_pair[0]][choosed_pair[1]].device1;
    auto tor2Ipv4 = backgroundLinkMap[choosed_pair[0]][choosed_pair[1]].ipv42;
    auto device2 = backgroundLinkMap[choosed_pair[0]][choosed_pair[1]].device2;
    tor1Ipv4->SetUp(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetUp(tor2Ipv4->GetInterfaceForDevice(device2));
    change_linkdevice_state(device1, device2, true);

    tor1Ipv4 = backgroundLinkMap[choosed_pair[2]][choosed_pair[3]].ipv41;
    device1 = backgroundLinkMap[choosed_pair[2]][choosed_pair[3]].device1;
    tor2Ipv4 = backgroundLinkMap[choosed_pair[2]][choosed_pair[3]].ipv42;
    device2 = backgroundLinkMap[choosed_pair[2]][choosed_pair[3]].device2;
    tor1Ipv4->SetUp(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetUp(tor2Ipv4->GetInterfaceForDevice(device2));
    change_linkdevice_state(device1, device2, true);

    groupLinkUp.push_back(choosed_pair[0]);
    groupLinkUp.push_back(choosed_pair[1]);
    groupLinkUp.push_back(choosed_pair[2]);
    groupLinkUp.push_back(choosed_pair[3]);

    // find delete link
    auto min_g0_g2 = std::min(choosed_pair[0], choosed_pair[2]);
    auto max_g0_g2 = std::max(choosed_pair[0], choosed_pair[2]);
    tor1Ipv4 = backgroundLinkMap[min_g0_g2][max_g0_g2].ipv41;
    device1 = backgroundLinkMap[min_g0_g2][max_g0_g2].device1;
    tor2Ipv4 = backgroundLinkMap[min_g0_g2][max_g0_g2].ipv42;
    device2 = backgroundLinkMap[min_g0_g2][max_g0_g2].device2;
    tor1Ipv4->SetDown(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetDown(tor2Ipv4->GetInterfaceForDevice(device2));
    change_linkdevice_state(device1, device2, false);

    auto min_g1_g3 = std::min(choosed_pair[1], choosed_pair[3]);
    auto max_g1_g3 = std::max(choosed_pair[1], choosed_pair[3]);
    tor1Ipv4 = backgroundLinkMap[min_g1_g3][max_g1_g3].ipv41;
    device1 = backgroundLinkMap[min_g1_g3][max_g1_g3].device1;
    tor2Ipv4 = backgroundLinkMap[min_g1_g3][max_g1_g3].ipv42;
    device2 = backgroundLinkMap[min_g1_g3][max_g1_g3].device2;
    tor1Ipv4->SetDown(tor1Ipv4->GetInterfaceForDevice(device1));
    tor2Ipv4->SetDown(tor2Ipv4->GetInterfaceForDevice(device2));
    change_linkdevice_state(device1, device2, false);

    groupLinkDown.push_back(choosed_pair[0]);
    groupLinkDown.push_back(choosed_pair[2]);
    groupLinkDown.push_back(choosed_pair[1]);
    groupLinkDown.push_back(choosed_pair[3]);

    std::cout << Simulator::Now() << " groupLinkUp: " << groupLinkUp[0] << " " << groupLinkUp[1]
              << " " << groupLinkUp[2] << " " << groupLinkUp[3] << std::endl;
    std::cout << Simulator::Now() << " groupLinkDown: " << groupLinkDown[0] << " "
              << groupLinkDown[1] << " " << groupLinkDown[2] << " " << groupLinkDown[3]
              << std::endl;

    Ipv4GlobalRoutingHelper::RecomputeRoutingTables();

    if (ugal && ecmp)
    {
        ugalRoutingHelperForECMPUgal->NotifyLinkChanges();
    }
    else if (ugal)
    {
        ugalRoutingHelper->NotifyLinkChanges();
    }
}

void
DragonflyTopology::set_direct_change(Ptr<Ipv4> ipv4, bool value)
{
    ipv4->local_drain(value);
}

void
DragonflyTopology::change_linkdevice_state(NS3NetDevice device1, NS3NetDevice device2, bool state)
{
    linkDeviceState[device1][device2] = state;
    linkDeviceState[device2][device1] = state;
    auto nodeId1 = device1->GetNode()->GetId();
    auto nodeId2 = device2->GetNode()->GetId();
    auto nodeIndex1 = switchID2Index[nodeId1];
    auto nodeIndex2 = switchID2Index[nodeId2];
    auto groupIndex1 = nodeIndex1 / a;
    auto groupIndex2 = nodeIndex2 / a;
    if (state == true)
    {
        groupLinkNumberMap[groupIndex1][groupIndex2]++;
        groupLinkNumberMap[groupIndex2][groupIndex1]++;
    }
    else
    {
        groupLinkNumberMap[groupIndex1][groupIndex2]--;
        groupLinkNumberMap[groupIndex2][groupIndex1]--;
    }
}

void
DragonflyTopology::Direct_change(std::vector<std::size_t>& to_choose_pair)
{
    choosed_pair = to_choose_pair;
    std::cout << "Direct_change " << choosed_pair[0] << " " << choosed_pair[1] << " "
              << choosed_pair[2] << " " << choosed_pair[3] << std::endl;

    // delete link[3][1]
    auto Ipv41 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[3]][choosed_pair[1]].ipv41;
    auto device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[3]][choosed_pair[1]].device1;
    auto ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    set_direct_change(Ipv41, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::set_direct_change,
                        this,
                        Ipv41,
                        true);
    Ipv41->SetDown(ipv4ifIndex1);

    auto Ipv42 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[3]][choosed_pair[1]].ipv42;
    auto device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[3]][choosed_pair[1]].device2;
    auto ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    set_direct_change(Ipv42, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::set_direct_change,
                        this,
                        Ipv42,
                        true);
    Ipv42->SetDown(ipv4ifIndex2);

    change_linkdevice_state(device1, device2, false);

    // delete link[0][2]
    Ipv41 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].device1;
    ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    set_direct_change(Ipv41, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::set_direct_change,
                        this,
                        Ipv41,
                        false);
    Ipv41->SetDown(ipv4ifIndex1);

    Ipv42 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].device2;
    ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    set_direct_change(Ipv42, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::set_direct_change,
                        this,
                        Ipv42,
                        false);
    Ipv42->SetDown(ipv4ifIndex2);

    change_linkdevice_state(device1, device2, false);

    // add link[0][1]
    Ipv41 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].device1;
    ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv41, ipv4ifIndex1);

    Ipv42 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[1]].device2;
    ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv42, ipv4ifIndex2);

    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::change_linkdevice_state,
                        this,
                        device1,
                        device2,
                        true);

    // add link[2][3]
    Ipv41 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].ipv41;
    device1 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].device1;
    ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv41, ipv4ifIndex1);

    Ipv42 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].ipv42;
    device2 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[2]][choosed_pair[3]].device2;
    ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv42, ipv4ifIndex2);

    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::change_linkdevice_state,
                        this,
                        device1,
                        device2,
                        true);

    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &Ipv4GlobalRoutingHelper::RecomputeRoutingTables);

    if (ugal && ecmp)
    {
        ugalRoutingHelperForECMPUgal->NotifyLinkChanges();
        Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                            &ns3::Ipv4UGALRoutingHelper<IntergroupCost>::NotifyLinkChanges,
                            &(*ugalRoutingHelperForECMPUgal));
    }
    else if (ugal)
    {
        ugalRoutingHelper->NotifyLinkChanges();
        Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                            &ns3::Ipv4UGALRoutingHelper<>::NotifyLinkChanges,
                            &(*ugalRoutingHelper));
    }
}

void
DragonflyTopology::Direct_change_for_MUSE(std::vector<std::size_t>& to_choose_pair)
{
    choosed_pair = to_choose_pair;
    std::cout << "Direct_change " << choosed_pair[0] << " " << choosed_pair[1] << " "
              << choosed_pair[2] << " " << choosed_pair[3] << std::endl;

    // delete link[3][1]
    auto Ipv41 = backgroundLinkMap[choosed_pair[3]][choosed_pair[1]].ipv41;
    auto device1 = backgroundLinkMap[choosed_pair[3]][choosed_pair[1]].device1;
    auto ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    set_direct_change(Ipv41, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::set_direct_change,
                        this,
                        Ipv41,
                        true);
    Ipv41->SetDown(ipv4ifIndex1);

    auto Ipv42 = backgroundLinkMap[choosed_pair[3]][choosed_pair[1]].ipv42;
    auto device2 = backgroundLinkMap[choosed_pair[3]][choosed_pair[1]].device2;
    auto ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    set_direct_change(Ipv42, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::set_direct_change,
                        this,
                        Ipv42,
                        true);
    Ipv42->SetDown(ipv4ifIndex2);

    change_linkdevice_state(device1, device2, false);

    // delete link[0][2]
    Ipv41 = backgroundLinkMap[choosed_pair[0]][choosed_pair[2]].ipv41;
    device1 = backgroundLinkMap[choosed_pair[0]][choosed_pair[2]].device1;
    ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    set_direct_change(Ipv41, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::set_direct_change,
                        this,
                        Ipv41,
                        false);
    Ipv41->SetDown(ipv4ifIndex1);

    Ipv42 = backgroundLinkMap[choosed_pair[0]][choosed_pair[2]].ipv42;
    device2 = backgroundLinkMap[choosed_pair[0]][choosed_pair[2]].device2;
    ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    set_direct_change(Ipv42, true);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::set_direct_change,
                        this,
                        Ipv42,
                        false);
    Ipv42->SetDown(ipv4ifIndex2);

    change_linkdevice_state(device1, device2, false);

    // add link[0][1]
    Ipv41 = backgroundLinkMap[choosed_pair[0]][choosed_pair[1]].ipv41;
    device1 = backgroundLinkMap[choosed_pair[0]][choosed_pair[1]].device1;
    ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv41, ipv4ifIndex1);

    Ipv42 = backgroundLinkMap[choosed_pair[0]][choosed_pair[1]].ipv42;
    device2 = backgroundLinkMap[choosed_pair[0]][choosed_pair[1]].device2;
    ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv42, ipv4ifIndex2);

    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::change_linkdevice_state,
                        this,
                        device1,
                        device2,
                        true);

    // add link[2][3]
    Ipv41 = backgroundLinkMap[choosed_pair[2]][choosed_pair[3]].ipv41;
    device1 = backgroundLinkMap[choosed_pair[2]][choosed_pair[3]].device1;
    ipv4ifIndex1 = Ipv41->GetInterfaceForDevice(device1);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv41, ipv4ifIndex1);

    Ipv42 = backgroundLinkMap[choosed_pair[2]][choosed_pair[3]].ipv42;
    device2 = backgroundLinkMap[choosed_pair[2]][choosed_pair[3]].device2;
    ipv4ifIndex2 = Ipv42->GetInterfaceForDevice(device2);
    Simulator::Schedule(Seconds(threshold) + Simulator::Now(), &Ipv4::SetUp, Ipv42, ipv4ifIndex2);

    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &DragonflyTopology::change_linkdevice_state,
                        this,
                        device1,
                        device2,
                        true);

    Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                        &Ipv4GlobalRoutingHelper::RecomputeRoutingTables);

    if (ugal && ecmp)
    {
        ugalRoutingHelperForECMPUgal->NotifyLinkChanges();
        Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                            &ns3::Ipv4UGALRoutingHelper<IntergroupCost>::NotifyLinkChanges,
                            &(*ugalRoutingHelperForECMPUgal));
    }
    else if (ugal)
    {
        ugalRoutingHelper->NotifyLinkChanges();
        Simulator::Schedule(Seconds(threshold) + Simulator::Now(),
                            &ns3::Ipv4UGALRoutingHelper<>::NotifyLinkChanges,
                            &(*ugalRoutingHelper));
    }
}

int
DragonflyTopology::block_flows(Ptr<Node> node, uint32_t ipv4ifIndex, Ptr<Node> peer)
{
    // get all src-dst pair from interface
    auto ipv4 = node->GetObject<Ipv4>();
    auto ipv4l3 = ipv4->GetObject<Ipv4L3Protocol>();
    auto interface = ipv4l3->GetInterface(ipv4ifIndex);
    auto routingTable = ipv4l3->GetRoutingProtocol()->GetObject<Ipv4GlobalRouting>();
    if (routingTable == nullptr)
    { // list routing exists
        auto ListRoutingTable = ipv4l3->GetRoutingProtocol()->GetObject<Ipv4ListRouting>();
        int16_t priority = 0;
        uint32_t index = 1;
        routingTable =
            ListRoutingTable->GetRoutingProtocol(index, priority)->GetObject<Ipv4GlobalRouting>();
    }

    // list routing exists && ecmp + ugal
    if (routingTable == nullptr)
    {
        auto ListRoutingTable = ipv4l3->GetRoutingProtocol()->GetObject<Ipv4ListRouting>();
        int16_t priority = 0;
        uint32_t index = 1;
        auto ugalRouting = ListRoutingTable->GetRoutingProtocol(index, priority)
                               ->GetObject<Ipv4UGALRouting<double>>();

        NS_ASSERT_MSG(ecmp == true, "ecmp is false");

        int nflow = 0;
        for (auto& trafficPair : traffic_pattern_end_pair)
        {
            auto dstNode = trafficPair[0];
            auto srcNode = trafficPair[1];
            auto dstID = servers.Get(dstNode)->GetId();
            auto srcID = servers.Get(srcNode)->GetId();
            // auto dstAddress = serverMap[dstID].address;
            auto srcAddress = serverMap[srcID].address;

            auto minCongestionRoutes = ugalRouting->minCongestionECMPRoute(node->GetId(), dstID);
            auto minHopsRoutes = ugalRouting->minHopsECMPRoute(node->GetId(), dstID);

            for (auto route : minCongestionRoutes)
            {
                if (route.nextHop == peer->GetId())
                {
                    NS_ASSERT_MSG(route.current == node->GetId(), "route.current != node->GetId()");
                    blockedPairState[srcID][dstID] = false;

                    auto sendApp = address2Onoffapplication[srcAddress];
                    auto socket = sendApp->GetSocket();
                    std::function<void(ns3::NS3NodeID, ns3::Address)> callback =
                        [this](ns3::NS3NodeID srcID, ns3::Address dstadd) {
                            alarm_local_drain(srcID, dstadd);
                        };
                    socket->block(callback);

                    nflow++;
                }
            }
            // for(auto route : minHopsRoutes)
            // {
            //     if (route.nextHop == peer->GetId())
            //     {
            //         NS_ASSERT_MSG(route.current == node->GetId(), "route.current !=
            //         node->GetId()"); nflow++; blockedPairState[srcID][dstID] = false;
            //     }
            // }
        }
        std::cout << "nflow: " << nflow << std::endl;
        return nflow;
    }

    int nflow = 0;
    for (auto& trafficPair : traffic_pattern_end_pair)
    {
        auto dstNode = trafficPair[0];
        auto srcNode = trafficPair[1];
        auto dstID = servers.Get(dstNode)->GetId();
        auto srcID = servers.Get(srcNode)->GetId();
        auto dstAddress = serverMap[dstID].address;
        auto srcAddress = serverMap[srcID].address;

        NS_ASSERT_MSG((routingTable) != nullptr, "routingTable is null");
        int nRoutes = routingTable->GetNRoutes();
        for (auto routeIndex = 0; routeIndex < nRoutes; routeIndex++)
        {
            auto route = routingTable->GetRoute(routeIndex);
            if (route->GetInterface() == ipv4ifIndex && route->GetDest() == dstAddress)
            {
                // std::cout << "srcID: " << srcID << " dstID: " << dstID << std::endl;
                blockedPairState[srcID][dstID] = false;
                auto sendApp = address2Onoffapplication[srcAddress];
                auto socket = sendApp->GetSocket();
                std::function<void(ns3::NS3NodeID, ns3::Address)> callback =
                    [this](ns3::NS3NodeID srcID, ns3::Address dstadd) {
                        alarm_local_drain(srcID, dstadd);
                    };
                socket->block(callback);

                nflow++;
            }
            else
            {
                continue;
            }
        }
    }
    std::cout << "nflow: " << nflow << std::endl;
    return nflow;
}

void
DragonflyTopology::unblock_flows()
{
    // find src-dst pair to unblock
    for (auto& srcDstPair : blockedPairState)
    {
        auto srcID = srcDstPair.first;
        auto srcAddress = serverMap[srcID].address;

        auto sendApp = address2Onoffapplication[srcAddress];
        auto socket = sendApp->GetSocket();
        socket->unblock();
    }
}

void
DragonflyTopology::Local_draining(std::vector<std::size_t>& to_choose_pair)
{
    // debug info
    std::cout << "local draining " << to_choose_pair[0] << " " << to_choose_pair[1] << " "
              << to_choose_pair[2] << " " << to_choose_pair[3] << std::endl;

    // ensure previous local draining is done
    for (auto& kv1 : blockedPairState)
    {
        for (auto& kv2 : kv1.second)
        {
            if (kv2.second == false)
            {
                std::cout << "ERROR: Previous local draining is not done." << std::endl;
                return;
            }
        }
    }

    // clear  blockedPairState
    blockedPairState.clear();
    choosed_pair = to_choose_pair;

    // build new blockedPairState
    // // to-delete link1
    auto Ipv411 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].ipv41;
    auto device11 =
        reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].device1;
    auto ipv4ifIndex11 = Ipv411->GetInterfaceForDevice(device11);

    auto Ipv412 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].ipv42;
    auto device12 =
        reconfigurableLinkMap[choosed_pair[4]][choosed_pair[0]][choosed_pair[2]].device2;
    auto ipv4ifIndex12 = Ipv412->GetInterfaceForDevice(device12);

    auto nflow_11 = block_flows(device11->GetNode(), ipv4ifIndex11, device12->GetNode());
    auto nflow_12 = block_flows(device12->GetNode(), ipv4ifIndex12, device11->GetNode());

    // // to-delete link2
    auto Ipv421 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[1]][choosed_pair[3]].ipv41;
    auto device21 =
        reconfigurableLinkMap[choosed_pair[4]][choosed_pair[1]][choosed_pair[3]].device1;
    auto ipv4ifIndex21 = Ipv421->GetInterfaceForDevice(device21);

    auto Ipv422 = reconfigurableLinkMap[choosed_pair[4]][choosed_pair[1]][choosed_pair[3]].ipv42;
    auto device22 =
        reconfigurableLinkMap[choosed_pair[4]][choosed_pair[1]][choosed_pair[3]].device2;
    auto ipv4ifIndex22 = Ipv422->GetInterfaceForDevice(device22);
    auto nflow_21 = block_flows(device21->GetNode(), ipv4ifIndex21, device22->GetNode());
    auto nflow_22 = block_flows(device22->GetNode(), ipv4ifIndex22, device21->GetNode());

    if (nflow_11 == 0 && nflow_12 == 0 && nflow_21 == 0 && nflow_22 == 0)
    {
        link_reconfiguration(choosed_pair);
    }

    std::cout << "blockedPairState size: " << blockedPairState.size() << std::endl;
}

void
DragonflyTopology::Local_draining_for_MUSE(std::vector<std::size_t>& to_choose_pair)
{
    // debug info
    std::cout << "local draining for MUSE " << to_choose_pair[0] << " " << to_choose_pair[1] << " "
              << to_choose_pair[2] << " " << to_choose_pair[3] << std::endl;

    // ensure previous local draining is done
    for (auto& kv1 : blockedPairState)
    {
        for (auto& kv2 : kv1.second)
        {
            if (kv2.second == false)
            {
                std::cout << "ERROR: Previous local draining is not done." << std::endl;
                return;
            }
        }
    }

    // clear  blockedPairState
    blockedPairState.clear();
    choosed_pair = to_choose_pair;

    // build new blockedPairState
    // // to-delete link1
    auto Ipv411 = backgroundLinkMap[choosed_pair[0]][choosed_pair[2]].ipv41;
    auto device11 = backgroundLinkMap[choosed_pair[0]][choosed_pair[2]].device1;
    auto ipv4ifIndex11 = Ipv411->GetInterfaceForDevice(device11);

    auto Ipv412 = backgroundLinkMap[choosed_pair[0]][choosed_pair[2]].ipv42;
    auto device12 = backgroundLinkMap[choosed_pair[0]][choosed_pair[2]].device2;
    auto ipv4ifIndex12 = Ipv412->GetInterfaceForDevice(device12);
    auto nflow_11 = block_flows(device11->GetNode(), ipv4ifIndex11, device12->GetNode());
    auto nflow_12 = block_flows(device12->GetNode(), ipv4ifIndex12, device11->GetNode());

    // // to-delete link2
    auto Ipv421 = backgroundLinkMap[choosed_pair[1]][choosed_pair[3]].ipv41;
    auto device21 = backgroundLinkMap[choosed_pair[1]][choosed_pair[3]].device1;
    auto ipv4ifIndex21 = Ipv421->GetInterfaceForDevice(device21);

    auto Ipv422 = backgroundLinkMap[choosed_pair[1]][choosed_pair[3]].ipv42;
    auto device22 = backgroundLinkMap[choosed_pair[1]][choosed_pair[3]].device2;
    auto ipv4ifIndex22 = Ipv422->GetInterfaceForDevice(device22);
    auto nflow_21 = block_flows(device21->GetNode(), ipv4ifIndex21, device22->GetNode());
    auto nflow_22 = block_flows(device22->GetNode(), ipv4ifIndex22, device21->GetNode());

    if (nflow_11 == 0 && nflow_12 == 0 && nflow_21 == 0 && nflow_22 == 0)
    {
        link_reconfiguration_for_MUSE(choosed_pair);
    }

    std::cout << "blockedPairState size: " << blockedPairState.size() << std::endl;
}

void
DragonflyTopology::alarm_local_drain(NS3NodeID srcID, Address dstadd)
{
    // cout << "alarm_local_drain: " << srcID << " " << dstadd << endl;

    // check if all pairs local drained
    int count = 0;
    bool allLocalDrained = true;
    for (auto& outerPair : blockedPairState)
    {
        if (outerPair.first == srcID)
        {
            // traffic pattern : one src to one dst
            for (auto& innerPair : outerPair.second)
            {
                innerPair.second = true;
            }
            continue;
        }
        for (auto& innerPair : outerPair.second)
        {
            if (innerPair.second == false)
            {
                allLocalDrained = false;
                break;
            }
            else
            {
                count++;
            }
        }
    }

    // std::cout << "allLocalDrained: " << allLocalDrained
    //           << " blockedPairState size: " << blockedPairState.size() << " count: " << count
    //           << endl;

    if (allLocalDrained == true)
    {
        // std::cout << Simulator::Now() << " allLocalDrained" << endl;
        if (MUSE)
        {
            link_reconfiguration_for_MUSE(choosed_pair);
        }
        else
        {
            link_reconfiguration(choosed_pair);
        }
        unblock_flows();
        blockedPairState.clear();
    }
}

// acticate one level
void
DragonflyTopology::activate_one_level(OCSLayerID layer)
{
    // std::cout << "acticate_one_level" << endl;
    std::vector<bool> choose_group(g, 0);
    GroupID choose_g_1 = 0;
    GroupID choose_g_2 = 0;
    std::vector<GroupID> count_choose;
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
            std::swap(choose_g_1, choose_g_2);

        auto ipv41 = reconfigurableLinkMap[layer][choose_g_1][choose_g_2].ipv41;
        auto device1 = reconfigurableLinkMap[layer][choose_g_1][choose_g_2].device1;
        auto ipv41ifIndex = ipv41->GetInterfaceForDevice(device1);
        ipv41->SetUp(ipv41ifIndex);

        auto ipv42 = reconfigurableLinkMap[layer][choose_g_1][choose_g_2].ipv42;
        auto device2 = reconfigurableLinkMap[layer][choose_g_1][choose_g_2].device2;
        auto ipv42ifIndex = ipv42->GetInterfaceForDevice(device2);
        ipv42->SetUp(ipv42ifIndex);

        change_linkdevice_state(device1, device2, true);

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

bool
DragonflyTopology::cmp_change_pair(const std::vector<std::size_t>& a,
                                   const std::vector<std::size_t>& b)
{
    if (a.size() != 4)
    {
        return true;
    }
    if (b.size() != 4)
    {
        return false;
    }
    auto target1 = AVG[a[2]][a[3]] + AVG[a[3]][a[2]] + AVG[a[0]][a[1]] + AVG[a[1]][a[0]] -
                   (AVG[a[0]][a[2]] + AVG[a[2]][a[0]]) - (AVG[a[1]][a[3]] + AVG[a[3]][a[1]]);
    auto target2 = AVG[b[2]][b[3]] + AVG[b[3]][b[2]] + AVG[b[0]][b[1]] + AVG[b[1]][b[0]] -
                   (AVG[b[0]][b[2]] + AVG[b[2]][b[0]]) - (AVG[b[1]][b[3]] + AVG[b[3]][b[1]]);
    if (target1 >= target2)
    {
        return true;
    }
    else if (target1 < target2)
    {
        return false;
    }
    else
    {
        return std::rand() % 2 == 0;
    }
}

// incremental_change
void
DragonflyTopology::incremental_change()
{
    std::cout << Simulator::Now() << " incremental_change" << endl;

    traffic_matrix = std::vector<std::vector<float>>(g, std::vector<float>(g, 0.0));
    topology_matrix = std::vector<std::vector<int>>(g, std::vector<int>(g, 0));
    AVG = std::vector<std::vector<float>>(g, std::vector<float>(g, 0.0));
    float target = 0;
    hottest_pair.clear();
    change_pair.clear();
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
    for (std::vector<uint32_t>& tp : traffic_pattern_end_pair)
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
                std::cout << "no link?" << std::endl;
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
                    auto g_index1 = std::min(hottest_pair[i][0], hottest_pair[i][1]);
                    auto g_index2 = std::max(hottest_pair[i][0], hottest_pair[i][1]);
                    auto g_index3 = std::min(k, l);
                    auto g_index4 = std::max(k, l);
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
                        auto ipv43 = reconfigurableLinkMap[layer][std::min(g_index1, g_index3)]
                                                          [std::max(g_index1, g_index3)]
                                                              .ipv41;
                        auto device3 = reconfigurableLinkMap[layer][std::min(g_index1, g_index3)]
                                                            [std::max(g_index1, g_index3)]
                                                                .device1;
                        auto ipv43ifIndex = ipv43->GetInterfaceForDevice(device3);
                        auto ipv44 = reconfigurableLinkMap[layer][std::min(g_index2, g_index4)]
                                                          [std::max(g_index2, g_index4)]
                                                              .ipv41;
                        auto device4 = reconfigurableLinkMap[layer][std::min(g_index2, g_index4)]
                                                            [std::max(g_index2, g_index4)]
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
                        auto ipv43 = reconfigurableLinkMap[layer][std::min(g_index1, g_index4)]
                                                          [std::max(g_index1, g_index4)]
                                                              .ipv41;
                        auto device3 = reconfigurableLinkMap[layer][std::min(g_index1, g_index4)]
                                                            [std::max(g_index1, g_index4)]
                                                                .device1;
                        auto ipv43ifIndex = ipv43->GetInterfaceForDevice(device3);
                        auto ipv44 = reconfigurableLinkMap[layer][std::min(g_index2, g_index3)]
                                                          [std::max(g_index2, g_index3)]
                                                              .ipv41;
                        auto device4 = reconfigurableLinkMap[layer][std::min(g_index2, g_index3)]
                                                            [std::max(g_index2, g_index3)]
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

    sort(change_pair.begin(),
         change_pair.end(),
         [this](const std::vector<std::size_t>& a, const std::vector<std::size_t>& b) {
             return cmp_change_pair(a, b);
         });
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
        // Direct_change(change_pair[choose_i]);
        // link_reconfiguration(change_pair[choose_i]);
        Local_draining(change_pair[choose_i]);
        break;
    }

    // std::cout << "end increment" << endl;
}

// incremental_change_for_MUSE
void
DragonflyTopology::incremental_change_for_MUSE()
{
    std::cout << Simulator::Now() << " incremental_change_for_MUSE" << endl;

    traffic_matrix = std::vector<std::vector<float>>(g, std::vector<float>(g, 0.0));
    topology_matrix = std::vector<std::vector<int>>(g, std::vector<int>(g, 0));
    AVG = std::vector<std::vector<float>>(g, std::vector<float>(g, 0.0));
    float target = 0;
    hottest_pair.clear();
    change_pair.clear();
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
    for (std::vector<uint32_t>& tp : traffic_pattern_end_pair)
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
                // std::cout << "no link?" << std::endl;
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
        for (GroupID k = 0; k < g; k++)
        {
            for (GroupID l = k + 1; l < g; l++)
            {
                auto g_index1 = std::min(hottest_pair[i][0], hottest_pair[i][1]);
                auto g_index2 = std::max(hottest_pair[i][0], hottest_pair[i][1]);
                auto g_index3 = std::min(k, l);
                auto g_index4 = std::max(k, l);
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
                    target = hottest + AVG[g_index3][g_index4] + AVG[g_index4][g_index3] -
                             (AVG[g_index1][g_index3] + AVG[g_index3][g_index1]) -
                             (AVG[g_index2][g_index4] + AVG[g_index4][g_index2]);
                    change_pair.push_back({g_index1, g_index2, g_index3, g_index4});
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
                    target = hottest + AVG[g_index3][g_index4] + AVG[g_index4][g_index3] -
                             (AVG[g_index1][g_index4] + AVG[g_index4][g_index1]) -
                             (AVG[g_index2][g_index3] + AVG[g_index3][g_index2]);
                    change_pair.push_back({g_index1, g_index2, g_index4, g_index3});
                }
                // std::cout << hottest_pair[i][0] << " " <<  hottest_pair[i][1] << " " << k <<
                // " " << l << " " << hottest + AVG[k][l] + AVG[l][k] -
                // (AVG[hottest_pair[i][1]][k] + AVG[k][hottest_pair[i][1]]) -
                // (AVG[hottest_pair[i][0]][l] + AVG[l][hottest_pair[i][0]]) << endl;
            }
        }
    }

    // sort(change_pair.begin(),
    //      change_pair.end(),
    //      [this](const std::vector<std::size_t>& a, const std::vector<std::size_t>& b) {
    //          return cmp_change_pair(a, b);
    //      });

    // sort change pair with AVG value ( if target is same, random choose order )
    for (uint32_t i = 0; i < change_pair.size(); i++)
    {
        for (uint32_t j = i + 1; j < change_pair.size(); j++)
        {
            auto a = change_pair[i];
            auto b = change_pair[j];
            auto target1 = AVG[a[2]][a[3]] + AVG[a[3]][a[2]] + AVG[a[0]][a[1]] + AVG[a[1]][a[0]] -
                           (AVG[a[0]][a[2]] + AVG[a[2]][a[0]]) -
                           (AVG[a[1]][a[3]] + AVG[a[3]][a[1]]);
            auto target2 = AVG[b[2]][b[3]] + AVG[b[3]][b[2]] + AVG[b[0]][b[1]] + AVG[b[1]][b[0]] -
                           (AVG[b[0]][b[2]] + AVG[b[2]][b[0]]) -
                           (AVG[b[1]][b[3]] + AVG[b[3]][b[1]]);
            if (target1 < target2 || (target1 == target2 && rand() % 2))
            {
                swap(change_pair[i], change_pair[j]);
            }
        }
    }

    int num = change_pair.size();
    bool choose_success = false;
    for (int choose_i = 0; choose_i < num; choose_i++)
    {
        std::vector<std::vector<SwitchID>> candidate_switch_change_pair;
        for (auto switchIndex1 = change_pair[choose_i][0] * a;
             switchIndex1 < change_pair[choose_i][0] * a + a;
             switchIndex1++)
        {
            for (auto switchIndex2 = change_pair[choose_i][1] * a;
                 switchIndex2 < change_pair[choose_i][1] * a + a;
                 switchIndex2++)
            {
                for (auto switchIndex3 = change_pair[choose_i][2] * a;
                     switchIndex3 < change_pair[choose_i][2] * a + a;
                     switchIndex3++)
                {
                    for (auto switchIndex4 = change_pair[choose_i][3] * a;
                         switchIndex4 < change_pair[choose_i][3] * a + a;
                         switchIndex4++)
                    {
                        candidate_switch_change_pair.push_back(
                            {switchIndex1, switchIndex2, switchIndex3, switchIndex4});
                    }
                }
            }
        }

        while (!choose_success && !candidate_switch_change_pair.empty())
        {
            auto randomIndex = std::rand() % candidate_switch_change_pair.size();
            auto& switch_change_pair = candidate_switch_change_pair[randomIndex];

            auto switchId1 = switches.Get(switch_change_pair[0])->GetId();
            auto switchId2 = switches.Get(switch_change_pair[1])->GetId();
            auto switchId3 = switches.Get(switch_change_pair[2])->GetId();
            auto switchId4 = switches.Get(switch_change_pair[3])->GetId();
            auto ipv411 = backgroundLinkMap[switchId1][switchId2].ipv41;
            auto ipv412 = backgroundLinkMap[switchId1][switchId2].ipv42;
            auto ipv421 = backgroundLinkMap[switchId3][switchId4].ipv41;
            auto ipv422 = backgroundLinkMap[switchId3][switchId4].ipv42;
            auto ipv431 = backgroundLinkMap[switchId1][switchId3].ipv41;
            auto ipv432 = backgroundLinkMap[switchId1][switchId3].ipv42;
            auto ipv441 = backgroundLinkMap[switchId2][switchId4].ipv41;
            auto ipv442 = backgroundLinkMap[switchId2][switchId4].ipv42;
            auto device11 = backgroundLinkMap[switchId1][switchId2].device1;
            auto device12 = backgroundLinkMap[switchId1][switchId2].device2;
            auto device21 = backgroundLinkMap[switchId3][switchId4].device1;
            auto device22 = backgroundLinkMap[switchId3][switchId4].device2;
            auto device31 = backgroundLinkMap[switchId1][switchId3].device1;
            auto device32 = backgroundLinkMap[switchId1][switchId3].device2;
            auto device41 = backgroundLinkMap[switchId2][switchId4].device1;
            auto device42 = backgroundLinkMap[switchId2][switchId4].device2;
            auto ipv4ifIndex11 = ipv411->GetInterfaceForDevice(device11);
            auto ipv4ifIndex12 = ipv412->GetInterfaceForDevice(device12);
            auto ipv4ifIndex21 = ipv421->GetInterfaceForDevice(device21);
            auto ipv4ifIndex22 = ipv422->GetInterfaceForDevice(device22);
            auto ipv4ifIndex31 = ipv431->GetInterfaceForDevice(device31);
            auto ipv4ifIndex32 = ipv432->GetInterfaceForDevice(device32);
            auto ipv4ifIndex41 = ipv441->GetInterfaceForDevice(device41);
            auto ipv4ifIndex42 = ipv442->GetInterfaceForDevice(device42);

            if (!ipv411->IsUp(ipv4ifIndex11) && !ipv412->IsUp(ipv4ifIndex12) &&
                !ipv421->IsUp(ipv4ifIndex21) && !ipv422->IsUp(ipv4ifIndex22) &&
                ipv431->IsUp(ipv4ifIndex31) && ipv432->IsUp(ipv4ifIndex32) &&
                ipv441->IsUp(ipv4ifIndex41) && ipv442->IsUp(ipv4ifIndex42))
            {
                std::vector<std::size_t> to_choose_pair = {switchId1,
                                                           switchId2,
                                                           switchId3,
                                                           switchId4};

                NS_ASSERT_MSG(switchID2Index[switchId1] / a != switchID2Index[switchId2] / a &&
                                  switchID2Index[switchId3] / a != switchID2Index[switchId4] / a &&
                                  switchID2Index[switchId1] / a != switchID2Index[switchId3] / a &&
                                  switchID2Index[switchId2] / a != switchID2Index[switchId4] / a,
                              "choose within one group");

                if (ugal && enable_reconfiguration && !ecmp)
                {
                    Direct_change_for_MUSE(to_choose_pair);
                }
                else
                {
                    // Local_draining_for_MUSE(to_choose_pair);
                    // link_reconfiguration_for_MUSE(to_choose_pair);
                    Direct_change_for_MUSE(to_choose_pair);
                }
                choose_success = true;
                break;
            }

            candidate_switch_change_pair.erase(candidate_switch_change_pair.begin() + randomIndex);
        }

        if (choose_success)
        {
            break;
        }
    }

    if (!choose_success)
    {
        std::cout << "no incremental choose : " << change_pair.size() << std::endl;
    }
}