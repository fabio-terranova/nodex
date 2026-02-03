#include "Serializer.h"
#include "Constants.h"
#include "nlohmann/json.hpp"
#include <cstddef>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>

namespace Nodex::Gui::Serializer {

/**
 * Factory function type for creating nodes from JSON parameters.
 * Returns the created node, or nullptr if creation failed.
 */
using NodeFactory = std::function<Core::Node*(Core::Graph&, const std::string&,
                                              const nlohmann::json&)>;

Core::Node* createRandom(Core::Graph& graph, const std::string& nodeName,
                         const nlohmann::json& params) {
  int samples = params.contains("samples") ? params["samples"].get<int>()
                                           : Constants::kDefaultSamples;

  return graph.createNode<RandomDataNode>(nodeName, samples);
}

Core::Node* createSine(Core::Graph& graph, const std::string& nodeName,
                       const nlohmann::json& params) {
  using namespace Constants;

  int    samples   = params.contains("samples") ? params["samples"].get<int>()
                                                : kDefaultSamples;
  double frequency = params.contains("frequency")
                         ? params["frequency"].get<double>()
                         : kDefaultFrequency;
  double amplitude = params.contains("amplitude")
                         ? params["amplitude"].get<double>()
                         : kDefaultAmplitude;
  double phase =
      params.contains("phase") ? params["phase"].get<double>() : kDefaultPhase;
  double fs =
      params.contains("fs") ? params["fs"].get<double>() : kDefaultSamplingFreq;
  double offset = params.contains("offset") ? params["offset"].get<double>()
                                            : kDefaultOffset;

  return graph.createNode<SineNode>(nodeName, samples, frequency, amplitude,
                                    phase, fs, offset);
}

Core::Node* createMixer(Core::Graph& graph, const std::string& nodeName,
                        const nlohmann::json& params) {
  std::size_t         inputs = params.contains("inputs")
                                   ? params["inputs"].get<std::size_t>()
                                   : Constants::kNumInputs;
  std::vector<double> gains  = params.contains("gains")
                                   ? params["gains"].get<std::vector<double>>()
                                   : std::vector<double>();

  return graph.createNode<MixerNode>(nodeName, inputs, gains);
}

Core::Node* createFilter(Core::Graph& graph, const std::string& nodeName,
                         const nlohmann::json& params) {
  using namespace Constants;

  auto   mode         = params.contains("filter_mode")
                            ? static_cast<Filter::Mode>(params["filter_mode"].get<int>())
                            : kDefaultFilterMode;
  auto   type         = params.contains("filter_type")
                            ? static_cast<Filter::Type>(params["filter_type"].get<int>())
                            : kDefaultFilterType;
  int    order        = params.contains("filter_order")
                            ? params["filter_order"].get<int>()
                            : kDefaultFilterOrder;
  double cutoffFreq   = params.contains("cutoff_freq")
                            ? params["cutoff_freq"].get<double>()
                            : kDefaultCutoffFreq;
  double samplingFreq = params.contains("sampling_freq")
                            ? params["sampling_freq"].get<double>()
                            : kDefaultSamplingFreq;

  return graph.createNode<FilterNode>(nodeName, mode, type, order, cutoffFreq,
                                      samplingFreq);
}

Core::Node* createViewer(Core::Graph& graph, const std::string& nodeName,
                         [[maybe_unused]] const nlohmann::json& params) {
  return graph.createNode<ViewerNode>(nodeName);
}

Core::Node* createMultiViewer(Core::Graph& graph, const std::string& nodeName,
                              const nlohmann::json& params) {
  std::size_t inputs = params.contains("inputs")
                           ? params["inputs"].get<std::size_t>()
                           : Constants::kNumInputs;

  return graph.createNode<MultiViewerNode>(nodeName, inputs);
}

Core::Node* createCSV(Core::Graph& graph, const std::string& nodeName,
                      const nlohmann::json& params) {
  std::string filePath =
      params.contains("filePath") ? params["filePath"].get<std::string>() : "";
  return graph.createNode<CSVNode>(nodeName, filePath);
}

/**
 * Registry of node factories by type name.
 */
static const std::map<std::string, NodeFactory>& getNodeFactories() {
  using namespace Constants;
  static const std::map<std::string, NodeFactory> factories = {
      {"RandomDataNode", createRandom},
      {      "SineNode",   createSine},
      {     "MixerNode",  createMixer},
      {    "FilterNode", createFilter},
      {    "ViewerNode", createViewer},
      {       "CSVNode",    createCSV},
  };

  return factories;
}

Core::Graph loadFromJson(const std::string& jsonString) {
  Core::Graph graph;

  try {
    nlohmann::json j = nlohmann::json::parse(jsonString);

    if (!j.contains("nodes")) {
      throw std::runtime_error("JSON missing 'nodes' array");
    }

    const auto& factories = getNodeFactories();

    for (const auto& nodeJson : j["nodes"]) {
      if (!nodeJson.contains("type")) {
        throw std::runtime_error("Node missing 'type' field");
      }
      if (!nodeJson.contains("name")) {
        throw std::runtime_error("Node missing 'name' field");
      }

      std::string nodeType = nodeJson["type"].get<std::string>();
      std::string nodeName = nodeJson["name"].get<std::string>();

      auto it = factories.find(nodeType);
      if (it == factories.end()) {
        throw std::runtime_error("Unknown node type: " + nodeType);
      }

      nlohmann::json params = nodeJson.contains("parameters")
                                  ? nodeJson["parameters"]
                                  : nlohmann::json::object();
      it->second(graph, nodeName, params);
    }

    // Second pass: Restore connections between nodes
    for (const auto& nodeJson : j["nodes"]) {
      std::string nodeName   = nodeJson["name"].get<std::string>();
      auto        nodes      = graph.getNodes();
      Core::Node* sourceNode = nullptr;

      // Find the source node
      for (auto& node : nodes) {
        if (node->name() == nodeName) {
          sourceNode = node.get();
          break;
        }
      }

      if (!sourceNode) {
        throw std::runtime_error("Node not found after creation: " + nodeName);
      }

      // Process outputs to restore connections
      if (nodeJson.contains("outputs")) {
        for (const auto& outputJson : nodeJson["outputs"]) {
          if (!outputJson.contains("name") ||
              !outputJson.contains("connections")) {
            continue;
          }

          std::string outputName = outputJson["name"].get<std::string>();
          Core::Port* sourcePort = sourceNode->outputPort(outputName);

          if (!sourcePort) {
            throw std::runtime_error("Output port not found: " + outputName);
          }

          // Connect to all target ports
          for (const auto& connJson : outputJson["connections"]) {
            if (!connJson.contains("node") || !connJson.contains("port")) {
              continue;
            }

            std::string targetNodeName = connJson["node"].get<std::string>();
            std::string targetPortName = connJson["port"].get<std::string>();

            // Find target node
            Core::Node* targetNode = nullptr;
            for (auto& node : nodes) {
              if (node->name() == targetNodeName) {
                targetNode = node.get();
                break;
              }
            }

            if (!targetNode) {
              throw std::runtime_error("Target node not found: " +
                                       targetNodeName);
            }

            Core::Port* targetPort = targetNode->inputPort(targetPortName);
            if (!targetPort) {
              throw std::runtime_error("Target input port not found: " +
                                       targetPortName);
            }

            // Establish connection
            targetPort->connect(sourcePort);
          }
        }
      }
    }

  } catch (const nlohmann::json::exception& e) {
    throw std::runtime_error(std::string("JSON parsing error: ") + e.what());
  }

  return graph;
}

} // namespace Nodex::Gui::Serializer
