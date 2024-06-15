//
// Created by Ricardo Evans on 2024/1/11.
//

#ifndef NS3_HIERARCHIC_CONGESTION_H
#define NS3_HIERARCHIC_CONGESTION_H

#include <compare>

class IntergroupCost {
private:
    std::size_t globalHops;
    std::size_t localHops;
public:
    IntergroupCost() = default;

    IntergroupCost(std::size_t globalHops, std::size_t localHops) : globalHops(globalHops), localHops(localHops) {}

    IntergroupCost operator+(const IntergroupCost &other) const {
        if (other.globalHops > 0) {
            return IntergroupCost(globalHops + other.globalHops, 0);
        }
        return IntergroupCost(globalHops + other.globalHops, localHops + other.localHops);
    }

    std::partial_ordering operator<=>(const IntergroupCost &other) const {
        auto globalOrder = globalHops <=> other.globalHops;
        if (globalOrder != std::partial_ordering::equivalent) {
            return globalOrder;
        }
        if (localHops <= 1 && other.localHops <= 1) {
            return std::partial_ordering::equivalent;
        }
        return localHops <=> other.localHops;
    }
};

#endif //NS3_HIERARCHIC_CONGESTION_H
