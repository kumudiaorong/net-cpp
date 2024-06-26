#pragma once
#ifndef _XSL_NET_HTTP_H_
#  define _XSL_NET_HTTP_H_
#  include "xsl/net/def.h"
#  include "xsl/net/http/component.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/http/server.h"
NET_NAMESPACE_BEGIN
using http::create_static_handler;
using http::DefaultResponse;
using http::HttpHandlerGenerator;
using http::HttpMethod;
using http::HttpParser;
using http::HttpRouter;
using http::HttpServer;
using HttpRequestView = http::RequestView;
using http::HTTP_METHOD_STRINGS;
using http::HttpVersion;
using HttpRouteHandleResult = http::RouteHandleResult;
using HttpRouteResult = http::RouteResult;
using HttpRouteContext = http::RouteContext;
using http::Request;
using http::to_string_view;
using HttpResponsePart = http::ResponsePart;
using http::HttpResponse;
using http::RouteErrorKind;
using http::StaticCreateResult;
using HttpParseError = http::ParseError;
using HttpParseErrorKind = http::ParseErrorKind;
using HttpParseResult = http::ParseResult;
NET_NAMESPACE_END
#endif
