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

  Edge(Edge&&) = default;

  Edge(const Edge&) = default;

  Edge& operator=(const Edge&) = default;

  std::variant<std::shared_ptr<Node>, std::weak_ptr<Node>> to;
  char letter;
};

namespace utils {
  template <NodeSmartPointer S>
  auto get_shared_ptr(const S& v) {
    std::shared_ptr<Node> node(nullptr);
    if constexpr (std::is_same_v<S, std::shared_ptr<Node>>) {
      node = v;
    } else {
      node = v.lock();
    }

    return node;
  }

  auto get_shared_ptr_from_variant(std::variant<std::shared_ptr<Node>, std::weak_ptr<Node>>& v)
    -> std::pair<std::shared_ptr<Node>*, std::weak_ptr<Node>*> {
    auto us = std::get_if<0>(&v);
    auto uw = std::get_if<1>(&v);

    return {us, uw};
  }
};
