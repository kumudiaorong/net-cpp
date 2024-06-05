#include "xsl/net/http/component/static.h"
#include "xsl/net/http/proto.h"
#include "xsl/net/http/router.h"
#include "xsl/wheel.h"

#include <spdlog/spdlog.h>
#include <sys/stat.h>

HTTP_HELPER_NAMESPACE_BEGIN
using namespace http;
class FileRouteHandler {
public:
  FileRouteHandler(string&& path);
  ~FileRouteHandler();
  RouteHandleResult operator()(RouteContext& ctx);
  string path;
  ContentType content_type;
};
FileRouteHandler::FileRouteHandler(string&& path)
    : path(path), content_type(content_type::MediaType{}, Charset::UTF_8) {
  if (auto point = path.rfind('.'); point != string::npos) {
    auto ext = string_view(path).substr(point + 1);
    this->content_type.media_type = content_type::MediaType::from_extension(ext);
  }
}

FileRouteHandler::~FileRouteHandler() {}

RouteHandleResult FileRouteHandler::operator()(RouteContext& ctx) {
  (void)ctx;
  struct stat buf;
  int res = stat(this->path.c_str(), &buf);
  if (res == -1) {
    SPDLOG_ERROR("stat failed: {}", strerror(errno));
    return RouteHandleResult(RouteHandleError("stat failed"));
  }
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(), make_unique<TcpSendFile>(xsl::move(this->path)));
  auto resp = make_unique<HttpResponse<TcpSendTasks>>(ResponsePart{200, "OK", HttpVersion::HTTP_1_1},
                                                  xsl::move(tasks));
  resp->part.headers.emplace("Content-Type", to_string(this->content_type));
  return RouteHandleResult{std::move(resp)};
}

class FolderRouteHandler {
public:
  FolderRouteHandler(string&& path);
  ~FolderRouteHandler();
  RouteHandleResult operator()(RouteContext& ctx);
  string path;
};
FolderRouteHandler::FolderRouteHandler(string&& path) : path(path) {}
FolderRouteHandler::~FolderRouteHandler() {}
RouteHandleResult FolderRouteHandler::operator()(RouteContext& ctx) {
  SPDLOG_DEBUG("FolderRouteHandler: {}", ctx.current_path);
  string full_path = this->path;
  full_path.append(ctx.current_path.substr(1));
  struct stat buf;
  int res = stat(full_path.c_str(), &buf);
  if (res == -1) {
    SPDLOG_ERROR("stat failed: path: {} error: {}", full_path, strerror(errno));
    return NOT_FOUND_HANDLER(ctx);
  }
  if (S_ISDIR(buf.st_mode)) {
    SPDLOG_DEBUG("FolderRouteHandler: is dir");
    return RouteHandleResult(NOT_FOUND_HANDLER(ctx));
  }
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(), make_unique<TcpSendFile>(xsl::move(full_path)));
  auto resp = make_unique<HttpResponse<TcpSendTasks>>(ResponsePart{200, "OK", HttpVersion::HTTP_1_1},
                                                  xsl::move(tasks));
  if (auto point = ctx.current_path.rfind('.'); point != string::npos) {
    auto ext = ctx.current_path.substr(point + 1);
    resp->part.headers.emplace(
        "Content-Type",
        to_string(ContentType{content_type::MediaType::from_extension(ext), Charset::UTF_8}));
    SPDLOG_DEBUG("FolderRouteHandler: Content-Type: {}", resp->part.headers["Content-Type"]);
  }
  return RouteHandleResult{std::move(resp)};
}
StaticCreateResult create_static_handler(string&& path) {
  if (path.empty()) {
    return AddRouteError{AddRouteErrorKind::InvalidPath};
  }
  struct stat buf;
  int res = stat(path.c_str(), &buf);
  if (res == -1) {
    SPDLOG_ERROR("stat failed: {}", strerror(errno));
    return AddRouteError{AddRouteErrorKind::InvalidPath};
  }
  if (path.back() == '/' && S_ISDIR(buf.st_mode)) {
    return RouteHandler{FolderRouteHandler{xsl::move(path)}};
  } else if (S_ISREG(buf.st_mode)) {
    return RouteHandler{FileRouteHandler{xsl::move(path)}};
  }
  return AddRouteError{AddRouteErrorKind::InvalidPath};
}

HTTP_HELPER_NAMESPACE_END
