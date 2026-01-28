#ifndef INCLUDE_SERIALIZER_H_
#define INCLUDE_SERIALIZER_H_

#include "Gui.h"
#include "nlohmann/json_fwd.hpp"
#include <string>

namespace Nodex::Gui {

/**
 * Serialization utilities for node graph persistence.
 * Separates serialization concerns from node classes.
 */
namespace Serializer {

/**
 * Deserializes a JSON string into a node graph.
 * Handles all node types and their parameters.
 *
 * @param jsonString JSON representation of the graph
 * @return Deserialized graph
 * @throws std::runtime_error if JSON is malformed or contains unknown node types
 */
Core::Graph loadFromJson(const std::string& jsonString);

/**
 * Saves a graph to JSON format.
 * This is a thin wrapper around graph.serialize() for API consistency.
 *
 * @param graph The graph to serialize
 * @return JSON representation
 */
inline nlohmann::json saveToJson(const Core::Graph& graph) {
  return graph.serialize();
}

} // namespace Serializer

} // namespace Nodex::Gui

#endif // INCLUDE_SERIALIZER_H_
