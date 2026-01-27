#ifndef INCLUDE_CORE_CORE_H_
#define INCLUDE_CORE_CORE_H_

#include <functional>
#include <map>
#include <memory>
#include <string_view>

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

inline constexpr std::string_view version() { return "0.1.0"; };
} // namespace Nodex::Core

#endif // INCLUDE_CORE_CORE_H_
