#pragma once
#ifndef XSL_NET_H
#  define XSL_NET_H
#  include "xsl/net/http/conn.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/http/server.h"
#  include "xsl/net/http/service.h"
#  include "xsl/net/io/buffer.h"
#  include "xsl/net/io/splice.h"
#  include "xsl/net/tcp.h"
XSL_NB
namespace net {
  using xsl::_net::io::Block;
  using xsl::_net::io::Buffer;
  using xsl::_net::io::splice;
}  // namespace net
namespace tcp {

  template <class LowerLayer>
  using Server = xsl::_net::TcpServer<LowerLayer>;

  template <class... Flags>
  decltype(auto) dial(const char *host, const char *port, Poller &poller) {
    return xsl::_net::tcp_dial<Flags...>(host, port, poller);
  }

  template <class... Flags>
  decltype(auto) serv(const char *host, const char *port) {
    return xsl::_net::tcp_serv<Flags...>(host, port);
  };
}  // namespace tcp

namespace http {
  using xsl::_net::http::HandleContext;
  using xsl::_net::http::HandleResult;
  using xsl::_net::http::Method;
  using xsl::_net::http::ParseData;
  using xsl::_net::http::Parser;
  using xsl::_net::http::ParseUnit;
  using xsl::_net::http::Request;
  using xsl::_net::http::RequestView;
  using xsl::_net::http::Response;
  using xsl::_net::http::ResponsePart;
  using xsl::_net::http::RouteContext;
  using xsl::_net::http::Router;
  using xsl::_net::http::RouteResult;
  using xsl::_net::http::StaticFileConfig;
  using xsl::_net::http::Status;
  using xsl::_net::http::to_string_view;
  using xsl::_net::http::Version;
}  // namespace http

namespace http1 {
  using xsl::_net::http::make_connection;
  using xsl::_net::http::make_service;
  using xsl::_net::http::Server;
  using xsl::_net::http::Service;
}  // namespace http1

XSL_NE
#endif
