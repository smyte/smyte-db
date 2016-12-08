#include <string>

#include "gtest/gtest.h"
#include "pipeline/DatabaseManager.h"

namespace pipeline {

TEST(DatabaseManagerTest, EscapeKeyStr) {
  // empty string
  std::string out;
  DatabaseManager::escapeKeyStr("", &out);
  EXPECT_TRUE(out.empty());

  // no escape
  out.clear();
  DatabaseManager::escapeKeyStr("abc", &out);
  EXPECT_EQ("abc", out);

  // escape ~
  out.clear();
  DatabaseManager::escapeKeyStr("abc~123", &out);
  EXPECT_EQ("abc%7E123", out);

  // escape with existing string
  out = "foo~";
  DatabaseManager::escapeKeyStr("abc~123", &out);
  EXPECT_EQ("foo~abc%7E123", out);

  // escape multiple characters
  out.clear();
  DatabaseManager::escapeKeyStr("abc~123 def\t789\n", &out);
  EXPECT_EQ("abc%7E123%20def%09789%0A", out);
}

}  // namespace pipeline
