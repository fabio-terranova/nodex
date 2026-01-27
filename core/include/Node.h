#include "Core.h"
#include <string>
#include <string_view>

namespace Nodex::Core {
using PortID = std::size_t;
using NodeID = std::size_t;

class Node;
class Graph;

class Port {
public:
  explicit Port(std::string_view name, Node* node)
      : m_name{name}, m_node{node} {}
  virtual ~Port() = default;

  std::string_view name() const { return m_name; }

  void setNode(Node* node) { m_node = node; }

  virtual void connect(Port*) {}
  virtual void disconnect(Port*) {}
  virtual void disconnectAll() {}

  virtual Port* connected() const { return nullptr; }
  virtual bool  connected(Port*) const { return false; }

protected:
  std::string m_name;
  Node*       m_node{};
};

template <typename T>
class InPort;

template <typename T>
class OutPort : public Port {
public:
  OutPort(std::string_view name, Function<T()> cb, Node* node)
      : Port{name, node}, m_cb{std::move(cb)} {}

  const T& value();

  void connect(Port* port) override {
    auto inPort = dynamic_cast<InPort<T>*>(port);
    if (!inPort)
      return;

    port->connect(this);
  };

  void disconnect(Port* port) override { port->disconnect(this); }

  void disconnectAll() {
    for (auto& inPort : m_connectedPorts) {
      inPort->disconnect(this);
    }
  }

  void addConnection(InPort<T>* port) { m_connectedPorts.emplace_back(port); }

  void removeConnection(InPort<T>* port) {
    auto it = std::find(m_connectedPorts.begin(), m_connectedPorts.end(), port);

    if (it != m_connectedPorts.end()) {
      m_connectedPorts.erase(it);
    }
  }

  bool connected(Port* port) const override {
    return std::find(m_connectedPorts.begin(), m_connectedPorts.end(), port) !=
           m_connectedPorts.end();
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
  InPort(std::string_view name, T defaultValue, Node* node)
      : Port{name, node}, m_value{std::move(defaultValue)} {}

  const T& value() const {
    return m_connected ? m_connected->value() : m_value;
  }

  Port* connected() const override { return m_connected; }
  bool  connected(Port* port) const override { return m_connected == port; }

  void connect(Port* port) override {
    auto outPort = dynamic_cast<OutPort<T>*>(port);
    if (!outPort)
      return;

    if (m_connected && m_connected != outPort) {
      m_connected->disconnect(this);
    }

    if (m_connected == outPort)
      return; // already connected

    m_connected = outPort;
    outPort->addConnection(dynamic_cast<InPort<T>*>(this));
  }

  void disconnect(Port* port) override {
    auto outPort = dynamic_cast<OutPort<T>*>(port);
    if (!outPort)
      throw std::runtime_error(
          "InPort can only disconnect from OutPort of same type");

    if (m_connected != outPort)
      throw std::runtime_error("Ports are not connected");

    m_connected = nullptr;
    outPort->removeConnection(dynamic_cast<InPort<T>*>(this));
  }

private:
  OutPort<T>* m_connected{};
  T           m_value{};
};

class Node {
public:
  explicit Node(std::string_view name, std::string_view label)
      : m_name{name}, m_label{label} {}
  virtual ~Node() = default;

  std::string_view name() const { return m_name; }

  template <typename T>
  InPort<T>* addInput(std::string_view name, T defaultValue) {
    auto port =
        std::make_unique<InPort<T>>(name, std::move(defaultValue), this);
    auto ptr = port.get();
    m_inputs.emplace(name, std::move(port));
    return ptr;
  }

  template <typename T>
  OutPort<T>* addOutput(std::string_view name, Function<T()> cb) {
    auto port = std::make_unique<OutPort<T>>(name, std::move(cb), this);
    auto ptr  = port.get();
    m_outputs.emplace(name, std::move(port));
    return ptr;
  }

  template <typename T>
  InPort<T>* input(std::string_view name) {
    return static_cast<InPort<T>*>(m_inputs.at(name).get());
  }

  template <typename T>
  OutPort<T>* output(std::string_view name) {
    return static_cast<OutPort<T>*>(m_outputs.at(name).get());
  }

  Port* inputPort(std::string_view name) { return m_inputs.at(name).get(); }

  Port* outputPort(std::string_view name) { return m_outputs.at(name).get(); }

  template <typename T>
  const T& inputValue(std::string_view name) {
    return input<T>(name)->value();
  }

  template <typename T>
  const T& outputValue(std::string_view name) {
    return output<T>(name)->value();
  }

  // inputNames
  std::vector<std::string_view> inputNames() const {
    std::vector<std::string_view> names;
    for (const auto& [name, _] : m_inputs) {
      names.push_back(name);
    }
    return names;
  }

  // outputNames
  std::vector<std::string_view> outputNames() const {
    std::vector<std::string_view> names;
    for (const auto& [name, _] : m_outputs) {
      names.push_back(name);
    }
    return names;
  }

  Graph* graph() const { return m_graph; }

  void setGraph(Graph* graph) { m_graph = graph; }

  NodeID id() const { return m_id; }

  void setID(NodeID id) { m_id = id; }

  std::string_view label() const { return m_label; }

  virtual void render() {}

protected:
  std::string m_name;
  std::string m_label{"Node"};
  Graph*      m_graph{};

  Map<std::string_view, UniquePtr<Port>> m_inputs;
  Map<std::string_view, UniquePtr<Port>> m_outputs;

  NodeID m_id{};
};

class Graph {
public:
  template <typename T, typename... Args>
  T* createNode(Args&&... args) {
    auto node = std::make_shared<T>(std::forward<Args>(args)...);
    node->setGraph(this);
    node->setID(m_nextNodeID++);
    auto ptr = node.get();
    m_nodes.emplace(node->name(), std::move(node));
    return ptr;
  }

  std::vector<SharedPtr<Node>> getNodes() const {
    std::vector<SharedPtr<Node>> nodes;
    for (const auto& [_, node] : m_nodes) {
      nodes.push_back(node);
    }
    return nodes;
  }

  void removeNode(std::string_view name) {
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

  std::size_t frame() const { return m_frame; }
  void        update() { ++m_frame; }

  NodeID numberOfNodes() const { return m_nextNodeID; }

private:
  UnorderedMap<std::string_view, SharedPtr<Node>> m_nodes;

  std::size_t m_frame{1};
  NodeID      m_nextNodeID{0};
};

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
} // namespace Nodex::Core
