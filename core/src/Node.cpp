#include "Node.h"

namespace Nodex::Core {
// Port implementation
Port::Port(std::string_view name, Node* node) : m_name{name}, m_node{node} {}

// Node implementation
Node::Node(std::string_view name, std::string_view label)
    : m_name{name}, m_label{label} {}

std::vector<std::string_view> Node::inputNames() const {
  std::vector<std::string_view> names;
  for (const auto& [name, _] : m_inputs) {
    names.push_back(name);
  }
  return names;
}

std::vector<std::string_view> Node::outputNames() const {
  std::vector<std::string_view> names;
  for (const auto& [name, _] : m_outputs) {
    names.push_back(name);
  }
  return names;
}

// Graph implementation
std::vector<SharedPtr<Node>> Graph::getNodes() const {
  std::vector<SharedPtr<Node>> nodes;
  for (const auto& [_, node] : m_nodes) {
    nodes.push_back(node);
  }
  return nodes;
}

void Graph::removeNode(std::string_view name) {
  auto it = m_nodes.find(name);
  if (it == m_nodes.end())
    throw std::runtime_error("Node not found");

  // Disconnect all ports
  for (const auto& outputName : it->second->outputNames()) {
    auto outPort = it->second->outputPort(outputName);
    outPort->disconnectAll();
  }

  for (const auto& inputName : it->second->inputNames()) {
    auto inPort        = it->second->inputPort(inputName);
    auto connectedPort = inPort->connected();
    if (connectedPort) {
      inPort->disconnect(connectedPort);
    }
  }

  m_nodes.erase(it);

  m_nextNodeID--;
}
} // namespace Nodex::Core
