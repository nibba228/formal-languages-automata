#include <cstdint>
#include <gtest/gtest.h>
#include <queue>

#include "re_nfa.cpp"

class NFATest : public ::testing::Test {
 protected:
   auto bfs(const NFA& nfa, size_t limit = SIZE_MAX);

  NFA nfa1;
};

auto NFATest::bfs(const NFA& nfa, size_t limit) {
  std::queue<std::pair<std::variant<std::shared_ptr<Node>, std::weak_ptr<Node>>, std::string>> q;
  q.push({nfa.start, ""});

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

TEST_F(NFATest, SimpleOnlyConcat) {
  nfa1 = Solve("ab.c.");
  bool flag = false;
  std::unordered_set<std::string> s = bfs(nfa1);
  decltype(s) st = {"abc"};

  EXPECT_EQ(st, s);

  nfa1 = Solve("ab.a.c.a.b.a.");
  flag = false;
  s = bfs(nfa1);
  st = {"abacaba"};

  EXPECT_EQ(st, s);
}

TEST_F(NFATest, ConcatPlus) {
  nfa1 = Solve("ab+cb.a.+");
  std::unordered_set<std::string> s = {"a", "b", "cba"};
  auto st = bfs(nfa1);
  ASSERT_EQ(st, s);

  nfa1 = Solve("aaab.+b+.");
  s = {"aa", "aab", "ab"};
  st = bfs(nfa1);
  EXPECT_EQ(st, s);

  nfa1 = Solve("aaab.+b+.c.");
  s = {"aac", "aabc", "abc"};
  st = bfs(nfa1);
  EXPECT_EQ(st, s);

  nfa1 = Solve("ab.a.c.aba.+b+.c.a.");
  s = {"abacaca", "abacbaca", "abacbca"};
  st = bfs(nfa1);
  EXPECT_EQ(st, s);
}

TEST_F(NFATest, Kleene) {
  nfa1 = Solve("a*");
  decltype(bfs(std::declval<NFA>())) s = {"", "a", "aa", "aaa"};
  auto st = bfs(nfa1, 3);
  EXPECT_EQ(st, s);

  nfa1 = Solve("bac+*.*");
  s = {"", "b", "ba", "baa", "bac", "bca", "bcc", "bc", "bb", "bba", "bbc", "bbb", "bab", "bcb"};
  st = bfs(nfa1, 3);
  EXPECT_EQ(st, s);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
