#include "dfa.cpp"
#include "edge.h"

template <NodeSmartPointer S>
void emphasis_x(const S& v, std::unordered_map<std::shared_ptr<Node>, bool>& used, char x,
         std::vector<std::shared_ptr<Node>>& x_nodes, int x_count = 0) {
  auto node = utils::get_shared_ptr(v);

  used[node] = true;
  for (auto& e : node->out) {
    auto [us, uw] = utils::get_shared_ptr_from_variant(e.to);
    std::shared_ptr<Node> ptr = (us ? *us : uw->lock());

    if (e.letter == x) {
      if (x_count == 0) {
        x_nodes.push_back(node);
      }
      if (!used[ptr]) {
        emphasis_x(ptr, used, x, x_nodes, x_count + 1);
      }
    } else if (e.letter != '1') {
      if (!used[ptr]) {
        emphasis_x(ptr, used, x, x_nodes, 0);  
      }
    }
  }
}

template <NodeSmartPointer S>
bool dfs(const S& v, std::unordered_map<std::shared_ptr<Node>, bool>& used, char x, int count, int k) {
  auto node = utils::get_shared_ptr(v);
  if (count >= k) {
    return true;
  }

  used[node] = true;
  for (auto& e : node->out) {
    auto [us, uw] = utils::get_shared_ptr_from_variant(e.to);
    std::shared_ptr<Node> ptr = (us ? *us : uw->lock());

    bool flag = false;
    if (e.letter == x) {
      if (used[ptr]) {
        return true;
      }
      flag = dfs(ptr, used, x, count + 1, k);
      if (flag) {
        return true;
      }
    }
  }

  return false;
}

std::string Solve(const std::string& s, char x, int k) {
  if (k == 0) {
    return "YES";
  }

  DFA dfa(s);

  std::vector<std::shared_ptr<Node>> x_nodes;
  std::unordered_map<std::shared_ptr<Node>, bool> used;
  emphasis_x(dfa.start, used, x, x_nodes);
  used.clear();

  bool flag = false;
  for (size_t i = 0; i < x_nodes.size(); ++i) {
    const auto& node = x_nodes[i];
    if (!used[node]) {
      flag = dfs(node, used, x, 0, k);
      if (flag) {
        break;
      }
    }
  }

  if (flag) {
    return "YES";
  } else {
    return "NO";
  }
}
