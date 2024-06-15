//
// Created by Ricardo Evans on 2023/9/18.
//

#ifndef NS3_IPV4_ROUTING_UTILITY_H
#define NS3_IPV4_ROUTING_UTILITY_H

#include <unordered_map>

#include <ns3/internet-module.h>
#include <optional>

namespace ns3 {
    struct Ipv4Flow {
        Ipv4Address src;
        Ipv4Address dst;

        std::strong_ordering operator<=>(const Ipv4Flow &rhs) const = default;

        bool operator==(const Ipv4Flow &rhs) const = default;
    };

    Ptr<NetDevice> RemoteDevice(Ptr<NetDevice> localDevice);

    Ptr<const NetDevice> RemoteDevice(Ptr<const NetDevice> localDevice);

    template<typename T>
    struct OptionalInserter {
        using difference_type [[maybe_unused]] = std::ptrdiff_t;

        std::optional<T> *o;

        OptionalInserter(std::optional<T> &o) : o(&o) {}

        OptionalInserter &operator++() {
            return *this;
        }

        OptionalInserter &operator++(int) {
            return *this;
        }

        OptionalInserter &operator*() {
            return *this;
        }

        template<typename U>
        OptionalInserter &operator=(U &&value) {
            o->emplace(std::forward<U>(value));
            return *this;
        }
    };
}

template<>
struct std::hash<ns3::Ipv4Address> {
    size_t operator()(const ns3::Ipv4Address &address) const {
        return hash<uint32_t>{}(address.Get());
    }
};

template<>
struct std::hash<ns3::Ipv4Flow> {
    std::size_t operator()(const ns3::Ipv4Flow &flow) const noexcept {
        return std::hash<uint32_t>{}(flow.src.Get()) ^ std::hash<uint32_t>{}(flow.dst.Get());
    }
};

#endif //NS3_IPV4_ROUTING_UTILITY_H
