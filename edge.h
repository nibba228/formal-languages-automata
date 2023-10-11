#pragma once

#include <memory>
#include <variant>

#include "node.h"

template <typename T> 
concept NodeSmartPointer =
  std::is_same_v<T, std::shared_ptr<Node>> || std::is_same_v<T, std::weak_ptr<Node>>;

struct Edge {
  Edge() = default;

  template <NodeSmartPointer T>
  Edge(const T& to, char letter): to(to), letter(letter) {}

  std::variant<std::shared_ptr<Node>, std::weak_ptr<Node>> to;
  char letter;
};
