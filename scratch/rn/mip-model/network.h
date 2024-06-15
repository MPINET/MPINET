//
// Created by Ricardo Evans on 2023/9/5.
//

#ifndef NS3_NETWORK_H
#define NS3_NETWORK_H

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <gurobi_c++.h>

namespace mip {
    struct Node {
        const std::string name;

        operator std::string() const noexcept;

        bool operator==(const Node &) const noexcept = default;
    };

    struct Edge {
        const std::string name;
        const Node start;
        const Node end;
        double capacity;

        operator std::string() const noexcept;

        bool operator==(const Edge &other) const noexcept;

        double netCoefficient(const Node &node) const noexcept;
    };

    struct Flow {
        const Node start;
        const Node end;
        double rate;

        operator std::string() const noexcept;

        bool operator==(const Flow &other) const noexcept;

        double netRateAt(const Node &node) const noexcept;
    };
}

template<>
struct std::hash<mip::Node> {
    std::size_t operator()(const mip::Node &node) const noexcept {
        return std::hash<std::string>()(node.name);
    }
};

template<>
struct std::hash<mip::Edge> {
    std::size_t operator()(const mip::Edge &edge) const noexcept {
        return std::hash<std::string>()(edge.name);
    }
};

template<>
struct std::hash<mip::Flow> {
    std::size_t operator()(const mip::Flow &flow) const noexcept {
        return std::hash<std::string>()(flow.start.name + flow.end.name);
    }
};

namespace mip {
    struct Variables {
        std::unordered_map<Edge, std::unordered_map<Flow, GRBVar>> flowStatus;
        GRBVar scaleFactor;
        std::optional<std::unordered_map<Edge, GRBVar>> enabledEdges;
    };

    struct Constraints {
        GRBConstr scaleFactorConstraint;
        std::unordered_map<Edge, GRBConstr> edgeCapacityConstraints;
        std::unordered_map<Node, std::unordered_map<Flow, GRBConstr>> netFlowRateConstraints;
        std::optional<std::unordered_map<Edge, GRBGenConstr>> enabledEdgesConstraints;
        std::optional<std::unordered_map<std::string, GRBConstr>> conflictEdgesConstraints;
        std::optional<std::unordered_map<std::string, GRBConstr>> synchronousEdgesConstraints;
    };

    struct Model {
        GRBModel model;
        Variables variables;
        Constraints constraints;
    };

    using TrafficPattern = std::unordered_set<Flow>;

    std::string FlowStatusVariableName(const Edge &edge, const Flow &flow) noexcept;

    std::string EdgeCapacityConstraintName(const Edge &edge) noexcept;

    std::string NetFlowRateConstraintName(const Node &node, const Flow &flow) noexcept;

    std::string EnabledEdgesVariableName(const Edge &edge) noexcept;

    std::string EnabledEdgesConstraintName(const Edge &edge) noexcept;

    std::string ConflictEdgesConstraintName(const std::string &name) noexcept;

    std::string SynchronousEdgesConstraintName(const std::string &name) noexcept;

    static const std::string ScaleFactorVariableName = "scale_factor";
    static const std::string ScaleFactorConstraintName = "scale_factor_constraint";

    class Network {
    private:
        std::unordered_map<std::string, Node> nodes;
        std::unordered_map<std::string, Edge> edges;
        std::unordered_set<Edge> reconfigurableEdges;
        std::unordered_map<std::string, std::unordered_set<Edge>> conflictEdges;
        std::unordered_map<std::string, std::unordered_set<Edge>> synchronousEdges;
    public:
        Network() = default;

        Network(std::unordered_map<std::string, Node> nodes, std::unordered_map<std::string, Edge> edges, std::unordered_set<Edge> reconfigurableEdges, std::unordered_map<std::string, std::unordered_set<Edge>> conflictEdges, std::unordered_map<std::string, std::unordered_set<Edge>> synchronousEdges) noexcept;

        const Node &insertNode(const std::string &name);

        const Node &findNode(const std::string &name) const;

        void deleteNode(const Node &node);

        const Edge &insertEdge(const std::string &name, const Node &from, const Node &to, double capacity);

        const Edge &findEdge(const std::string &name) const;

        void deleteEdge(const Edge &edge);

        void defineReconfigurableEdges(const std::unordered_set<Edge> &edges);

        void defineConflictEdges(const std::string &name, const std::unordered_set<Edge> &edges);

        void defineConflictEdges(const std::string &name, std::unordered_set<Edge> &&edges);

        void defineSynchronousEdges(const std::string &name, const std::unordered_set<Edge> &edges);

        void defineSynchronousEdges(const std::string &name, std::unordered_set<Edge> &&edges);

        Model compile(const GRBEnv &env, TrafficPattern trafficPattern, double initialScaleFactor = 1.0, bool optimizeEmptyFlows = true);
    };
}

#endif //NS3_NETWORK_H
