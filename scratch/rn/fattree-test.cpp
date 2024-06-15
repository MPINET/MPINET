//
// Created by Ricardo Evans on 2023/10/4.
//

#include "main.h"
#include "routing/ipv4-ugal-routing-helper.h"
#include "topology/fattree-topology.h"
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

int
main(int argc, char** argv)
{
    // ns3::LogComponentEnable("Ipv4UGALRouting", ns3::LOG_LEVEL_ALL);
    // ns3::LogComponentEnable("CongestionMonitorApplication", ns3::LOG_LEVEL_ALL);
    // ns3::LogComponentEnable("Ipv4UGALRoutingHelper", ns3::LOG_LEVEL_ALL);
    // ns3::LogComponentEnableAll(ns3::LOG_LEVEL_WARN);
    // ns3::LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
    // ns3::LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    // setting default parameters
    std::string tracePath = "";

    float start_time = 1.5;
    float stop_time = 1.5006;

    std::size_t k = 4;
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
    bool only_reconfiguration = false;
    uint32_t maxBytes = 1500;

    // double threshold = 0.00000001;

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
    cmd.AddValue(
        "k",
        "the fattree parameter k, represents the number of the ports per switch in the topology",
        k);
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
    cmd.AddValue("only_reconfig", "whether no background links", only_reconfiguration);
    cmd.AddValue("max_bytes", "the max bytes of flow", maxBytes);

    cmd.Parse(argc, argv);

    if (ecmp)
    {
        Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",
                           BooleanValue(true)); // 启用多路径路由
    }

    // the rate and packet size of sending
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(1024));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(app_bd));

    ns3::Time::SetResolution(ns3::Time::NS);

    FattreeTopology fattree(k,
                            bandwidth,
                            delay,
                            ocs,
                            ugal,
                            flowRouting,
                            congestionMonitorPeriod,
                            enable_reconfiguration,
                            reconfiguration_timestep,
                            stop_time,
                            is_adversial,
                            ecmp,
                            app_bd,
                            bias,
                            reconfiguration_count,
                            only_reconfiguration);

    fattree.core_switches.Create(fattree.number_of_core_switches);
    fattree.aggre_switches.Create(fattree.number_of_aggre_switches);
    fattree.edge_switches.Create(fattree.number_of_edge_switches);
    fattree.servers.Create(fattree.number_of_servers);

    fattree.switches.Add(fattree.core_switches);
    fattree.switches.Add(fattree.aggre_switches);
    fattree.switches.Add(fattree.edge_switches);
    fattree.allNodes.Add(fattree.switches);
    fattree.allNodes.Add(fattree.servers);

    // install routing
    if (ugal && ecmp)
    {
        auto listRoutinghelper = Ipv4ListRoutingHelper();
        listRoutinghelper.Add(*fattree.ugalRoutingHelperForECMPUgal, 0);
        listRoutinghelper.Add(*fattree.routeMonitorHelper, 1);
        fattree.internetStackHelper.SetRoutingHelper(listRoutinghelper);
        // congestion monitor should not be with inter-group-ecmp+ugal
    }
    else if (ugal)
    {
        auto listRoutinghelper = Ipv4ListRoutingHelper();
        listRoutinghelper.Add(*fattree.ugalRoutingHelper, 0);
        listRoutinghelper.Add(*fattree.routeMonitorHelper, 1);
        fattree.internetStackHelper.SetRoutingHelper(listRoutinghelper);
        // congestion monitor should not be with inter-group-ecmp+ugal
        fattree.congestionMonitorApplications =
            fattree.ugalRoutingHelper->InstallCongestionMonitorApplication(
                fattree.allNodes,
                ns3::Seconds(congestionMonitorPeriod));
    }
    else
    {
        auto listRoutinghelper = Ipv4ListRoutingHelper();
        listRoutinghelper.Add(fattree.globalRoutingHelper, 0);
        listRoutinghelper.Add(*fattree.routeMonitorHelper, 1);
        fattree.internetStackHelper.SetRoutingHelper(listRoutinghelper);
    }

    // install internet stack
    fattree.internetStackHelper.Install(fattree.allNodes);

    // set p2p link parameters
    ns3::PointToPointHelper p2pHelper;
    p2pHelper.SetDeviceAttribute("DataRate", ns3::StringValue(bandwidth));
    p2pHelper.SetChannelAttribute("Delay", ns3::StringValue(delay));

    // set ipv4 address
    ns3::Ipv4AddressHelper ipv4AddressHelper{"10.0.0.0", "255.255.255.0"};

    // background links
    // server-tor links
    for (SwitchID switch_index = 0; switch_index < fattree.number_of_edge_switches; ++switch_index)
    {
        auto tor = fattree.edge_switches.Get(switch_index);
        for (ServerID server_index = 0; server_index < fattree.number_of_servers_per_edge_switch;
             ++server_index)
        {
            auto server = fattree.servers.Get(switch_index * fattree.number_of_servers_per_edge_switch + server_index);
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
            fattree.serverMap[server->GetId()] = {serverDevice, serverAddress};
            fattree.backgroundLinkMap[tor->GetId()][server->GetId()] = {torDevice,
                                                                        serverDevice,
                                                                        torIpv4,
                                                                        serverIpv4,
                                                                        torAddress,
                                                                        serverAddress,
                                                                        torIpv4L3Protocol,
                                                                        serverIpv4L3Protocol};
            fattree.backgroundLinkMap[server->GetId()][tor->GetId()] = {serverDevice,
                                                                        torDevice,
                                                                        serverIpv4,
                                                                        torIpv4,
                                                                        serverAddress,
                                                                        torAddress,
                                                                        serverIpv4L3Protocol,
                                                                        torIpv4L3Protocol};
            fattree.linkDeviceState[torDevice][serverDevice] = true;
            fattree.linkDeviceState[serverDevice][torDevice] = true;
            ipv4AddressHelper.NewNetwork();
        }
    }

    // links in every pod
    for (GroupID group_index = 0; group_index < fattree.number_of_pod; ++group_index)
    {
        for (SwitchID switch_index1 = 0; switch_index1 < fattree.number_of_edge_switches_per_pod;
             ++switch_index1)
        { // tor1 means edge switch
            auto tor1 = fattree.edge_switches.Get(
                group_index * fattree.number_of_edge_switches_per_pod + switch_index1);
            for (SwitchID switch_index2 = switch_index1 + 1;
                 switch_index2 < fattree.number_of_aggre_switches_per_pod;
                 ++switch_index2)
            { // tor2 means aggre switch
                auto tor2 = fattree.aggre_switches.Get(
                    group_index * fattree.number_of_aggre_switches_per_pod + switch_index2);
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
                fattree.backgroundLinkMap[tor1->GetId()][tor2->GetId()] = {tor1Device,
                                                                           tor2Device,
                                                                           tor1Ipv4,
                                                                           tor2Ipv4,
                                                                           tor1Address,
                                                                           tor2Address,
                                                                           tor1Ipv4L3Protocol,
                                                                           tor2Ipv4L3Protocol};
                fattree.backgroundLinkMap[tor2->GetId()][tor1->GetId()] = {tor2Device,
                                                                           tor1Device,
                                                                           tor2Ipv4,
                                                                           tor1Ipv4,
                                                                           tor2Address,
                                                                           tor1Address,
                                                                           tor2Ipv4L3Protocol,
                                                                           tor1Ipv4L3Protocol};
                fattree.linkDeviceState[tor1Device][tor2Device] = true;
                fattree.linkDeviceState[tor2Device][tor1Device] = true;
                ipv4AddressHelper.NewNetwork();
            }
        }
    }

    // links between core and aggre
    for (SwitchID switch_index1 = 0; switch_index1 < fattree.number_of_core_switches;
         ++switch_index1)
    { // tor1 means core switch
        auto tor1 = fattree.core_switches.Get(switch_index1);
        for (GroupID group_index = 0; group_index < fattree.number_of_pod; ++group_index)
        {
            // tor2 means aggre switch
            auto switch_index2 = switch_index1 / fattree.number_of_aggre_switches_per_pod;
            auto tor2 = fattree.aggre_switches.Get(
                group_index * fattree.number_of_aggre_switches_per_pod + switch_index2);
            auto tor1Ipv4 = tor1->GetObject<ns3::Ipv4>();
            auto tor2Ipv4 = tor2->GetObject<ns3::Ipv4>();
            auto tor1Ipv4L3Protocol = tor1->GetObject<ns3::Ipv4L3Protocol>();
            auto tor2Ipv4L3Protocol = tor2->GetObject<ns3::Ipv4L3Protocol>();
            auto devices = p2pHelper.Install(tor1, tor2);
            auto [tor1Device, tor2Device] = ContainerPattern<2>(devices);
            auto addresses = ipv4AddressHelper.Assign(devices);
            auto [tor1Address, tor2Address] =
                ContainerPattern<2>(addresses,
                                    [](const ns3::Ipv4InterfaceContainer& c, std::size_t offset) {
                                        return c.GetAddress(offset);
                                    });
            fattree.backgroundLinkMap[tor1->GetId()][tor2->GetId()] = {tor1Device,
                                                                       tor2Device,
                                                                       tor1Ipv4,
                                                                       tor2Ipv4,
                                                                       tor1Address,
                                                                       tor2Address,
                                                                       tor1Ipv4L3Protocol,
                                                                       tor2Ipv4L3Protocol};
            fattree.backgroundLinkMap[tor2->GetId()][tor1->GetId()] = {tor2Device,
                                                                       tor1Device,
                                                                       tor2Ipv4,
                                                                       tor1Ipv4,
                                                                       tor2Address,
                                                                       tor1Address,
                                                                       tor2Ipv4L3Protocol,
                                                                       tor1Ipv4L3Protocol};
            fattree.linkDeviceState[tor1Device][tor2Device] = true;
            fattree.linkDeviceState[tor2Device][tor1Device] = true;
            ipv4AddressHelper.NewNetwork();
        }
    }

    for (auto iterator = fattree.servers.Begin(); iterator < fattree.servers.End(); ++iterator)
    {
        auto node = *iterator;
        NS_ASSERT_MSG(
            node->GetNDevices() == 2,
            "network devices of server are not installed correctly"); // with an additional loopback
                                                                      // device
    }

    for (auto iterator = fattree.switches.Begin(); iterator < fattree.switches.End(); ++iterator)
    {
        auto node = *iterator;
        NS_ASSERT_MSG(node->GetNDevices() == k,
                      "network devices of tor are not installed correctly"); // with an additional
                                                                             // loopback device
    }

    auto backgroundLinkCount = (fattree.switches.GetN() * k + fattree.servers.GetN() * 1) / 2;

    NS_ASSERT_MSG(fattree.serverMap.size() == fattree.number_of_servers, "conflict server ports");
    auto backgroundLinkCountRange =
        fattree.backgroundLinkMap | std::views::values |
        std::views::transform(&std::unordered_map<NS3NodeID, LinkInfo>::size);
    NS_ASSERT_MSG(std::accumulate(backgroundLinkCountRange.begin(),
                                  backgroundLinkCountRange.end(),
                                  (std::size_t)0) == backgroundLinkCount,
                  "conflict background links");

    // reconfigurable links, disabled by default
    for (OCSLayerID layer = 0; layer < ocs; ++layer)
    { // three layers all have ocs
        // layer top : between core and aggre
        for (SwitchID switch_index1 = 0; switch_index1 < fattree.number_of_core_switches;
             ++switch_index1)
        { // tor1 means core switch
            for (GroupID group_index = 0; group_index < fattree.number_of_pod; ++group_index)
            { // tor2 means aggre switch
                auto switch_index2 =
                    group_index * fattree.number_of_aggre_switches_per_pod +
                    (switch_index1 / fattree.number_of_aggre_switches_per_pod + layer) %
                        fattree.number_of_aggre_switches_per_pod;
                auto tor1 = fattree.core_switches.Get(switch_index1);
                auto tor2 = fattree.aggre_switches.Get(switch_index2);
                auto devices = p2pHelper.Install(tor1, tor2);
                auto addresses = ipv4AddressHelper.Assign(devices);
                auto [tor1Device, tor2Device] = ContainerPattern<2>(devices);
                // top layer: link is up when initialized
                auto tor1Ipv4 = tor1->GetObject<ns3::Ipv4>();
                auto tor2Ipv4 = tor2->GetObject<ns3::Ipv4>();
                auto [tor1Address, tor2Address] =
                    ContainerPattern<2>(addresses,
                                        [](const ns3::Ipv4InterfaceContainer& c,
                                           std::size_t offset) { return c.GetAddress(offset); });
                auto tor1Ipv4L3Protocol = tor1->GetObject<ns3::Ipv4L3Protocol>();
                auto tor2Ipv4L3Protocol = tor2->GetObject<ns3::Ipv4L3Protocol>();
                fattree.top_reconfigurableLinkMap[layer][switch_index1][group_index] = {
                    tor1Device,
                    tor2Device,
                    tor1Ipv4,
                    tor2Ipv4,
                    tor1Address,
                    tor2Address,
                    tor1Ipv4L3Protocol,
                    tor2Ipv4L3Protocol};
                // top layer: link is up when initialized
                fattree.linkDeviceState[tor1Device][tor2Device] = true;
                fattree.linkDeviceState[tor2Device][tor1Device] = true;
                ipv4AddressHelper.NewNetwork();
            }
        }

        // layer middle: between aggre and edge (down by default)
        auto switch_index = layer % fattree.number_of_aggre_switches_per_pod;
        for (GroupID group_index1 = 0; group_index1 < fattree.number_of_pod; ++group_index1)
        { // tor1 means aggre switch
            for (GroupID group_index2 = 0; group_index2 < fattree.number_of_pod; ++group_index2)
            { // tor2 means edge switch
                auto tor1 = fattree.aggre_switches.Get(
                    group_index1 * fattree.number_of_aggre_switches_per_pod + switch_index);
                auto tor2 = fattree.edge_switches.Get(
                    group_index2 * fattree.number_of_edge_switches_per_pod + switch_index);
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
                fattree.reconfigurableLinkMap[layer][group_index1][group_index2] = {
                    tor1Device,
                    tor2Device,
                    tor1Ipv4,
                    tor2Ipv4,
                    tor1Address,
                    tor2Address,
                    tor1Ipv4L3Protocol,
                    tor2Ipv4L3Protocol};
                fattree.linkDeviceState[tor1Device][tor2Device] = false;
                fattree.linkDeviceState[tor2Device][tor1Device] = false;
                ipv4AddressHelper.NewNetwork();
            }
        }
    }

    auto reconfigurableLinkCount = fattree.ocs * fattree.number_of_aggre_switches_per_pod *
                                   fattree.number_of_edge_switches_per_pod;

    auto reconfigurableLinkCountRange =
        fattree.reconfigurableLinkMap | std::views::values | std::views::join | std::views::values |
        std::views::transform(&std::unordered_map<GroupID, LinkInfo>::size);
    NS_ASSERT_MSG(std::accumulate(reconfigurableLinkCountRange.begin(),
                                  reconfigurableLinkCountRange.end(),
                                  (std::size_t)0) == reconfigurableLinkCount,
                  "conflict reconfigurable links");

    // initialize GroupLinkNumberMap
    for (GroupID i = 0; i < fattree.number_of_pod; i++)
    {
        for (LinkID j = 0; j < fattree.number_of_pod; j++)
        {
            if (i != j)
            {
                fattree.groupLinkNumberMap[i][j] = 1;
            }
        }
    }

    // neighbor pattern
    for (uint32_t i = 0; i < fattree.number_of_servers; i++)
    {
        uint32_t j =
            (int)(i + fattree.number_of_servers_per_pod) % (int)(fattree.number_of_servers);
        fattree.neighbor_end_pair.push_back({i, (uint32_t)j});
    }

    // ad pattern
    for (uint32_t i = 0; i < fattree.number_of_servers; i++)
    {
        uint32_t j =
            (i + fattree.number_of_servers_per_pod) % (2 * fattree.number_of_servers_per_pod) +
            (int)(i / (2 * fattree.number_of_servers_per_pod)) * 2 *
                fattree.number_of_servers_per_pod;
        if (j >= fattree.number_of_servers)
        {
            continue;
        }
        fattree.adverse_end_pair.push_back({i, (uint32_t)j});
    }

    // set traffic pattern
    if (is_adversial)
    {
        cout << "is_adversial" << endl;
        fattree.traffic_pattern_end_pair.assign(fattree.adverse_end_pair.begin(),
                                                fattree.adverse_end_pair.end());
    }
    else
    {
        cout << "is_neighbor" << endl;
        fattree.traffic_pattern_end_pair.assign(fattree.neighbor_end_pair.begin(),
                                                fattree.neighbor_end_pair.end());
    }

    // set server app: sinkApp parameters
    uint32_t port = 50000;
    ns3::ApplicationContainer sinkApp;
    ns3::Address sinkLocalAddress(ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), port));
    ns3::PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);

    // add server
    for (uint32_t i = 0; i < fattree.traffic_pattern_end_pair.size(); i++)
    {
        // std::cout << traffic_pattern_end_pair[i][0] << " " << traffic_pattern_end_pair[i][1] <<
        // endl;
        auto app = sinkHelper.Install(fattree.servers.Get(fattree.traffic_pattern_end_pair[i][0]));
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
    std::cout << "maxBytes " << maxBytes << std::endl;

    // can add different clientapp here(eg.clientApp1,clientApp2) to set different start&stop time
    ns3::ApplicationContainer clientApps;

    // add client
    ns3::AddressValue remoteAddress;
    for (uint32_t i = 0; i < fattree.traffic_pattern_end_pair.size(); i++)
    {
        // std::cout <<"a "<< traffic_pattern_end_pair[i][0] <<" "<< traffic_pattern_end_pair[i][1]
        // <<" "<< end_interfaces[traffic_pattern_end_pair[i][0]].GetAddress(1) << " ";
        auto peer = fattree.servers.Get(fattree.traffic_pattern_end_pair[i][0])->GetId();
        remoteAddress = ns3::AddressValue(
            ns3::InetSocketAddress(fattree.serverMap[peer].address, port)); // server ip and port
        clientHelper.SetAttribute("Remote", remoteAddress);
        // std::cout <<"b "<< traffic_pattern_end_pair[i][1] << " " <<
        // end_interfaces[traffic_pattern_end_pair[i][1]].GetAddress(1) << endl;
        auto clientApp =
            clientHelper.Install((fattree.servers.Get(fattree.traffic_pattern_end_pair[i][1])));
        auto address =
            fattree.serverMap[fattree.servers.Get(fattree.traffic_pattern_end_pair[i][1])->GetId()]
                .address;
        fattree.address2Onoffapplication[address] = DynamicCast<OnOffApplication>(clientApp.Get(0));
        clientApps.Add(clientApp);
    }

    // set the same kind client app start&stop time
    clientApps.Start(ns3::Seconds(start_time));
    clientApps.Stop(ns3::Seconds(stop_time));
    fattree.congestionMonitorApplications.Stop(ns3::Seconds(stop_time));

    // uint16_t port = 9999;
    // std::map<ns3::MPIRankIDType, ns3::Address> addresses;
    // std::map<ns3::Address, ns3::MPIRankIDType> ranks;
    // auto traces = ns3::parse_traces(tracePath);
    // NS_ASSERT_MSG(traces.size() <= serverCount, "fattree size is not enough");

    // for (ns3::MPIRankIDType rankID = 0; rankID < traces.size(); ++rankID) {
    //     auto node = fattree.servers.Get(rankID);
    //     auto ip = fattree.serverMap[node->GetId()].address;
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
    //     auto node = fattree.servers.Get(rankID);
    //     auto app = ns3::Create<ns3::MPIApplication>(rankID, addresses, ranks, std::move(trace));
    //     node->AddApplication(app);
    //     applications.Add(app);
    // }

    mip::Network network;
    fattree.initializeNetwork(network);

    // set aggre/edge/core SwitchID2Index
    for (auto i = 0; i < (int)fattree.number_of_aggre_switches; ++i)
    {
        fattree.aggreSwitchID2Index[fattree.aggre_switches.Get(i)->GetId()] = i;
    }
    for (auto i = 0; i < (int)fattree.number_of_edge_switches; ++i)
    {
        fattree.edgeSwitchID2Index[fattree.edge_switches.Get(i)->GetId()] = i;
    }
    for (auto i = 0; i < (int)fattree.number_of_core_switches; ++i)
    {
        fattree.coreSwitchID2Index[fattree.core_switches.Get(i)->GetId()] = i;
    }

    // activate one level if ocs enabled
    for (OCSLayerID layer = 0; layer < ocs; layer++)
    {
        fattree.activate_one_level(layer);
    }

    if (enable_reconfiguration)
    {
        for (float time = start_time + reconfiguration_timestep;
             time < start_time + reconfiguration_timestep * reconfiguration_count;
             time += reconfiguration_timestep)
        {
            ns3::Simulator::Schedule(Seconds(time), &FattreeTopology::incremental_change, &fattree);
        }
    }

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    ns3::Simulator::Schedule(Seconds(1.0), &ns3::Ipv4GlobalRoutingHelper::RecomputeRoutingTables);

    if (ugal && ecmp)
    {
        fattree.ugalRoutingHelperForECMPUgal->NotifyLinkChanges();
    }
    else if (ugal)
    {
        fattree.ugalRoutingHelper->NotifyLinkChanges();
    }
    fattree.congestionMonitorApplications.Start(ns3::Seconds(start_time));
    // applications.Start(ns3::Seconds(0));
    Simulator::Stop(Seconds(stop_time + 10));
    std::cout << "Run..." << endl;
    ns3::Simulator::Run();
    std::cout << "Run done." << endl;
    std::cout << "totalHops: " << fattree.totalHops << endl;
    flowMonitor->SerializeToXmlFile("flow.xml", true, true);
    ns3::Simulator::Destroy();
    std::cout << "Done." << endl;
    return 0;
}
