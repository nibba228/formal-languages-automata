#pragma once

#include <vector>

struct Edge;

struct Node {
  Node() = default;

  Node(bool start, bool term, const std::vector<Edge>& out = {}):
    out(out), start(start), term(term) {}

  std::vector<Edge> out = {};
  bool start = false;
  bool term = false;
};
