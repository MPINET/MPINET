//
// Created by Ricardo Evans on 2023/9/5.
//

#include "network.h"

#include <algorithm>
#include <numeric>
#include <ranges>

mip::Node::operator std::string() const noexcept {
    return name;
}

mip::Edge::operator std::string() const noexcept {
    return name;
}

bool mip::Edge::operator==(const Edge &other) const noexcept {
    return name == other.name;
}

double mip::Edge::netCoefficient(const Node &node) const noexcept {
    if (node == start) {
        return -1;
    }
    if (node == end) {
        return 1;
    }
    return 0;
}

mip::Flow::operator std::string() const noexcept {
    return "flow from " + start.name + " to " + end.name + ", rate: " + std::to_string(rate);
}

bool mip::Flow::operator==(const Flow &other) const noexcept {
    return start == other.start && end == other.end;
}

double mip::Flow::netRateAt(const Node &node) const noexcept {
    if (node == start) {
        return -rate;
    }
    if (node == end) {
        return rate;
    }
    return 0;
}

std::string mip::FlowStatusVariableName(const Edge &edge, const Flow &flow) noexcept {
    return "rate of " + static_cast<std::string>(flow) + "at edge: " + static_cast<std::string>(edge);
}

std::string mip::EdgeCapacityConstraintName(const Edge &edge) noexcept {
    return "capacity constraint at " + static_cast<std::string>(edge);
}

std::string mip::NetFlowRateConstraintName(const Node &node, const Flow &flow) noexcept {
    return "net rate constraint of " + static_cast<std::string>(flow) + " at " + static_cast<std::string>(node);
}

std::string mip::EnabledEdgesVariableName(const Edge &edge) noexcept {
    return static_cast<std::string>(edge) + " enabled";
}

std::string mip::EnabledEdgesConstraintName(const Edge &edge) noexcept {
    return "enabled constraint at " + static_cast<std::string>(edge);
}

std::string mip::ConflictEdgesConstraintName(const std::string &name) noexcept {
    return "conflict edges constraint for " + name;
}

std::string mip::SynchronousEdgesConstraintName(const std::string &name) noexcept {
    return "synchronous edges constraint for " + name;
}

mip::Network::Network(std::unordered_map<std::string, Node> nodes, std::unordered_map<std::string, Edge> edges, std::unordered_set<Edge> reconfigurableEdges, std::unordered_map<std::string, std::unordered_set<Edge>> conflictEdges, std::unordered_map<std::string, std::unordered_set<Edge>> synchronousEdges) noexcept: nodes(std::move(nodes)), edges(std::move(edges)), reconfigurableEdges(std::move(reconfigurableEdges)), conflictEdges(std::move(conflictEdges)), synchronousEdges(std::move(synchronousEdges)) {}

const mip::Node &mip::Network::insertNode(const std::string &name) {
    auto node = nodes.find(name);
    if (node == nodes.end()) {
        auto [it, success] = nodes.emplace(name, Node{name});
        return it->second;
    } else {
        throw std::runtime_error("node " + name + " already exists");
    }
}

const mip::Node &mip::Network::findNode(const std::string &name) const {
    auto node = nodes.find(name);
    if (node != nodes.end()) {
        return node->second;
    } else {
        throw std::runtime_error("node " + name + " does not exist");
    }
}

void mip::Network::deleteNode(const Node &node) {
    auto it = nodes.find(node.name);
    if (it != nodes.end()) {
        nodes.erase(it);
    } else {
        throw std::runtime_error("node " + node.name + " does not exist");
    }
}

const mip::Edge &mip::Network::insertEdge(const std::string &name, const Node &from, const Node &to, double capacity) {
    auto edge = edges.find(name);
    if (edge == edges.end()) {
        auto [it, success] = edges.emplace(name, Edge{name, from, to, capacity});
        return it->second;
    } else {
        throw std::runtime_error("edge " + name + " already exists");
    }
}

const mip::Edge &mip::Network::findEdge(const std::string &name) const {
    auto edge = edges.find(name);
    if (edge != edges.end()) {
        return edge->second;
    } else {
        throw std::runtime_error("edge " + name + " does not exist");
    }
}

void mip::Network::deleteEdge(const Edge &edge) {
    auto it = edges.find(edge.name);
    if (it != edges.end()) {
        edges.erase(it);
    } else {
        throw std::runtime_error("edge " + edge.name + " does not exist");
    }
}

void mip::Network::defineReconfigurableEdges(const std::unordered_set<Edge> &_edges) {
    reconfigurableEdges.insert(_edges.begin(), _edges.end());
}

void mip::Network::defineConflictEdges(const std::string &name, const std::unordered_set<Edge> &_edges) {
    conflictEdges[name] = _edges;
}

void mip::Network::defineConflictEdges(const std::string &name, std::unordered_set<Edge> &&_edges) {
    conflictEdges[name] = std::move(_edges);
}

void mip::Network::defineSynchronousEdges(const std::string &name, const std::unordered_set<Edge> &_edges) {
    synchronousEdges[name] = _edges;
}

void mip::Network::defineSynchronousEdges(const std::string &name, std::unordered_set<Edge> &&_edges) {
    synchronousEdges[name] = std::move(_edges);
}

