#include "dfa.cpp"

auto bfs(const DFA& dfa, size_t limit = SIZE_MAX) {
  std::queue<std::pair<std::variant<std::shared_ptr<Node>, std::weak_ptr<Node>>, std::string>> q;
  q.push({dfa.start, ""});

  std::unordered_set<std::string> s;

  while (!q.empty()) {
    auto [node, str] = q.front();
    q.pop();

    if (std::holds_alternative<std::shared_ptr<Node>>(node) &&
        std::get<std::shared_ptr<Node>>(node)->term) {
      s.insert(str);
    }
    
    std::shared_ptr<Node> ptr =
      (std::holds_alternative<std::shared_ptr<Node>>(node) ? std::get<0>(node) :
       (std::get<1>(node).expired() ? nullptr : std::get<1>(node).lock()));
    if (ptr) {
      for (auto& e : ptr->out) {
        if (e.letter == '1') {
          continue;
        } else {
          if (str.size() + 1 <= limit) {
            q.push({e.to, str + e.letter});
          }
        }
      }
    }
  }

  return s;
}

int main() {
  std::string s;
  std::cin >> s;
  DFA dfa(s);

  std::unordered_map<std::shared_ptr<Node>, bool> used;
  auto st = bfs(dfa, 3);

  for (auto s : st) {
    std::cout << s << ' ';
  }
}
