#include "xsl/coro/task.h"
#include "xsl/logctl.h"
#include "xsl/net/sync.h"
#include "xsl/net/transport/resolve.h"
#include "xsl/net/transport/tcp.h"

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <thread>
using namespace xsl::coro;
// there should have a echo server
uint16_t port = 12347;

TEST(bind, create) {
  using namespace xsl::net::transport::tcp;
  using namespace xsl::net::transport;
  using namespace xsl;
  auto res = Resolver<feature::ip<4>, feature::tcp>::resolve(port);
  ASSERT_TRUE(res.has_value());
  auto ai = std::move(res.value());
  auto skt = bind(ai);
  EXPECT_TRUE(skt.has_value());
  EXPECT_NE(skt.value().raw_fd(), 0);
}

int main(int argc, char **argv) {
  xsl::no_log();

  CLI::App app{"TCP Server"};
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}