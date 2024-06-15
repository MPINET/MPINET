//
// Created by Ricardo Evans on 2023/7/14.
//

#include "mpi-util.h"

ns3::Address ns3::retrieveIPAddress(const Address &address) {
    if (InetSocketAddress::IsMatchingType(address)) {
        auto socketAddress = InetSocketAddress::ConvertFrom(address);
        return socketAddress.GetIpv4();
    }
    if (Inet6SocketAddress::IsMatchingType(address)) {
        auto socketAddress = Inet6SocketAddress::ConvertFrom(address);
        return socketAddress.GetIpv6();
    }
    throw std::runtime_error("unsupported address type");
}

uint16_t ns3::retrievePort(const Address &address) {
    if (InetSocketAddress::IsMatchingType(address)) {
        auto socketAddress = InetSocketAddress::ConvertFrom(address);
        return socketAddress.GetPort();
    }
    if (Inet6SocketAddress::IsMatchingType(address)) {
        auto socketAddress = Inet6SocketAddress::ConvertFrom(address);
        return socketAddress.GetPort();
    }
    throw std::runtime_error("unsupported address type");
}

ns3::Address ns3::addressWithPort(const Address &address, uint16_t port) {
    if (Ipv4Address::IsMatchingType(address)) {
        return InetSocketAddress{Ipv4Address::ConvertFrom(address), port};
    }
    if (Ipv6Address::IsMatchingType(address)) {
        return Inet6SocketAddress{Ipv6Address::ConvertFrom(address), port};
    }
    throw std::runtime_error("unsupported address type");
}
