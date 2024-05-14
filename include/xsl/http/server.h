#pragma once

#ifndef _XSL_NET_HTTP_SERVER_H_
#  define _XSL_NET_HTTP_SERVER_H_
#  include "xsl/http/context.h"
#  include "xsl/http/def.h"
#  include "xsl/http/msg.h"
#  include "xsl/http/parse.h"
#  include "xsl/http/router.h"
#  include "xsl/sync/poller.h"
#  include "xsl/transport/tcp/tcp.h"
#  include "xsl/wheel/wheel.h"

#  include <spdlog/spdlog.h>
HTTP_NAMESPACE_BEGIN

template <Router R>
class Handler {
public:
  Handler(wheel::shared_ptr<R> router) : parser{}, router{router}, recv_data{}, send_tasks{} {}
  Handler(Handler&&) = default;
  ~Handler() {}
  TcpHandleConfig init() {
    SPDLOG_TRACE("");
    TcpHandleConfig cfg{};
    cfg.recv_tasks.emplace_front(wheel::make_unique<TcpRecvString>(this->recv_data));
    return cfg;
  }
  TcpHandleState recv([[maybe_unused]] TcpRecvTasks& tasks) {
    SPDLOG_TRACE("");
    if (this->recv_data.empty()) {
      return TcpHandleState{sync::IOM_EVENTS::IN, TcpHandleHint::NONE};
    }
    size_t len = this->recv_data.size();
    auto req = this->parser.parse(recv_data.c_str(), len);
    if (req.is_err()) {
      if (req.unwrap_err().kind == ParseErrorKind::Partial) {
        return TcpHandleState{sync::IOM_EVENTS::IN, TcpHandleHint::NONE};
      } else {
        // TODO: handle request error
        return TcpHandleState{sync::IOM_EVENTS::NONE, TcpHandleHint::NONE};
      }
    }
    auto ctx = Context{Request{this->recv_data.substr(0, len), req.unwrap()}};
    auto rtres = this->router->route(ctx);
    if (rtres.is_err()) {
      // TODO: handle route error
      return TcpHandleState{sync::IOM_EVENTS::NONE, TcpHandleHint::NONE};
    }
    this->recv_data.clear();
    this->send_tasks = wheel::move(rtres.unwrap()->into_send_tasks());
    return TcpHandleState{sync::IOM_EVENTS::OUT, TcpHandleHint::WRITE};
  }
  TcpHandleState send(TcpSendTasks& tasks) {
    SPDLOG_TRACE("");
    if (this->send_tasks.empty()) {
      return TcpHandleState{sync::IOM_EVENTS::NONE, TcpHandleHint::NONE};
    }
    tasks.splice_after(tasks.before_begin(), wheel::move(this->send_tasks));
    return TcpHandleState{sync::IOM_EVENTS::OUT, TcpHandleHint::NONE};
  }

private:
  HttpParser parser;
  wheel::shared_ptr<R> router;
  wheel::string recv_data;
  TcpSendTasks send_tasks;
};

using DefaultHandler = Handler<DefaultRouter>;

template <Router R, TcpHandler H>
class HandlerGenerator {
public:
  HandlerGenerator(wheel::shared_ptr<R> router) : router(router) {}
  H operator()() { return Handler<R>{this->router}; }

private:
  wheel::shared_ptr<R> router;
};

using DefaultHandlerGenerator = HandlerGenerator<DefaultRouter, DefaultHandler>;

using DefaultServer = TcpServer<Handler<DefaultRouter>, DefaultHandlerGenerator>;
using DefaultServerConfig = TcpServerConfig<Handler<DefaultRouter>, DefaultHandlerGenerator>;

HTTP_NAMESPACE_END
#endif
