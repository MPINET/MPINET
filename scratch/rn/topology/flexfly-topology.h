#ifndef FLEXFLY_TOPOLOGY_H
#define FLEXFLY_TOPOLOGY_H

#include "../main.h"
#include "../routing/ipv4-ugal-routing-helper.h"
#include "../utility.h"
#include "dragonfly-topology.h"

#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/mpi-application-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>

using namespace std;
using namespace ns3;

class FlexflyTopology : public DragonflyTopology
{
  public:
    void optimized_links(int optimizedGroupPairs, int optimizedLinkPerPair);
    // initialize topology use (parameter to initialize topology)
    FlexflyTopology(std::size_t p,
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
                    bool only_reconfiguration);
};

#endif // FLEXFLY_TOPOLOGY_H
