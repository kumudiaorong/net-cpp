#pragma once
#ifndef XSL_SYS_IO
#  define XSL_SYS_IO
#  include "xsl/ai.h"
#  include "xsl/coro.h"
#  include "xsl/sys/def.h"
#  include "xsl/wheel.h"

#  include <fcntl.h>
#  include <sys/sendfile.h>
XSL_SYS_NB
/**
 * @brief write to socket
 *
 * @tparam Executor default is coro::ExecutorBase
 * @tparam S socket type
 * @param skt socket
 * @param data data
 * @return Task<Result, Executor>
 */
template <class Executor = coro::ExecutorBase, RawDeviceLike S>
Task<Result, Executor> imm_write(S &skt, std::span<const byte> data) {
  std::size_t total = 0;
  while (true) {
    ssize_t n = ::write(skt.raw(), data.data(), data.size());
    if ((n == -1) && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
      co_return {total, {std::errc(errno)}};
    }
    total += n;
    if (static_cast<size_t>(n) == data.size()) {
      co_return {total, std::nullopt};
    }
    if (n > 0) {
      LOG5("[write] write {} bytes", n);
      data = data.subspan(n);
      continue;
    }
    if (!co_await skt.sem()) {
      co_return {total, {std::errc::not_connected}};//TODO: not not_connected
    }
  }
}
/**
 * @brief read from socket
 *
 * @tparam Executor default is coro::ExecutorBase
 * @tparam S socket type
 * @param skt socket
 * @param buf buffer
 * @return Task<Result, Executor>
 */
template <class Executor = coro::ExecutorBase, RawDeviceLike S>
Task<Result, Executor> imm_read(S &skt, std::span<byte> buf) {
  while (true) {
    ssize_t n = ::read(skt.raw(), buf.data(), buf.size());
    if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (!co_await skt.sem()) {
          co_return {0, {std::errc::not_connected}};
        }
        continue;
      } else {
        co_return {0, {std::errc(errno)}};
      }
    } else if (n == 0) {
      co_return {0, {std::errc::no_message}};
    }
    co_return {n, std::nullopt};
  }
}

struct SendfileHint {
  std::string
      path;  ///< file path, must be string, not string_view. Because this function will be called
             ///< in coroutine, and the path may be destroyed before the function is called.
  std::size_t offset;
  std::size_t size;
};
/**
 * @brief send file to socket
 *
 * @tparam Executor default is coro::ExecutorBase
 * @tparam S socket type
 * @param skt socket
 * @param hint sendfile hint
 * @return Task<Result, Executor>
 * @note The skt must keep alive until the task is finished, that is, the task and the socket must
 * have the same lifetime.
 */
template <class Executor = coro::ExecutorBase, RawDeviceLike S>
Task<Result, Executor> imm_sendfile(S &skt, SendfileHint hint) {
  using Result = Result;
  int ffd = open(hint.path.c_str(), O_RDONLY | O_CLOEXEC);
  if (ffd == -1) {
    LOG2("open file failed");
    co_return Result{0, {std::errc(errno)}};
  }
  Defer defer{[ffd] { close(ffd); }};
  off_t offset = hint.offset;
  while (true) {
    ssize_t n = ::sendfile(skt.raw(), ffd, &offset, hint.size);
    if (n == static_cast<ssize_t>(hint.size)) {
      LOG6("{} send {} bytes file", skt.raw(), n);
      co_return Result{static_cast<std::size_t>(offset), std::nullopt};
    }
    if (n > 0) {
      LOG5("[sendfile] send {} bytes", n);
      hint.size -= n;
      continue;
    }
    if ((n == -1) && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
      // TODO: handle sendfile error
      co_return Result{static_cast<std::size_t>(offset), {std::errc(errno)}};
    }

    if (!co_await skt.sem()) {
      co_return Result{static_cast<std::size_t>(offset), {std::errc::not_connected}};
    }
  }
}
template <class Executor = coro::ExecutorBase>
Task<Result, Executor> imm_sendfile(ABW &abw, SendfileHint hint) {
  using Result = Result;
  int ffd = open(hint.path.c_str(), O_RDONLY | O_CLOEXEC);
  if (ffd == -1) {
    LOG2("open file failed");
    co_return Result{0, {std::errc(errno)}};
  }
  Defer defer{[ffd] { close(ffd); }};
  off_t offset = hint.offset;
  std::size_t map_size = hint.size;
  auto pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
  auto pa_size = map_size + (offset - pa_offset);
  auto *src = mmap(nullptr, pa_size, PROT_READ, MAP_PRIVATE, ffd, pa_offset);
  if (src == MAP_FAILED) {
    LOG2("mmap failed");
    co_return Result{0, {std::errc(errno)}};
  }
  Defer defer2{[src, pa_size] { munmap(src, pa_size); }};
  std::span<byte> data{reinterpret_cast<byte *>(src) + (offset - pa_offset), map_size};
  co_return co_await abw.write(data);
}
XSL_SYS_NE
#endif
