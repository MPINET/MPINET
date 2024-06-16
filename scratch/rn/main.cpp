#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>

#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mpi-application-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>

#include "routing/ipv4-ugal-routing-helper.h"
#include "main.h"
#include "utility.h"

int main(int argc, char **argv) {
//    ns3::LogComponentEnable("Ipv4UGALRouting", ns3::LOG_LEVEL_ALL);
//    ns3::LogComponentEnable("CongestionMonitorApplication", ns3::LOG_LEVEL_ALL);
//    ns3::LogComponentEnable("Ipv4UGALRoutingHelper", ns3::LOG_LEVEL_ALL);
//    ns3::LogComponentEnableAll(ns3::LOG_LEVEL_WARN);
    std::string tracePath = "../scratch/rn/traces/HPCG";

    std::size_t p = 2;
    std::size_t a = 4;
    std::size_t h = 2;
    std::size_t g = 0;
    std::string bandwidth = "10Gbps";
    std::string delay = "1us";
    std::size_t ocs = 0;
    bool ugal = false;
    bool flowRouting = false;
    double congestionMonitorPeriod = 0.1;
    ns3::CommandLine cmd;
    cmd.Usage("simulations of HPC applications in reconfigurable network");
    cmd.AddNonOption("trace-path", "the path containing binary dumpi traces", tracePath);
    cmd.AddValue("p", "the dragonfly parameter p, represents the number of servers per TOR switch", p);
    cmd.AddValue("a", "the dragonfly parameter a, represents the number of TOR switches per group ", a);
    cmd.AddValue("h", "the dragonfly parameter h, represents the count of inter-group links per TOR switch", h);
    cmd.AddValue("g", "the dragonfly parameter g, represents the number of groups, setting to zero means using balanced dragonfly (g = a * h + 1)", g);
    cmd.AddValue("bandwidth", "the bandwidth of the links in the topology", bandwidth);
    cmd.AddValue("delay", "the delay of the links in the topology", delay);
    cmd.AddValue("ocs", "the ocs count used in the reconfigurable topology, each ocs is connected to a TOR in every group", ocs);
    cmd.AddValue("ugal", "whether to use ugal routing", ugal);
    cmd.AddValue("flow-routing", "whether to use flow routing", flowRouting);
    cmd.AddValue("congestion-monitor-period", "the period of congestion monitor in seconds", congestionMonitorPeriod);
    cmd.Parse(argc, argv);

    if (g <= 0) { // using balanced dragonfly according to the definitions of options
        g = a * h + 1;
    }
    NS_ASSERT_MSG((a * h) % (g - 1) == 0, "invalid group count");
    auto switchCount = g * a;
    auto serverCount = switchCount * p;
    auto backgroundLinkCount = switchCount * (a + h - 1) + serverCount * 2; // server-tor links + links in every group + links between groups (simplex links)
    auto reconfigurableLinkCount = ocs * g * (g - 1); // ocs links between groups (simplex links)

    ns3::Time::SetResolution(ns3::Time::NS);

    ns3::NodeContainer switches;
    ns3::NodeContainer servers;
    ns3::NodeContainer allNodes;

    switches.Create(switchCount);
    servers.Create(serverCount);
    allNodes.Add(switches);
    allNodes.Add(servers);

    ServerMap serverMap;
    BackgroundLinkMap backgroundLinkMap;
    ReconfigurableLinkMap reconfigurableLinkMap;

    ns3::InternetStackHelper internetStackHelper;
    ns3::Ipv4UGALRoutingHelper ugalRoutingHelper{flowRouting, RespondToLinkChanges};
    ns3::Ipv4GlobalRoutingHelper globalRoutingHelper;
    ns3::ApplicationContainer congestionMonitorApplications;
    if (ugal) {
        internetStackHelper.SetRoutingHelper(ugalRoutingHelper);
        congestionMonitorApplications = ugalRoutingHelper.InstallCongestionMonitorApplication(allNodes, ns3::Seconds(congestionMonitorPeriod));
    } else {
        internetStackHelper.SetRoutingHelper(globalRoutingHelper);
    }
    internetStackHelper.Install(allNodes);

    ns3::PointToPointHelper p2pHelper;
    p2pHelper.SetDeviceAttribute("DataRate", ns3::StringValue(bandwidth));
    p2pHelper.SetChannelAttribute("Delay", ns3::StringValue(delay));

    ns3::Ipv4AddressHelper ipv4AddressHelper{"10.0.0.0", "255.255.255.252"};
    // server-tor links
    for (SwitchID switch_index = 0; switch_index < switchCount; ++switch_index) {
        auto tor = switches.Get(switch_index);
        for (ServerID server_index = 0; server_index < p; ++server_index) {
            auto server = servers.Get(switch_index * p + server_index);
            auto torIpv4 = tor->GetObject<ns3::Ipv4>();
            auto serverIpv4 = server->GetObject<ns3::Ipv4>();
            auto devices = p2pHelper.Install(tor, server);
            auto [torDevice, serverDevice] = ContainerPattern<2>(devices);
            auto addresses = ipv4AddressHelper.Assign(devices);
            auto [torAddress, serverAddress] = ContainerPattern<2>(addresses, [](const ns3::Ipv4InterfaceContainer &c, std::size_t offset) { return c.GetAddress(offset); });
            serverMap[server->GetId()] = {serverDevice, serverAddress};
            backgroundLinkMap[tor->GetId()][server->GetId()] = {torDevice, serverDevice, torIpv4, serverIpv4, torAddress, serverAddress};
            backgroundLinkMap[server->GetId()][tor->GetId()] = {serverDevice, torDevice, serverIpv4, torIpv4, serverAddress, torAddress};
            ipv4AddressHelper.NewNetwork();
        }
    }
    // links in every group
    for (GroupID group_index = 0; group_index < g; ++group_index) {
        for (SwitchID switch_index1 = 0; switch_index1 < a; ++switch_index1) {
            auto tor1 = switches.Get(group_index * a + switch_index1);
            for (SwitchID switch_index2 = switch_index1 + 1; switch_index2 < a; ++switch_index2) {
                auto tor2 = switches.Get(group_index * a + switch_index2);
                auto tor1Ipv4 = tor1->GetObject<ns3::Ipv4>();
                auto tor2Ipv4 = tor2->GetObject<ns3::Ipv4>();
                auto devices = p2pHelper.Install(tor1, tor2);
                auto [tor1Device, tor2Device] = ContainerPattern<2>(devices);
                auto addresses = ipv4AddressHelper.Assign(devices);
                auto [tor1Address, tor2Address] = ContainerPattern<2>(addresses, [](const ns3::Ipv4InterfaceContainer &c, std::size_t offset) { return c.GetAddress(offset); });
                backgroundLinkMap[tor1->GetId()][tor2->GetId()] = {tor1Device, tor2Device, tor1Ipv4, tor2Ipv4, tor1Address, tor2Address};
                backgroundLinkMap[tor2->GetId()][tor1->GetId()] = {tor2Device, tor1Device, tor2Ipv4, tor1Ipv4, tor2Address, tor1Address};
                ipv4AddressHelper.NewNetwork();
            }
        }
    }
    // links between groups, every group connect to groups whose ID is less than its
    for (GroupID group_index1 = 0; group_index1 < g; ++group_index1) {
        for (SwitchID switch_index1 = 0; switch_index1 < a; ++switch_index1) {
            auto tor1 = switches.Get(group_index1 * a + switch_index1);
            for (LinkID link_index = 0; link_index < h; ++link_index) {
                auto offset = (switch_index1 * h + link_index) % (g - 1) + 1;
                auto group_index2 = (g + group_index1 - offset) % g;
                if (group_index2 >= group_index1) {
                    continue;
                }
                auto switch_index2 = a - switch_index1 - 1; // symmetric switch placement
                auto tor2 = switches.Get(group_index2 * a + switch_index2);
                auto tor1Ipv4 = tor1->GetObject<ns3::Ipv4>();
                auto tor2Ipv4 = tor2->GetObject<ns3::Ipv4>();
                auto devices = p2pHelper.Install(tor1, tor2);
                auto [tor1Device, tor2Device] = ContainerPattern<2>(devices);
                auto addresses = ipv4AddressHelper.Assign(devices);
                auto [tor1Address, tor2Address] = ContainerPattern<2>(addresses, [](const ns3::Ipv4InterfaceContainer &c, std::size_t offset) { return c.GetAddress(offset); });
                backgroundLinkMap[tor1->GetId()][tor2->GetId()] = {tor1Device, tor2Device, tor1Ipv4, tor2Ipv4, tor1Address, tor2Address};
                backgroundLinkMap[tor2->GetId()][tor1->GetId()] = {tor2Device, tor1Device, tor2Ipv4, tor1Ipv4, tor2Address, tor1Address};
                ipv4AddressHelper.NewNetwork();
            }
        }
    }

    for (auto iterator = servers.Begin(); iterator < servers.End(); ++iterator) {
        auto node = *iterator;
        NS_ASSERT_MSG(node->GetNDevices() == 2, "network devices of server are not installed correctly"); // with an additional loopback device
    }

    for (auto iterator = switches.Begin(); iterator < switches.End(); ++iterator) {
        auto node = *iterator;
        NS_ASSERT_MSG(node->GetNDevices() == p + a + h, "network devices of tor are not installed correctly"); // with an additional loopback device
    }

    NS_ASSERT_MSG(serverMap.size() == serverCount, "conflict server ports");
    auto backgroundLinkCountRange = backgroundLinkMap | std::views::values | std::views::transform(&std::unordered_map<NS3NodeID, LinkInfo>::size);
    NS_ASSERT_MSG(std::accumulate(backgroundLinkCountRange.begin(), backgroundLinkCountRange.end(), (std::size_t) 0) == backgroundLinkCount, "conflict background links");

    // reconfigurable links, disabled by default
    for (OCSLayerID layer = 0; layer < ocs; ++layer) {
        auto switch_index = layer % a;
        for (GroupID group_index1 = 0; group_index1 < g; ++group_index1) {
            for (GroupID group_index2 = group_index1 + 1; group_index2 < g; ++group_index2) {
                auto tor1 = switches.Get(group_index1 * a + switch_index);
                auto tor2 = switches.Get(group_index2 * a + switch_index);
                auto devices = p2pHelper.Install(tor1, tor2);
                auto addresses = ipv4AddressHelper.Assign(devices);
                auto [tor1Device, tor2Device] = ContainerPattern<2>(devices);
                auto tor1Ipv4 = tor1->GetObject<ns3::Ipv4>();
                tor1Ipv4->SetDown(tor1Ipv4->GetInterfaceForDevice(tor1Device));
                auto tor2Ipv4 = tor2->GetObject<ns3::Ipv4>();
                tor2Ipv4->SetDown(tor2Ipv4->GetInterfaceForDevice(tor2Device));
                auto [tor1Address, tor2Address] = ContainerPattern<2>(addresses, [](const ns3::Ipv4InterfaceContainer &c, std::size_t offset) { return c.GetAddress(offset); });
                reconfigurableLinkMap[layer][group_index1][group_index2] = {tor1Device, tor2Device, tor1Ipv4, tor2Ipv4, tor1Address, tor2Address};
                reconfigurableLinkMap[layer][group_index2][group_index1] = {tor2Device, tor1Device, tor2Ipv4, tor1Ipv4, tor2Address, tor1Address};
                ipv4AddressHelper.NewNetwork();
            }
        }
    }

    auto reconfigurableLinkCountRange = reconfigurableLinkMap | std::views::values | std::views::join | std::views::values | std::views::transform(&std::unordered_map<GroupID, LinkInfo>::size);
    NS_ASSERT_MSG(std::accumulate(reconfigurableLinkCountRange.begin(), reconfigurableLinkCountRange.end(), (std::size_t) 0) == reconfigurableLinkCount, "conflict reconfigurable links");

    uint16_t port = 9999;
    std::map<ns3::MPIRankIDType, ns3::Address> addresses;
    std::map<ns3::Address, ns3::MPIRankIDType> ranks;
    auto traces = ns3::parse_traces(tracePath);
    NS_ASSERT_MSG(traces.size() <= serverCount, "topology size is not enough");

    for (ns3::MPIRankIDType rankID = 0; rankID < traces.size(); ++rankID) {
        auto node = servers.Get(rankID);
        auto ip = serverMap[node->GetId()].address;
        addresses[rankID] = ns3::InetSocketAddress{ip, port};
        ranks[ip] = rankID;
    }

    std::size_t remaining = traces.size();
    ns3::ApplicationContainer applications;
    for (auto &trace: traces) {
        trace.emplace([&remaining, &congestionMonitorApplications](auto &) -> ns3::MPIOperation<void> {
            --remaining;
            if (remaining <= 0) {
                congestionMonitorApplications.Stop(ns3::Now());
            }
            co_return;
        });
        ns3::MPIRankIDType rankID = applications.GetN();
        auto node = servers.Get(rankID);
        auto app = ns3::Create<ns3::MPIApplication>(rankID, addresses, ranks, std::move(trace));
        node->AddApplication(app);
        applications.Add(app);
    }

    ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    ugalRoutingHelper.NotifyLinkChanges();
    congestionMonitorApplications.Start(ns3::Seconds(0));
    applications.Start(ns3::Seconds(0));
    ns3::Simulator::Run();
    ns3::Simulator::Destroy();
    return 0;
}
