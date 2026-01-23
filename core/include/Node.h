#include <algorithm>
#include <cstddef>
#include <cwchar>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

using PortID = std::size_t;
using NodeID = std::size_t;

template <typename T>
using Function = std::function<T>;

template <typename T>
using UniquePtr = std::unique_ptr<T>;

template <typename T>
using SharedPtr = std::shared_ptr<T>;

template <typename T>
using WeakPtr = std::weak_ptr<T>;

template <typename Key, typename Value>
using Map = std::map<Key, Value>;

template <typename Key, typename Value>
using UnorderedMap = std::unordered_map<Key, Value>;

class Node;
class NodeGraph;

class Port {
public:
  explicit Port(std::string_view name, NodeGraph* graph)
      : m_name{name}, m_graph{graph} {}
  virtual ~Port() = default;

  std::string_view name() const { return m_name; }

  void setGraph(NodeGraph* graph) { m_graph = graph; }

  virtual Port* connected() const { return nullptr; }

protected:
  std::string m_name;
  NodeGraph*  m_graph{};
};

template <typename T>
class OutPort : public Port {
public:
  OutPort(std::string_view name, Function<T()> cb, NodeGraph* graph)
      : Port{name, graph}, m_cb{std::move(cb)} {}

  const T& value();

  void connect(Port* port) { port->connected() = this; }

private:
  Function<T()> m_cb;
  T             m_value{};
  std::size_t   m_lastEvalFrame{};
};

template <typename T>
class InPort : public Port {
public:
  InPort(std::string_view name, T defaultValue, NodeGraph* graph)
      : Port{name, graph}, m_value{std::move(defaultValue)} {}

  const T& value() const {
    return m_connected ? dynamic_cast<OutPort<T>*>(m_connected)->value()
                       : m_value;
  }

  void connect(Port* port) { m_connected = port; }

  Port* connected() const { return m_connected; }

private:
  Port* m_connected{};
  T     m_value{};
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
        std::make_unique<InPort<T>>(name, std::move(defaultValue), m_graph);
    auto ptr = port.get();
    m_inputs.emplace(name, std::move(port));
    return ptr;
  }

  template <typename T>
  OutPort<T>* addOutput(std::string_view name, Function<T()> cb) {
    auto port = std::make_unique<OutPort<T>>(name, std::move(cb), m_graph);
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

  void setGraph(NodeGraph* graph) {
    m_graph = graph;
    for (auto& [_, port] : m_outputs) {
      port.get()->setGraph(graph);
    }
  }

  NodeID id() const { return m_id; }

  void setID(NodeID id) { m_id = id; }

  std::string_view label() const { return m_label; }

  virtual void render() {}

protected:
  std::string m_name;
  std::string m_label{"Node"};
  NodeGraph*  m_graph{};

  Map<std::string_view, UniquePtr<Port>> m_inputs;
  Map<std::string_view, UniquePtr<Port>> m_outputs;

  NodeID m_id{};
};

class NodeGraph {
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

  std::size_t frame() const { return m_frame; }
  void        update() { ++m_frame; }

private:
  UnorderedMap<std::string_view, SharedPtr<Node>> m_nodes;

  std::size_t m_frame{1};
  NodeID      m_nextNodeID{0};
};

template <typename T>
const T& OutPort<T>::value() {
  if (!m_graph)
    throw std::runtime_error("OutPort has no graph");

  if (m_lastEvalFrame != m_graph->frame()) {
    m_lastEvalFrame = m_graph->frame();
    m_value         = m_cb();
  }

  return m_value;
}
