#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include "edge.h"
#include "node.h"

struct NFA {
  NFA(): finite{}, start(std::make_shared<Node>(true, false)) {}

  explicit NFA(const std::string& str);

  NFA(const NFA&) = default;

  NFA(NFA&&) = default;

  NFA& operator=(const NFA&) = default;

  void concat(NFA&& nfa);

  void sum(NFA& nfa);

  void kleene();

  void add_node(const std::shared_ptr<Node>& from, bool fin, char letter = '1');

  void make_finite(std::shared_ptr<Node>& node);

  std::vector<std::shared_ptr<Node>> finite;
  std::shared_ptr<Node> start;

private:
  template <NodeSmartPointer S>
  bool dfs_(const S& v, std::vector<std::pair<std::shared_ptr<Node>, Edge>>& new_edges,
            std::unordered_map<std::shared_ptr<Node>, bool>& used,
            const std::shared_ptr<Node>& eps = nullptr);

  void condense_eps_();
};
