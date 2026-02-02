#ifndef INCLUDE_INCLUDE_CORE_H_
#define INCLUDE_INCLUDE_CORE_H_

#include <functional>
#include <map>
#include <memory>

/**
 * @file Core.h
 * @brief Core utilities and type aliases for the Nodex library.
 */
namespace Nodex::Core {
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
} // namespace Nodex::Core

#endif // INCLUDE_INCLUDE_CORE_H_
