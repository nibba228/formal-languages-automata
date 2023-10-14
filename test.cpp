#include <cstdint>
#include <gtest/gtest.h>
#include <queue>
#include <variant>

#include "solve.cpp"

auto bfs(const NFA& nfa, size_t limit = SIZE_MAX);

auto bfs(const NFA& nfa, size_t limit) {
  std::queue<std::pair<std::variant<std::shared_ptr<Node>, std::weak_ptr<Node>>, std::string>> q;
  q.push({nfa.start, ""});

  std::unordered_set<std::string> s;

  while (!q.empty()) {
    auto [node, str] = q.front();
    q.pop();

    if ((std::holds_alternative<std::shared_ptr<Node>>(node) &&
        std::get<std::shared_ptr<Node>>(node)->term) ||
        (std::holds_alternative<std::weak_ptr<Node>>(node) &&
        std::get<1>(node).lock()->term)) {
      s.insert(str);
    }
    
    std::shared_ptr<Node> ptr =
      (std::holds_alternative<std::shared_ptr<Node>>(node) ? std::get<0>(node) :
       (std::get<1>(node).expired() ? nullptr : std::get<1>(node).lock()));
    if (ptr) {
      for (auto& e : ptr->out) {
        if (e.letter == '1') {
          q.push({e.to, str});
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

TEST(NFATest, SimpleOnlyConcat) {
  auto nfa1 = NFA("aa.a.a.a.");
  std::unordered_set<std::string> s = bfs(nfa1);
  decltype(s) st = {"aaaaa"};
  EXPECT_EQ(st, s);
  nfa1 = NFA("ab.c.");
  s = bfs(nfa1);
  st = {"abc"};

  EXPECT_EQ(st, s);

  nfa1 = NFA("ab.a.c.a.b.a.");
  s = bfs(nfa1);
  st = {"abacaba"};

  EXPECT_EQ(st, s);
}

TEST(NFATest, ConcatPlus) {
  auto nfa1 = NFA("ab+cb.a.+");
  std::unordered_set<std::string> s = {"a", "b", "cba"};
  auto st = bfs(nfa1);
  ASSERT_EQ(st, s);

  auto test = "aaab.+b+.";
  nfa1 = NFA(test);
  s = {"aa", "aab", "ab"};
  st = bfs(nfa1);
  EXPECT_EQ(st, s);

  for (int i = 0; i < 10; ++i) {
    auto str = Solve(test, 'a', i);
    if (i < 3) {
      EXPECT_EQ("YES", str);
    } else {
      EXPECT_EQ("NO", str);
    }
  }


  test = "aaab.+b+.c.";
  nfa1 = NFA(test);
  s = {"aac", "aabc", "abc"};
  st = bfs(nfa1);
  EXPECT_EQ(st, s);

  for (int i = 0; i < 10; ++i) {
    auto str = Solve(test, 'a', i);
    if (i < 3) {
      EXPECT_EQ("YES", str);
    } else {
      EXPECT_EQ("NO", str);
    }
  }

  test = "ab.a.c.aba.+b+.c.a.";
  nfa1 = NFA(test);
  s = {"abacaca", "abacbaca", "abacbca"};
  st = bfs(nfa1);
  EXPECT_EQ(st, s);

  for (int i = 0; i < 10; ++i) {
    auto str = Solve(test, 'b', i);
    if (i < 2) {
      EXPECT_EQ("YES", str);
    } else {
      EXPECT_EQ("NO", str);
    }
  }
}

TEST(NFATest, Kleene) {
  auto nfa1 = NFA("a*");
  decltype(bfs(std::declval<NFA>())) s = {"", "a", "aa", "aaa"};
  auto st = bfs(nfa1, 3);
  EXPECT_EQ(st, s);

  auto str = Solve("a*", 'b', 0);
  EXPECT_EQ("YES", str);
  str = Solve("a*", 'b', 1);
  EXPECT_EQ(str, "NO");

  for (int i = 0; i < 10; ++i) {
    str = Solve("a*", 'a', i);
    EXPECT_EQ(str, "YES");
  }

  auto test = "bac+*.*";
  nfa1 = NFA(test);
  s = {"", "b", "ba", "baa", "bac", "bca", "bcc", "bc", "bb", "bba", "bbc", "bbb", "bab", "bcb"};
  st = bfs(nfa1, 3);
  EXPECT_EQ(st, s);

  std::vector<char> alp = {'a', 'b', 'c'};

  for (int i = 0; i < 10; ++i) {
    for (char c : alp) {
      str = Solve(test, c, i);
      EXPECT_EQ("YES", str);
    }
  }
}

TEST(NFATest, ErrorTest) {
  using err = std::logic_error;
  EXPECT_THROW(Solve("aaa", 'a', 1), err);
  EXPECT_THROW(Solve("ab..", 'a', 1), err);
  EXPECT_THROW(Solve("ru.", 'a', 1), err);
  EXPECT_THROW(Solve("a.", 'a', 1), err);
  EXPECT_THROW(Solve("a+", 'a', 1), err);
  EXPECT_THROW(Solve("*", 'a', 1), err);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
