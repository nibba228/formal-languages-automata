#include <stdexcept>
#include <variant>
#include <vector>
#include <iostream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <memory>

#include "edge.h"
#include "nfa.h"

void NFA::concat(NFA&& nfa) {
  for (auto& q : finite) {
    q->term = false;
    q->out.emplace_back(nfa.start, '1');
    q = nullptr;
  }
  nfa.start->start = false;
  nfa.start = nullptr;
  finite = std::move(nfa.finite);
}

void NFA::sum(NFA& nfa) {
  NFA q0;
  q0.start->out.emplace_back(start, '1');
  q0.start->out.emplace_back(nfa.start, '1');
  
  NFA qf;
  auto process_term = [&qf](auto& q) {
    q->term = false;
    q->out.emplace_back(qf.start, '1');
    q = nullptr;
  };

  for (auto& q : finite) {
    process_term(q);
  }

  for (auto& q : nfa.finite) {
    process_term(q);
  }

  auto update_start = [&q0](size_t i) {
    std::get<0>(q0.start->out[i].to)->start = false;
  };

  update_start(0);
  update_start(1);

  start->start = false;
  start = q0.start;

  qf.make_finite(qf.start);
  finite = std::move(qf.finite);
}

void NFA::kleene() {
  NFA q0;
  q0.start->out.emplace_back(start, '1');

  for (auto& q : finite) {
    q->term = false;
    q->out.emplace_back(std::weak_ptr<Node>(q0.start), '1');
    q = nullptr;
  }

  NFA qf;
  qf.make_finite(qf.start);
  q0.start->out.emplace_back(qf.start, '1');

  start->start = false;
  start = q0.start;

  finite = std::move(qf.finite);
}

void NFA::add_node(const std::shared_ptr<Node>& from, bool fin, char letter) {
  auto new_node = std::make_shared<Node>(false, fin);
  from->out.emplace_back(new_node, letter);

  if (fin) {
    finite.push_back(std::move(new_node));
  }
}

void NFA::make_finite(std::shared_ptr<Node>& node) {
  node->term = true;
  finite.push_back(node);
}

NFA::NFA(const std::string& s) {
  const std::unordered_set<char> operators = {'.', '+', '*'};
  const decltype(operators) alph = {'a', 'b', 'c', '.', '+', '*'};
  std::vector<NFA> automatas;

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
      NFA& ref = automatas.emplace_back();
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
            NFA nfa_r = std::move(automatas[sz - 1]);
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
  
  if (automatas.size() != 1) {
    throw std::logic_error("incorrect RE! It must be written in reversed Polish notation!");
  }

  start = std::move(automatas[0].start);
  finite = std::move(automatas[0].finite);

  condense_eps_();
  std::unordered_map<std::shared_ptr<Node>, bool> used;
}

template <NodeSmartPointer S>
bool NFA::dfs_(const S& v, std::vector<std::pair<std::shared_ptr<Node>, Edge>>& new_edges,
               std::unordered_map<std::shared_ptr<Node>, bool>& used,
               const std::shared_ptr<Node>& eps) {
  auto node = utils::get_shared_ptr(v);
  used[node] = true;
  bool term = node->term;

  for (auto& e : node->out) {
    auto [us, uw] = utils::get_shared_ptr_from_variant(e.to);

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

void NFA::condense_eps_() {
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
