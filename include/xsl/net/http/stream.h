#pragma once
#ifndef XSL_NET_HTTP_STREAM
#  define XSL_NET_HTTP_STREAM
#  include "xsl/coro/task.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/sys/net/io.h"

#  include <cstddef>
#  include <memory>
#  include <utility>
HTTP_NB

class HttpReader {
public:
  HttpReader(sys::io::AsyncReadDevice&& ard)
      : _ard(std::make_shared<sys::io::AsyncReadDevice>(std::move(ard))),
        buffer(),
        parse_len(0),
        parser() {
    buffer.resize(1024);
  }
  HttpReader(HttpReader&&) = default;
  ~HttpReader() {}
  template <class Executor = coro::ExecutorBase>
  coro::Task<std::expected<Request, std::errc>, Executor> recv() {
    this->parse_len = 0;
    while (true) {
      auto [sz, err] = co_await sys::net::immediate_recv<Executor>(
          *this->_ard, std::as_writable_bytes(std::span(buffer)));
      if (err) {
        LOG3("recv error: {}", std::make_error_code(*err).message());
        continue;
      }
      size_t len = buffer.size() - parse_len;
      auto req = this->parser.parse(buffer.c_str() + parse_len, len);
      if (!req) {
        if (req.error() == std::errc::resource_unavailable_try_again) {
          this->parse_len += len;
          continue;
        } else {
          LOG3("parse error: {}", std::make_error_code(req.error()).message());
          continue;
        }
      }
      req->length = len;
      co_return Request{std::exchange(this->buffer, ""), std::move(*req), BodyStream(this->_ard)};
    }
  }

private:
  std::shared_ptr<sys::io::AsyncReadDevice> _ard;
  std::string buffer;
  size_t parse_len;
  HttpParser parser;
};

HTTP_NE
#endif
