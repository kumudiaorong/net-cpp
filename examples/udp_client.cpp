/**
 * @file udp_client.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/feature.h"

#include <CLI/CLI.hpp>
#include <xsl/coro.h>
#include <xsl/logctl.h>
#include <xsl/net.h>
#include <xsl/sys.h>
#include <xsl/wheel.h>

#include <span>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::feature;
using namespace xsl::coro;
using namespace xsl;



template <class Executor = ExecutorBase>
Lazy<void, Executor> talk(std::string_view ip, std::string_view port,
                          std::shared_ptr<xsl::Poller> poller) {
  std::string buffer(4096, '\0');
  auto [r, w] = udp::dial<Ip<4>>(ip.data(), port.data()).value().async(*poller).split();
  while (true) {
    sys::net::SockAddr<> dst;
    auto [n_recv, err_recv]
        = co_await sys::net::imm_recv(r, xsl::as_writable_bytes(std::span(buffer)));
    if (err_recv.has_value()) {
      LOG2("Failed to recv data, err : {}", std::make_error_code(err_recv.value()).message());
      break;
    }
    LOG4("Recv: {}", std::string_view{buffer.data(), n_recv});
    // std::cin >> buffer;
    LOG5("Input: {} bytes", buffer.size());
    auto [n, err] = co_await sys::net::imm_send(w, xsl::as_bytes(std::span(buffer)));
    if (err.has_value()) {
      LOG2("Failed to send data, err : {}", std::make_error_code(err.value()).message());
      break;
    }
    LOG4("Sent: {}", std::string_view{buffer.data(), n});
  }
  poller->shutdown();
  co_return;
}

int main(int argc, char *argv[]) {
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  auto poller = std::make_shared<xsl::Poller>();
  auto executor = std::make_shared<NewThreadExecutor>();
  talk<NewThreadExecutor>(ip, port, poller).detach(std::move(executor));
  // echo(ip, port, poller).detach();
  while (true) {
    poller->poll();
  }
  return 0;
}