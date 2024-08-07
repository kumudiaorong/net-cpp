#include "xsl/coro/executor.h"

#include <gtest/gtest.h>
using namespace xsl::coro;

TEST(ExecutorTest, NoopExecutor) {
  NoopExecutor executor;
  int value = 0;
  executor.schedule([&value] { value = 1; });
  EXPECT_EQ(value, 1);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
