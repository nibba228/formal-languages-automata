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
#include "nfa.h"

void NFA::concat(NFA&& nfa) {
  /*
  for (auto& node : finite) {
    node->term = false;
    for (auto& e : nfa.start->out) {
      node->out.push_back(e);
    }
    node = nullptr;
  }

  nfa.start.reset();
  finite = std::move(finite);
  */

  for (auto q : finite) {
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
  for (auto q : finite) {
    q->term = false;
    q->out.emplace_back(qf.start, '1');
    q = nullptr;
  }

  for (auto q : nfa.finite) {
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

void NFA::kleene() {
  NFA q0;
  q0.start->out.emplace_back(start, '1');

  for (auto q : finite) {
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
    finite.push_back(new_node);
  }
}

void NFA::make_finite(std::shared_ptr<Node> node) {
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
  start = std::move(automatas[0].start);
  finite = std::move(automatas[0].finite);
}
