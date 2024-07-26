#pragma once
#ifndef XSL_SYS_NET_SOCKET
#  define XSL_SYS_NET_SOCKET
#  include "xsl/feature.h"
#  include "xsl/sys/io/dev.h"
#  include "xsl/sys/net/def.h"

#  include <sys/socket.h>

#  include <cstdint>
#  include <expected>
#  include <string>
SYS_NET_NB

namespace impl {
  class IpBase {
  public:
    IpBase() = default;
    IpBase(const char *ip) : _ip(ip) {}
    IpBase(std::string_view ip) : _ip(ip) {}
    bool operator==(const IpBase &rhs) const { return _ip == rhs._ip; }
    std::string _ip;
  };
  template <uint8_t version>
  class Ip : public IpBase {};
}  // namespace impl
using IpAddr = impl::IpBase;
// ipv4
using IpV4Addr = impl::Ip<4>;
// ipv6
using IpV6Addr = impl::Ip<6>;

class SockAddr {
public:
  constexpr SockAddr() noexcept : _addr(), _len(sizeof(_addr)) {}
  constexpr std::tuple<sockaddr *, socklen_t *> raw() { return {&_addr, &_len}; }

private:
  sockaddr _addr;
  socklen_t _len;
};

// namespace impl {
//   class IpAddrBase {
//   public:
//     IpAddrBase() = default;
//     IpAddrBase(IpAddrBase &&) = default;
//     IpAddrBase &operator=(IpAddrBase &&) = default;
//     IpAddrBase(const IpAddrBase &) = default;
//     IpAddrBase &operator=(const IpAddrBase &) = default;
//     IpAddrBase(const char *ip, const char *port) : ip(ip), port(port) {}
//     IpAddrBase(std::string_view ip, std::string_view port) : ip(ip), port(port) {}
//     IpAddrBase(std::string_view sa4) : ip(), port() {
//       size_t pos = sa4.find(':');
//       if (pos == std::string_view::npos) {
//         ip = sa4;
//         port = "";
//       } else {
//         ip = sa4.substr(0, pos);
//         port = sa4.substr(pos + 1);
//       }
//     }
//     bool operator==(const IpAddrBase &rhs) const { return ip == rhs.ip && port == rhs.port; }
//     std::string to_string() const { return ip + ":" + port; }
//     std::string ip;
//     std::string port;
//   };
//   template <uint8_t version>
//   class IpAddr : public IpAddrBase {};
// }  // namespace impl

// using IpAddr = impl::IpAddrBase;
// // ipv4
// using IpV4Addr = impl::IpAddr<4>;
// // ipv6
// using IpV6Addr = impl::IpAddr<6>;
using Socket = sys::io::Device<feature::In, feature::Out>;
using AsyncSocket = sys::io::AsyncDevice<feature::In, feature::Out>;

SYS_NET_NE
#endif
