//
// Created by Ricardo Evans on 2023/9/21.
//

#include "ipv4-routing-utility.h"

ns3::Ptr<ns3::NetDevice> ns3::RemoteDevice(Ptr<NetDevice> localDevice) {
    auto channel = localDevice->GetChannel();
    auto deviceCount = channel->GetNDevices();
    NS_ASSERT_MSG(deviceCount == 2, "not point to point channel");
    auto device1 = channel->GetDevice(0);
    auto device2 = channel->GetDevice(1);
    return device1 == localDevice ? device2 : device1;
}

ns3::Ptr<const ns3::NetDevice> ns3::RemoteDevice(Ptr<const NetDevice> localDevice) {
    auto channel = localDevice->GetChannel();
    auto deviceCount = channel->GetNDevices();
    NS_ASSERT_MSG(deviceCount == 2, "not point to point channel");
    auto device1 = channel->GetDevice(0);
    auto device2 = channel->GetDevice(1);
    return device1 == localDevice ? device2 : device1;
}
