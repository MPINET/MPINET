#include "flexfly-topology.h"

#include "../main.h"

using namespace ns3;
using namespace std;

FlexflyTopology::FlexflyTopology(std::size_t p,
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
                                 bool only_reconfiguration)
    : DragonflyTopology(p,
                        a,
                        h,
                        g,
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
                        only_reconfiguration,
                        false)
{
}

void
FlexflyTopology::optimized_links(int optimizedGroupPairs, int optimizedLinkPerPair)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    if (traffic_pattern_end_pair.size() == adverse_end_pair.size() || traffic_pattern_end_pair.size() == neighbor_end_pair.size()) // both traffic pattern and adverse traffic pattern
    {
        for (auto traffic_pair = 0; traffic_pair < optimizedGroupPairs; traffic_pair++)
        {
            std::vector<std::pair<std::size_t, std::size_t>> candicate_delete_link;
            std::vector<std::pair<std::size_t, std::size_t>> candicate_add_link;
            auto swtich_optimized_index_min = 2 * traffic_pair * a;
            auto swtich_optimized_index_max = 2 * (traffic_pair + 1) * a - 1;
            for (auto switchindex = swtich_optimized_index_max + 1; switchindex < g * a;
                 switchindex++)
            {
                for (auto swtich_optimized_index = swtich_optimized_index_min;
                     swtich_optimized_index <= swtich_optimized_index_max;
                     swtich_optimized_index++)
                {
                    candicate_delete_link.push_back({switchindex, swtich_optimized_index});
                }
            }
            for (auto swtich_optimized_index1 = swtich_optimized_index_min;
                 swtich_optimized_index1 < swtich_optimized_index_min + a;
                 swtich_optimized_index1++)
            {
                for (auto swtich_optimized_index2 = swtich_optimized_index_min + a;
                     swtich_optimized_index2 < swtich_optimized_index_max;
                     swtich_optimized_index2++)
                {
                    candicate_add_link.push_back(
                        {swtich_optimized_index1, swtich_optimized_index2});
                }
            }

            auto delete_link = groupLinkNumberMap[traffic_pair * 2][traffic_pair * 2 + 1];
            auto add_link = groupLinkNumberMap[traffic_pair * 2][traffic_pair * 2 + 1];
            while (delete_link < optimizedLinkPerPair && !candicate_delete_link.empty())
            {
                auto randomIndex = std::rand() % candicate_delete_link.size();
                auto switch_index_pair = candicate_delete_link[randomIndex];
                auto switch_index1 = switch_index_pair.first;
                auto switch_index2 = switch_index_pair.second;
                auto switchID1 = switches.Get(switch_index1)->GetId();
                auto switchID2 = switches.Get(switch_index2)->GetId();

                if (backgroundLinkMap.find(switchID1) == backgroundLinkMap.end() ||
                    backgroundLinkMap[switchID1].find(switchID2) ==
                        backgroundLinkMap[switchID1].end() ||
                    backgroundLinkMap.find(switchID2) == backgroundLinkMap.end() ||
                    backgroundLinkMap[switchID2].find(switchID1) ==
                        backgroundLinkMap[switchID2].end())
                {
                    std::cout << "ERROR: switch1 or switch2 not found in backgroundLinkMap" << endl;
                    continue;
                }

                auto tor1Ipv4 = backgroundLinkMap[switchID1][switchID2].ipv41;
                auto tor1Device = backgroundLinkMap[switchID1][switchID2].device1;
                auto tor2Ipv4 = backgroundLinkMap[switchID1][switchID2].ipv42;
                auto tor2Device = backgroundLinkMap[switchID1][switchID2].device2;
                
                // check the graph is connected
                auto groupLinkNumberswitch1 = 0;
                auto groupLinkNumberswitch2 = 0;
                for (GroupID groupId = 0; groupId < g; groupId++)
                {
                    if (groupId != switch_index1 / a)
                    {
                        groupLinkNumberswitch1 += groupLinkNumberMap[switch_index1 / a][groupId];
                    }
                    if (groupId != switch_index2 / a)
                    {
                        groupLinkNumberswitch2 += groupLinkNumberMap[switch_index2 / a][groupId];
                    }
                }

                if (tor1Ipv4->IsUp(tor1Ipv4->GetInterfaceForDevice(tor1Device)) &&
                    tor2Ipv4->IsUp(tor2Ipv4->GetInterfaceForDevice(tor2Device)) &&
                    groupLinkNumberswitch1 > 1 && groupLinkNumberswitch2 > 1)
                {
                    tor1Ipv4->SetDown(tor1Ipv4->GetInterfaceForDevice(tor1Device));
                    tor2Ipv4->SetDown(tor2Ipv4->GetInterfaceForDevice(tor2Device));
                    change_linkdevice_state(tor1Device, tor2Device, false);
                    delete_link++;
                }
                candicate_delete_link.erase(candicate_delete_link.begin() + randomIndex);
            }

            while (add_link < optimizedLinkPerPair && !candicate_add_link.empty())
            {
                auto randomIndex = std::rand() % candicate_add_link.size();
                auto switch_index_pair = candicate_add_link[randomIndex];
                auto switch_index1 = switch_index_pair.first;
                auto switch_index2 = switch_index_pair.second;
                auto switchID1 = switches.Get(switch_index1)->GetId();
                auto switchID2 = switches.Get(switch_index2)->GetId();

                auto tor1Ipv4 = backgroundLinkMap[switchID1][switchID2].ipv41;
                auto tor1Device = backgroundLinkMap[switchID1][switchID2].device1;
                auto tor2Ipv4 = backgroundLinkMap[switchID1][switchID2].ipv42;
                auto tor2Device = backgroundLinkMap[switchID1][switchID2].device2;

                if (!tor1Ipv4->IsUp(tor1Ipv4->GetInterfaceForDevice(tor1Device)) ||
                    !tor2Ipv4->IsUp(tor2Ipv4->GetInterfaceForDevice(tor2Device)))
                {
                    tor1Ipv4->SetUp(tor1Ipv4->GetInterfaceForDevice(tor1Device));
                    tor2Ipv4->SetUp(tor2Ipv4->GetInterfaceForDevice(tor2Device));
                    change_linkdevice_state(tor1Device, tor2Device, true);
                    add_link++;
                }
                candicate_add_link.erase(candicate_add_link.begin() + randomIndex);
            }

            std::cout << "delete_link:" << delete_link << " add_link:" << add_link << std::endl;
        }
    }
    else
    {
        // neighbor
    }
}