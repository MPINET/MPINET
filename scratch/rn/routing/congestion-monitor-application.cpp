#include <cmath>

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/log.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>

#include "congestion-monitor-application.h"
#include "ipv4-routing-utility.h"

NS_LOG_COMPONENT_DEFINE("CongestionMonitorApplication");

ns3::CongestionMonitorApplication::CongestionMonitorApplication(bool usingAbsoluteQueueLength, const Time &period, const ns3::CongestionFeedbackCallback &callback) : usingAbsoluteQueueLength(usingAbsoluteQueueLength), period(period), callback(callback) {}

ns3::CongestionMonitorApplication::CongestionMonitorApplication(Time &&period, ns3::CongestionFeedbackCallback &&callback) : period(std::move(period)), callback(std::move(callback)) {}

void ns3::CongestionMonitorApplication::StartApplication() {
    running = true;
    Simulator::ScheduleNow(&CongestionMonitorApplication::monitor, this);
}

void ns3::CongestionMonitorApplication::StopApplication() {
    running = false;
}

void ns3::CongestionMonitorApplication::monitor() {
    auto localNode = GetNode();
    auto localIpv4 = localNode->GetObject<Ipv4>();
    for (uint32_t i = 0; i < localNode->GetNDevices(); ++i) {
        auto device = localNode->GetDevice(i);
        auto localInterface = localIpv4->GetInterfaceForDevice(device);
        if (localInterface < 0 || localIpv4->GetNAddresses(localInterface) <= 0) {
            NS_LOG_WARN("Invalid local interface");
            continue;
        }
        auto localAddress = localIpv4->GetAddress(localInterface, 0).GetLocal();
        if (DynamicCast<LoopbackNetDevice>(device)) {
            continue;
        }
        if (device->IsPointToPoint()) {
            auto remote = RemoteDevice(device);
            auto remoteNode = remote->GetNode();
            auto remoteIpv4 = remoteNode->GetObject<Ipv4>();
            auto remoteInterface = remoteIpv4->GetInterfaceForDevice(remote);
            if (remoteInterface < 0 || remoteIpv4->GetNAddresses(remoteInterface) <= 0) {
                NS_LOG_WARN("Invalid remote interface");
                continue;
            }
            auto remoteAddress = remoteIpv4->GetAddress(remoteInterface, 0).GetLocal();
            if (localIpv4->IsUp(localInterface) && localIpv4->IsForwarding(localInterface) && remoteIpv4->IsUp(remoteInterface) && remoteIpv4->IsForwarding(remoteInterface)) {
                auto pointToPointDevice = DynamicCast<PointToPointNetDevice>(device);
                auto queue = pointToPointDevice->GetQueue();
                double cost = usingAbsoluteQueueLength ? queue->GetCurrentSize().GetValue() : 1.0 * queue->GetCurrentSize().GetValue() / queue->GetMaxSize().GetValue();
                cost = std::max(cost, 0.0001);
                NS_LOG_LOGIC("Cost from " << localAddress << " to " << remoteAddress << ": " << cost);
                if (callback) {
                    callback(localAddress, remoteAddress, cost);
                }
            }
        } else {
            NS_LOG_WARN("Unknown device type");
            continue;
        }
    }
    NS_LOG_LOGIC("congestion monitor on " << localNode->GetId() << " finished an epoch at " << Simulator::Now());
    if (running) {
        Simulator::Schedule(period, &CongestionMonitorApplication::monitor, this);
    }
}
