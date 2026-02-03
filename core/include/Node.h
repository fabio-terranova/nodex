#ifndef INCLUDE_INCLUDE_NODE_H_
#define INCLUDE_INCLUDE_NODE_H_

#include "Core.h"
#include "nlohmann/json.hpp"
#include <string>
#include <string_view>

/**
 * @file Node.h
 * @brief Node and Graph classes.
 */
namespace Nodex::Core {
using PortID = std::size_t;
using NodeID = std::size_t;

class Node;
class Graph;

class Port {
public:
  Port(std::string_view name, Node* node);
  virtual ~Port() = default;

  std::string_view name() const { return m_name; }

  void  setNode(Node* node) { m_node = node; }
  Node* node() const { return m_node; }

  virtual void connect(Port*) {}
  virtual void disconnect(Port*) {}
  virtual void disconnectAll() {}

  virtual Port* connected() const { return nullptr; }
  virtual bool  connected(Port*) const { return false; }

  virtual nlohmann::json serialize() const {
    nlohmann::json j;
    j["name"] = m_name;

    return j;
  }

protected:
  std::string m_name;
  Node*       m_node{};
};

template <typename T>
class InPort;

template <typename T>
class OutPort : public Port {
public:
  OutPort(std::string_view name, Function<T()> cb, Node* node);

  const T& value();

  void connect(Port* port) override;
  void disconnect(Port* port) override { port->disconnect(this); }
  void disconnectAll() override;

  void addConnection(InPort<T>* port) { m_connectedPorts.emplace_back(port); }
  void removeConnection(InPort<T>* port);

  bool connected(Port* port) const override;

  nlohmann::json serialize() const override {
    nlohmann::json j = Port::serialize();
    // Serialize connected ports Node -> Port names
    j["connections"] = nlohmann::json::array();
    for (const auto& inPort : m_connectedPorts) {
      nlohmann::json conn;
      conn["node"] = inPort->node()->name();
      conn["port"] = inPort->name();
      j["connections"].push_back(conn);
    }

    return j;
  }

private:
  Function<T()>           m_cb;
  T                       m_value{};
  std::vector<InPort<T>*> m_connectedPorts{};
  std::size_t             m_lastEvalFrame{0};
};

template <typename T>
class InPort : public Port {
public:
  InPort(std::string_view name, T defaultValue, Node* node);

  const T& value() const;

  void connect(Port* port) override;
  void disconnect(Port* port) override;

  Port* connected() const override { return m_connected; }
  bool  connected(Port* port) const override { return m_connected == port; }

  nlohmann::json serialize() const override {
    nlohmann::json j = Port::serialize();
    if (m_connected) {
      j["connection"]["node"] = m_connected->node()->name();
      j["connection"]["port"] = m_connected->name();
    }

    return j;
  }

private:
  OutPort<T>* m_connected{};
  T           m_value{};
};

class Node {
public:
  Node(std::string_view name, std::string_view label);
  virtual ~Node() = default;

  std::string_view name() const { return m_name; }

  template <typename T>
  InPort<T>* addInput(std::string_view name, T defaultValue);

  template <typename T>
  OutPort<T>* addOutput(std::string_view name, Function<T()> cb);

  template <typename T>
  InPort<T>* input(std::string_view name);

  template <typename T>
  OutPort<T>* output(std::string_view name);

  Port* inputPort(std::string_view name) {
    return m_inputs.at(std::string{name}).get();
  }
  Port* outputPort(std::string_view name) {
    return m_outputs.at(std::string{name}).get();
  }

  template <typename T>
  const T& inputValue(std::string_view name);

  template <typename T>
  const T& outputValue(std::string_view name);

  std::vector<std::string_view> inputNames() const;
  std::vector<std::string_view> outputNames() const;

  Graph* graph() const { return m_graph; }
  void   setGraph(Graph* graph) { m_graph = graph; }

  NodeID id() const { return m_id; }
  void   setID(NodeID id) { m_id = id; }

  std::string_view label() const { return m_label; }

  virtual void render() {}

  virtual nlohmann::json serialize() const {
    nlohmann::json j{};
    j["name"]  = m_name;
    j["label"] = m_label;
    j["id"]    = m_id;

    for (const auto& [name, port] : m_inputs) {
      j["inputs"].push_back(port->serialize());
    }
    for (const auto& [name, port] : m_outputs) {
      j["outputs"].push_back(port->serialize());
    }

    return j;
  }

protected:
  std::string m_name;
  std::string m_label{"Node"};
  Graph*      m_graph{};

  Map<std::string, UniquePtr<Port>> m_inputs;
  Map<std::string, UniquePtr<Port>> m_outputs;

  NodeID m_id{};
};

class Graph {
public:
  template <typename T, typename... Args>
  T*   createNode(Args&&... args);
  void removeNode(std::string_view name);

