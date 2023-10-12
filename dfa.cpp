#include <stdexcept>
#include <variant>
#include <vector>
#include <queue>
#include <iostream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <memory>

#include "edge.h"
#include "dfa.h"

void DFA::concat(DFA&& nfa) {
  for (auto& q : finite) {
    q->term = false;
    q->out.emplace_back(nfa.start, '1');
    q = nullptr;
  }
  nfa.start->start = false;
  nfa.start = nullptr;
  finite = std::move(nfa.finite);
}

void DFA::sum(DFA& nfa) {
  DFA q0;
  q0.start->out.emplace_back(start, '1');
  q0.start->out.emplace_back(nfa.start, '1');
  
  DFA qf;
  for (auto& q : finite) {
    q->term = false;
    q->out.emplace_back(qf.start, '1');
    q = nullptr;
  }

  for (auto& q : nfa.finite) {
    q->term = false;
    q->out.emplace_back(qf.start, '1');
    q = nullptr;
  }

  std::get<0>(q0.start->out[0].to)->start = false;
  std::get<0>(q0.start->out[1].to)->start = false;

  start->start = false;
  start = q0.start;

  qf.make_finite(qf.start);
  finite = std::move(qf.finite);
}

void DFA::kleene() {
  DFA q0;
  q0.start->out.emplace_back(start, '1');

  for (auto& q : finite) {
    q->term = false;
    q->out.emplace_back(std::weak_ptr<Node>(q0.start), '1');
    q = nullptr;
  }

  DFA qf;
  qf.make_finite(qf.start);
  q0.start->out.emplace_back(qf.start, '1');

  start->start = false;
  start = q0.start;

  finite = std::move(qf.finite);
}

void DFA::add_node(const std::shared_ptr<Node>& from, bool fin, char letter) {
  auto new_node = std::make_shared<Node>(false, fin);
  from->out.emplace_back(new_node, letter);

  if (fin) {
    finite.push_back(std::move(new_node));
  }
}

void DFA::make_finite(std::shared_ptr<Node> node) {
  node->term = true;
  finite.push_back(node);
}

DFA::DFA(const std::string& s) {
  const std::unordered_set<char> operators = {'.', '+', '*'};
  const decltype(operators) alph = {'a', 'b', 'c', '.', '+', '*'};
  std::vector<DFA> automatas;

  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (alph.find(c) == alph.end()) {
      std::string s = "incorrect RE! character "; 
      s += "'" + std::string(1, c) + "' ";
      s += "is not in the alphabet on index";
      s += " " + std::to_string(i);
      s += " (counting starting off with 0 index)";
      throw std::logic_error(s);
    }

    if (operators.find(c) == operators.end()) {
      DFA& ref = automatas.emplace_back();
      ref.add_node(ref.start, true, c);
    } else {
      switch (c) {
        case '.':
          if (automatas.size() < 2) {
            std::string s = "incorrect RE! '.' must have 2 operands on index";
            s += " " + std::to_string(i);
            s += " (counting starting off with 0 index)";
            throw std::logic_error(s);
          }

          {
            size_t sz = automatas.size();
            DFA nfa_r = std::move(automatas[sz - 1]);
            automatas[sz - 2].concat(std::move(nfa_r));
            automatas.erase(automatas.begin() + sz - 1);
          }

          break;
        case '+':
          if (automatas.size() < 2) {
            std::string s = "incorrect RE! '+' must have 2 operands"; 
            s += " " + std::to_string(i);
            s += " (counting starting off with 0 index)";
            throw std::logic_error(s);
          }

          {
            size_t sz = automatas.size();
            automatas[sz - 2].sum(automatas[sz - 1]);
            automatas.erase(automatas.begin() + sz - 1);
          }
          break;

        case '*':
          if (automatas.empty()) {
            std::string s = "incorrect RE! '*' must have 1 operand!";
            s += " " + std::to_string(i);
            s += " (counting starting off with 0 index)";
            throw std::logic_error(s);
          }

          automatas[automatas.size() - 1].kleene();
          break;
      }
    }
  }
  start = std::move(automatas[0].start);
  finite = std::move(automatas[0].finite);

  // Making DFA
  condense_eps_();
  std::unordered_map<std::shared_ptr<Node>, bool> used;
  delete_eps_(start, used);
}

template <NodeSmartPointer S>
bool DFA::dfs_(S& v, std::vector<std::pair<std::shared_ptr<Node>, Edge>>& new_edges,
               std::unordered_map<std::shared_ptr<Node>, bool>& used,
               const std::shared_ptr<Node>& eps) {
  std::shared_ptr<Node> node(nullptr);
  if constexpr (std::is_same_v<S, std::shared_ptr<Node>>) {
    node = v;
  } else {
    node = v.lock();
  }

  used[node] = true;
  bool term = node->term;

  for (auto& e : node->out) {
    auto us = std::get_if<0>(&e.to);
    auto uw = std::get_if<1>(&e.to);

    if (e.letter == '1') {
      std::shared_ptr<Node>& temp = const_cast<std::shared_ptr<Node>&>(eps);
      if (!eps) {
        temp = node;
      }

      if (us) {
        term |= dfs_(*us, new_edges, used, temp);
      } else {
        term |= dfs_(*uw, new_edges, used, temp);
      }
    } else {
      bool is_used = false;
      if (us) {
        is_used = used[*us];
      } else {
        is_used = used[uw->lock()];
      }

      if (is_used) {
        std::weak_ptr<Node> w = (us ? std::weak_ptr<Node>(*us) : *uw);
        new_edges.push_back({eps, Edge(w, e.letter)});
      } else {
        std::shared_ptr<Node> sh = (us ? *us : uw->lock());
        new_edges.push_back({eps, Edge(sh, e.letter)});
        dfs_(sh, new_edges, used);
      }
    }
  }

  bool prev = node->term;
  bool ret = node->term |= term;
  if (!prev && ret) {
    finite.push_back(node);
  }
  used[node] = false;

  return ret;
}

void DFA::condense_eps_() {
  std::vector<std::pair<std::shared_ptr<Node>, Edge>> new_edges;
  std::unordered_map<std::shared_ptr<Node>, bool> used;
  dfs_(start, new_edges, used); 

  for (auto& [node, edge] : new_edges) {
    if (!node) {
      continue;
    }
    node->out.push_back(std::move(edge));
  }
}

template <NodeSmartPointer S>
void DFA::delete_eps_(S& v, std::unordered_map<std::shared_ptr<Node>, bool>& used) {
  std::shared_ptr<Node> node(nullptr);
  if constexpr (std::is_same_v<S, std::shared_ptr<Node>>) {
    node = v;
  } else {
    node = v.lock();
  }

  used[node] = true;
  for (size_t i = 0; i < node->out.size(); ++i) {
    if (node->out[i].letter == '1') {
      node->out.erase(node->out.begin() + i);
      continue;
    }

    auto& e = node->out[i];

    auto us = std::get_if<0>(&e.to);
    auto uw = std::get_if<1>(&e.to);

    bool is_used = false;
    if (us) {
      is_used = used[*us];
    } else {
      is_used = used[uw->lock()];
    }

    if (!is_used) {
      auto sh = (us ? *us : uw->lock());
      delete_eps_(sh, used);
    }
  }
}
