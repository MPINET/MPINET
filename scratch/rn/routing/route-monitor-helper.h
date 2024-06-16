#ifndef NS3_ROUTE_MONITOR_HELPER_H
#define NS3_ROUTE_MONITOR_HELPER_H

#include <ns3/internet-module.h>

#include "route-monitor.h"

namespace ns3 {

    // Place this first in the list routing
    template<HeaderConcept ...Headers>
    class RouteMonitorHelper : public Ipv4RoutingHelper {
    private:
        RouteMonitorCallback<Headers...> callback;
    public:
        explicit RouteMonitorHelper(RouteMonitorCallback<Headers...> callback) : callback(std::move(callback)) {}

        Ptr<Ipv4RoutingProtocol> Create(Ptr<Node> node) const override {
            return CreateObject<RouteMonitor<Headers...>>(callback);
        }

        Ipv4RoutingHelper* Copy() const override {
            return new RouteMonitorHelper(*this);
        }

        ~RouteMonitorHelper() override = default;
    };
}

#endif //NS3_ROUTE_MONITOR_HELPER_H
