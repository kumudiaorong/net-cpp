#include "xsl/feature.h"
#include "xsl/logctl.h"
#include "xsl/sys.h"

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
using namespace xsl::coro;
using namespace xsl;
// there should have a echo server
uint16_t port = 12348;

TEST(bind, create) {
  using namespace xsl::net;
  using namespace xsl::feature;
  auto res = Resolver{}.resolve<Tcp<Ip<4>>>(port);
  ASSERT_TRUE(res.has_value());
  auto ai = std::move(res.value());
  auto skt = xsl::sys::tcp::bind(ai);
  ASSERT_TRUE(skt.has_value());
  ASSERT_NE(skt.value().raw(), 0);
}

int main(int argc, char **argv) {
  xsl::no_log();

  CLI::App app{"TCP Server"};
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