  std::vector<SharedPtr<Node>>                    getNodes() const;
  UnorderedMap<std::string_view, SharedPtr<Node>> getNodesMap() const;

  std::size_t frame() const { return m_frame; }
  void        update() { ++m_frame; }
  void        clear() {
    m_nodes.clear();
    m_nextNodeID = 0;
  }

  NodeID numberOfNodes() const { return m_nextNodeID; }

  nlohmann::json serialize() const;
  
  void connect(Port* outputPort, Port* inputPort);

private:
  UnorderedMap<std::string_view, SharedPtr<Node>> m_nodes;

  std::size_t m_frame{1};
  NodeID      m_nextNodeID{0};
};

/////////////////////
// Implementations //
/////////////////////

// OutPort implementation
template <typename T>
OutPort<T>::OutPort(std::string_view name, Function<T()> cb, Node* node)
    : Port{name, node}, m_cb{std::move(cb)} {}

template <typename T>
void OutPort<T>::connect(Port* port) {
  auto inPort = dynamic_cast<InPort<T>*>(port);
  if (!inPort)
    return;

  port->connect(this);
}

template <typename T>
void OutPort<T>::disconnectAll() {
  for (auto& inPort : m_connectedPorts) {
    inPort->disconnect(this);
  }
}

template <typename T>
void OutPort<T>::removeConnection(InPort<T>* port) {
  auto it = std::ranges::find(m_connectedPorts, port);

  if (it != m_connectedPorts.end()) {
    m_connectedPorts.erase(it);
  }
}

template <typename T>
bool OutPort<T>::connected(Port* port) const {
  return std::ranges::find(m_connectedPorts, port) != m_connectedPorts.end();
}

template <typename T>
const T& OutPort<T>::value() {
  Graph* graph = m_node ? m_node->graph() : nullptr;

  if (!graph)
    throw std::runtime_error("OutPort has no graph");

  if (m_lastEvalFrame != graph->frame()) {
    m_lastEvalFrame = graph->frame();
    m_value         = m_cb();
  }

  return m_value;
}

// InPort implementation
template <typename T>
InPort<T>::InPort(std::string_view name, T defaultValue, Node* node)
    : Port{name, node}, m_value{std::move(defaultValue)} {}

template <typename T>
const T& InPort<T>::value() const {
  return m_connected ? m_connected->value() : m_value;
}

template <typename T>
void InPort<T>::connect(Port* port) {
  auto outPort = dynamic_cast<OutPort<T>*>(port);
  if (!outPort)
    return;

  if (m_connected && m_connected != outPort) {
    m_connected->disconnect(this);
  }

  if (m_connected == outPort) // already connected
    return;

  m_connected = outPort;
  outPort->addConnection(dynamic_cast<InPort<T>*>(this));
}

template <typename T>
void InPort<T>::disconnect(Port* port) {
  auto outPort = dynamic_cast<OutPort<T>*>(port);
  if (!outPort)
    throw std::runtime_error(
        "InPort can only disconnect from OutPort of same type");

  if (m_connected != outPort)
    throw std::runtime_error("Ports are not connected");

  m_connected = nullptr;
  outPort->removeConnection(dynamic_cast<InPort<T>*>(this));
}

// Node implementation
template <typename T>
InPort<T>* Node::addInput(std::string_view name, T defaultValue) {
  auto port = std::make_unique<InPort<T>>(name, std::move(defaultValue), this);
  auto ptr  = port.get();
  m_inputs.emplace(std::string{name}, std::move(port));
  return ptr;
}

template <typename T>
OutPort<T>* Node::addOutput(std::string_view name, Function<T()> cb) {
  auto port = std::make_unique<OutPort<T>>(name, std::move(cb), this);
  auto ptr  = port.get();
  m_outputs.emplace(std::string{name}, std::move(port));
  return ptr;
}

template <typename T>
InPort<T>* Node::input(std::string_view name) {
  return dynamic_cast<InPort<T>*>(m_inputs.at(std::string{name}).get());
}

template <typename T>
OutPort<T>* Node::output(std::string_view name) {
  return dynamic_cast<OutPort<T>*>(m_outputs.at(std::string{name}).get());
}

template <typename T>
const T& Node::inputValue(std::string_view name) {
  return input<T>(name)->value();
}

template <typename T>
const T& Node::outputValue(std::string_view name) {
  return output<T>(name)->value();
}

// Graph implementation
template <typename T, typename... Args>
T* Graph::createNode(Args&&... args) {
  auto node = std::make_shared<T>(std::forward<Args>(args)...);
  node->setGraph(this);
  node->setID(m_nextNodeID++);
  auto ptr = node.get();
  m_nodes.emplace(node->name(), std::move(node));
  return ptr;
}
} // namespace Nodex::Core

#endif // INCLUDE_INCLUDE_NODE_H_