mip::Model mip::Network::compile(const GRBEnv &env, std::unordered_set<Flow> trafficPattern, double initialScaleFactor, bool optimizeEmptyFlows) {
    GRBModel model{env};
    std::unordered_map<Node, std::unordered_set<Edge>> directlyConnectedEdges;
    for (auto &edge: edges | std::ranges::views::values) {
        if (directlyConnectedEdges.find(edge.start) == directlyConnectedEdges.end()) {
            directlyConnectedEdges[edge.start] = {};
        }
        if (directlyConnectedEdges.find(edge.end) == directlyConnectedEdges.end()) {
            directlyConnectedEdges[edge.end] = {};
        }
        directlyConnectedEdges[edge.start].insert(edge);
        directlyConnectedEdges[edge.end].insert(edge);
    }
    std::unordered_map<Edge, std::unordered_map<Flow, GRBVar>> flowStatus;
    for (auto &edge: edges | std::ranges::views::values) {
        flowStatus[edge] = {};
        for (auto &flow: trafficPattern) {
            if (flow.rate != 0 || !optimizeEmptyFlows) {
                flowStatus[edge][flow] = model.addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, FlowStatusVariableName(edge, flow));
            }
        }
    }
    GRBVar scaleFactor = model.addVar(0, GRB_INFINITY, initialScaleFactor, GRB_CONTINUOUS, ScaleFactorVariableName);
    GRBConstr scaleFactorConstraint = model.addConstr(scaleFactor == initialScaleFactor, ScaleFactorConstraintName);
    std::unordered_map<Edge, GRBConstr> edgeCapacityConstraints;
    for (auto &edge: edges | std::ranges::views::values) {
        auto flows = flowStatus[edge] | std::ranges::views::values;
        edgeCapacityConstraints[edge] = model.addConstr(std::accumulate(flows.begin(), flows.end(), GRBLinExpr{}) <= edge.capacity, EdgeCapacityConstraintName(edge));
    }
    std::unordered_map<Node, std::unordered_map<Flow, GRBConstr>> netFlowRateConstraints;
    for (auto &node: nodes | std::ranges::views::values) {
        netFlowRateConstraints[node] = {};
        for (auto &flow: trafficPattern) {
            if (flow.rate != 0 || !optimizeEmptyFlows) {
                GRBLinExpr expression = 0;
                for (auto &edge: directlyConnectedEdges[node]) {
                    if (edge.netCoefficient(node) != 0) {
                        expression += edge.netCoefficient(node) * flowStatus[edge][flow];
                    }
                }
                netFlowRateConstraints[node][flow] = model.addConstr(expression == flow.netRateAt(node) * scaleFactor, NetFlowRateConstraintName(node, flow));
            }
        }
    }
    std::optional<std::unordered_map<Edge, GRBVar>> enabledEdges;
    std::optional<std::unordered_map<Edge, GRBGenConstr>> enabledEdgesConstraints;
    std::optional<std::unordered_map<std::string, GRBConstr>> conflictEdgesConstraints;
    std::optional<std::unordered_map<std::string, GRBConstr>> synchronousEdgesConstraints;
    if (!reconfigurableEdges.empty()) {
        enabledEdges = {};
        enabledEdgesConstraints = {};
        for (auto &edge: reconfigurableEdges) {
            enabledEdges->emplace(edge, model.addVar(0, 1, 0, GRB_BINARY, EnabledEdgesVariableName(edge)));
            auto enabled = enabledEdges->at(edge);
            auto flows = flowStatus[edge] | std::ranges::views::values;
            enabledEdgesConstraints->emplace(edge, model.addGenConstrIndicator(enabled, 0, std::accumulate(flows.begin(), flows.end(), GRBLinExpr{}) == 0, EnabledEdgesConstraintName(edge)));
        }
        if (!conflictEdges.empty()) {
            conflictEdgesConstraints = {};
            for (auto &[name, conflictEdgeSet]: conflictEdges) {
                GRBLinExpr expression = 0;
                for (auto &edge: conflictEdgeSet) {
                    expression += enabledEdges->at(edge);
                }
                conflictEdgesConstraints->emplace(name, model.addConstr(expression <= 1, ConflictEdgesConstraintName(name)));
            }
        }
        if (!synchronousEdgesConstraints->empty()) {
            synchronousEdgesConstraints = {};
            for (auto &[name, synchronousEdgeSet]: synchronousEdges) {
                GRBLinExpr expression = 0;
                for (auto &edge: synchronousEdgeSet) {
                    expression += enabledEdges->at(edge);
                }
                synchronousEdgesConstraints->emplace(name, model.addConstr(expression * (expression - synchronousEdgeSet.size()) == 0, SynchronousEdgesConstraintName(name)));
            }
        }
    }
    return Model{
            std::move(model),
            Variables{
                    std::move(flowStatus),
                    std::move(scaleFactor),
                    std::move(enabledEdges)
            },
            Constraints{
                    std::move(scaleFactorConstraint),
                    std::move(edgeCapacityConstraints),
                    std::move(netFlowRateConstraints),
                    std::move(enabledEdgesConstraints),
                    std::move(conflictEdgesConstraints),
                    std::move(synchronousEdgesConstraints)
            }
    };
}

