#ifndef NS3_CONGESTION_MONITOR_APPLICATION_H
#define NS3_CONGESTION_MONITOR_APPLICATION_H

#include <functional>

#include <ns3/application.h>

namespace ns3 {
    using CongestionFeedbackCallback = std::function<void(Ipv4Address, Ipv4Address, double)>;

    class CongestionMonitorApplication : public Application {
    private:
        bool running = false;
        bool usingAbsoluteQueueLength;
        Time period;
        CongestionFeedbackCallback callback;

        void monitor();

    public:
        CongestionMonitorApplication(bool usingAbsoluteQueueLength, const Time &period, const CongestionFeedbackCallback &callback);

        CongestionMonitorApplication(Time &&period, CongestionFeedbackCallback &&callback);

        void StartApplication() override;

        void StopApplication() override;

        ~CongestionMonitorApplication() override = default;
    };
}

#endif //NS3_CONGESTION_MONITOR_APPLICATION_H
