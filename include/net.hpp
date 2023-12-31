#pragma once
#include <algorithm>
#ifndef XSL_NET
#define XSL_NET
#define XSL_NET_ZLIB_SUPPORT

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#ifdef XSL_NET_ZLIB_SUPPORT
#include <zlib.h>
#endif

#include <ranges>
#include <regex>

#ifdef _MSC_VER
#pragma comment(lib, "zlibstatic.lib")
#endif  // _MSC_VER
// #ifdef __GNUC__
// #pragma comment(lib, "z")
// #endif  // _MSC_VER
// #pragma comment(lib, "ssl")
// #pragma comment(lib, "crypto")
#ifdef UNICODE
#define CSTR(str) L##str
#else
#define CSTR(str) str
#endif  // UNICODE
// using string = basic_string<TCHAR>;
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32")
#else
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif  // _WIN32

namespace xsl::net {
  template <class T>
  using Vec = std::vector<T>;
  using String = std::basic_string<char>;
  using String_View = std::basic_string_view<char>;
  using MS2S = std::map<std::string, std::string>;
  using MI2S = std::map<int, String>;
  using MMS2S = std::multimap<String, String>;
  using t4sv = std::tuple<String_View, String_View, String_View, String_View>;
  template <class F>
  using Func = std::function<F>;
  class decompressor {
    z_stream Stream;
    bool Is_valid;
  public:
    decompressor()
      : Stream()
      , Is_valid(inflateInit2(&Stream, MAX_WBITS + 32) == Z_OK) {
    }
    String decompress(const String_View& src) {
      String dest(src.size() * 4, '\0'), buf(1024 * 256, '\0');
      int err = Z_OK;
      Stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(src.data()));
      Stream.avail_in = static_cast<uInt>(src.size());
      while(err != Z_STREAM_END) {
        Stream.next_out = reinterpret_cast<Bytef *>(buf.data());
        Stream.avail_out = static_cast<uInt>(buf.capacity());
        err = inflate(&Stream, Z_NO_FLUSH);
        if(err != Z_OK && err != Z_STREAM_END)
          throw "error zlib";
        dest.append(buf.data(), buf.capacity() - Stream.avail_out);
      }
      return std::move(dest);
    }
    constexpr bool is_valid() {
      return Is_valid;
    }
    ~decompressor() {
      inflateEnd(&Stream);
    }
  };
#ifdef _WIN32
  class httpstartup {
  public:
    WSADATA data;
    bool is_valid;
    httpstartup()
      : is_valid(WSAStartup(0x0202, &data)) {
      // SSL_load_error_strings();
      // OpenSSL_add_all_algorithms();
    }
    ~httpstartup() {
      if(!is_valid)
        WSACleanup();
    }
  };
  static httpstartup httpsu{};
#endif
  // protocol
  // http 0
  // https 1
  inline String percent_encode(const String_View& url) {
    String result;
    result.reserve(url.size());
    for(auto c : url) {
      uint8_t tmp = c;
      if(tmp <= 0X20 || tmp >= 0X7F) {
        result += '%';
        char hex[4];
        auto len = snprintf(hex, sizeof(hex) - 1, "%02X", tmp);
        result.append(hex, len);
      }
      switch(c) {
        case '"' :
          result += "%22";
          break;
        case '%' :
          result += "%25";
          break;
        case '<' :
          result += "%3C";
          break;
        case '>' :
          result += "%3E";
          break;
        case '\\' :
          result += "%5C";
          break;
        case '^' :
          result += "%5E";
          break;
        case '`' :
          result += "%60";
          break;
        case '{' :
          result += "%7B";
          break;
        case '|' :
          result += "%7C";
          break;
        case '}' :
          result += "%7D";
          break;
        default :
          result += c;
      }
    }
    return result;
  }
  constexpr String decode_Transfer_Encoding_chunked(const String_View& buf) {
    String tmp(buf.size(), '\0');
    size_t nstart = 0;
    while(buf[nstart] != '0') {
      auto l = strtol(&buf[nstart], nullptr, 16);
      size_t nend = buf.find("\r\n", nstart + 1);
      tmp.append(buf.data() + nend + 2, l);
      nstart = nend + 2 + l + 2;
    }
    // buf.clear();
    // buf << tmp;
    return std::move(tmp);
  }
#ifdef _WIN32
  class socket {
  protected:
    ADDRINFOA Hints;
    SOCKET Socket;
    constexpr WSABUF to_wbuf(const char *buf, size_t len) {
      return {static_cast<uLong>(len), const_cast<char *>(buf)};
    }
  public:
    socket()
      : Hints()
      , Socket(INVALID_SOCKET){};
    socket(const char *host, const char *port)
      : socket() {
      connect(host, port);
    }
    virtual ~socket() {
      closesocket(Socket);
    }
    virtual void connect(const char *host, const char *port) {
      ADDRINFOA *result{}, *ptr{};
      MSet(&Hints, sizeof(Hints), '\0');
      Hints.ai_family = AF_UNSPEC;
      Hints.ai_socktype = SOCK_STREAM;
      Hints.ai_protocol = IPPROTO_TCP;
      int ec = GetAddrInfoA(host, port, &Hints, &result);
      if(ec != 0)
        return;
      Socket = WSASocket(Hints.ai_family, Hints.ai_socktype, Hints.ai_protocol, nullptr, 0,
        WSA_FLAG_NO_HANDLE_INHERIT | WSA_FLAG_OVERLAPPED);
      if(Socket == INVALID_SOCKET)
        return;
      for(ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        ec = ::connect(Socket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen));
        if(ec == 0)
          break;
      }
      FreeAddrInfoA(result);
      if(ptr == nullptr)
        closesocket(Socket);
      else
        Hints = *ptr;
    }
    virtual void send(const Buffer& buf) {
      WSABUF wbuf = to_wbuf(buf.data(), buf.size());
      DWORD num{};
      if(WSASend(Socket, &wbuf, 1, &num, 0, nullptr, nullptr) != 0)
        throw GetLastError();
    }

    virtual bool recv_able(int_64 timeout) {
      WSAPOLLFD pollfd{Socket, POLLIN};
      auto ret = WSAPoll(&pollfd, 1, timeout);
      if(ret >= 1) {
        if((pollfd.revents & POLLIN) != 0)
          return true;
        else if((pollfd.revents & POLLHUP) != 0)
          return false;
      } else if(ret == 0)
        return false;
      else {
        auto err = GetLastError();
        switch(err) {
          case WSAENETDOWN :
            throw "poll WSAENETDOWN";
          case WSAEFAULT :
            throw "poll WSAEFAULT";
          case WSAEINVAL :
            throw "poll WSAEINVAL";
          case WSAENOBUFS :
            throw "poll WSAENOBUFS";
          default :
            break;
        }
      }
      return false;
    }

    virtual void recv(Buffer& buf) {
      if(buf.full())
        buf.reserve(buf.capacity() << 1);
      WSABUF wbuf = to_wbuf(buf.data() + buf.size(), buf.capacity() - buf.size());
      DWORD num{}, flag{};
      if(WSARecv(Socket, &wbuf, 1, &num, &flag, nullptr, nullptr) != 0)
        throw "error recv";
      buf.resize(buf.size() + num);
    }
    virtual void shutdown() {
      ::shutdown(Socket, SD_BOTH);
    }
    virtual void close() {
      ::closesocket(Socket);
      Socket = INVALID_SOCKET;
    }
  };
#else
  static size_t once_recv_size = 2048;
  class socket {
  protected:
  public:
    typedef int comp_type;
    addrinfo Hints;
    int fd;  //-1
  public:
    socket()
      : Hints()
      , fd(-1){};
    socket(const char *host, const char *port)
      : socket() {
      connect(host, port);
    }
    virtual ~socket() {
      if(fd != -1)
        ::close(fd);
    }
    // virtual int fd() { return sfd; }
    constexpr int to_comp() const {
      return fd;
    }
    virtual bool invalid() {
      return fd == -1;
    }
    virtual void connect(const char *host, const char *port) {
      addrinfo *result{}, *ptr{};
      memset(&Hints, 0, sizeof(Hints));
      Hints.ai_family = AF_UNSPEC;
      Hints.ai_socktype = SOCK_STREAM;
      Hints.ai_protocol = IPPROTO_TCP;
      int ec = getaddrinfo(host, port, &Hints, &result);
      if(ec != 0)
        throw "get addrinfo error";
      for(ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        fd = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(fd == -1)
          continue;
        ec = ::connect(fd, ptr->ai_addr, ptr->ai_addrlen);
        if(ec == 0)
          break;
        ::close(fd);
      }
      freeaddrinfo(result);
      if(ptr == nullptr)
        throw "free addrinfo error";
    }
    virtual void send(const String& buf) {
      if(::write(fd, buf.data(), buf.size()) != buf.size()) {
        int a = errno;
        throw "send error";
        // EBADF
      }
    }
    virtual bool recv_able(std::int64_t timeout) {
      pollfd pfd{};
      pfd.events = POLLIN;
      pfd.fd = fd;
      int n = poll(&pfd, 1, timeout);
      if(n == -1)
        throw "error";
      return n;
    }
    virtual String recv() {
      String buf(once_recv_size, '\0'), ret{};
      ssize_t n = ::read(fd, buf.data(), once_recv_size);
      if(n > 0) {
        std::cout << n << '\n';
        ret.append(buf, 0, n);
      }
      if(n == -1) {
        int a = errno;
        std::cout << errno;
        throw "error";
      }
      return std::move(ret);
    }
    virtual void shutdown() {
      ::shutdown(fd, SHUT_RDWR);
    }
    virtual void close() {
      ::close(fd);
      fd = -1;
    }
  };
#endif
  class ssocket : public socket {
    // 	SSL_CTX* Ctx;
    // 	SSL* Ssl;
    // public:
    // 	using base_type = socket;
    // 	ssocket():socket(), Ctx(SSL_CTX_new(TLS_client_method())) {
    // 	};
    // 	ssocket(const char* host, const char* port):ssocket() {
    // 		connect(host, port);
    // 	}
    // 	~ssocket()override {
    // 		SSL_CTX_free(Ctx);
    // 		if (sfd != -1)
    // 			::close(sfd);
    // 	}
    // 	void connect(const char* host, const char* port)override {
    // 		this->base_type::connect(host, port);
    // 		Ssl = SSL_new(Ctx);
    // 		SSL_set_fd(Ssl, static_cast<int>(sfd));
    // 		SSL_connect(Ssl);
    // 	}
    // 	void send(const String& buf) override {
    // 		int count = SSL_write(Ssl, buf.data(),
    // static_cast<int>(buf.size()));
    // 	}
    // 	void recv(String& buf) override {
    // 		buf.resize(buf.capacity() - 1);
    // 		int count = SSL_read(Ssl, buf.data(), buf.size());
    // 		while (count <= 0) {
    // 			int err = SSL_get_error(Ssl, count);
    // 			switch (err) {
    // 			case SSL_ERROR_ZERO_RETURN:
    // 				return;
    // 			case SSL_ERROR_WANT_READ:count = SSL_read(Ssl,
    // buf.data()
    // + buf.size(), static_cast<int>(buf.capacity() - buf.size())); break;
    // default: 				throw "error ssl recv";
    // 			}
    // 		}
    // 		buf.resize(count);
    // 	}
    // 	void shutdown()override {
    // 		SSL_shutdown(Ssl);
    // 		::shutdown(sfd, SHUT_RDWR);
    // 	}
    // 	void close()override {
    // 		SSL_free(Ssl);
    // 		::close(sfd);
    // 		sfd = -1;
    // 	}
  };
#ifdef XSL_NET_ZLIB_SUPPORT
#define XSL_NET_ZLIB_EXIST(sta) sta
#else
#define XSL_NET_ZLIB_EXIST(sta)
#endif
#define XSL_NET_ACCEPT_ENCODING "" XSL_NET_ZLIB_EXIST("gzip,deflate")

  namespace http {
    void add_default_http_header(MS2S& header) {
      header.try_emplace("Connection", "close");
      // header.try_emplace("Accept", "*/* ");
      header.try_emplace("Accept-Encoding", XSL_NET_ACCEPT_ENCODING);
      // header.try_emplace("User-Agent", "xsl");
    }
    class IOmultiplexing {
    protected:
      int Fd;
      constexpr IOmultiplexing(int fd)
        : Fd(fd) {
      }
    public:
    };
    enum IOM_EVENTS {
      IN = 0x001,
      PRI = 0x002,
      OUT = 0x004,
      RDNORM = 0x040,
      RDBAND = 0x080,
      WRNORM = 0x100,
      WRBAND = 0x200,
      MSG = 0x400,
      ERR = 0x008,
      HUP = 0x010,
      RDHUP = 0x2000,
      EXCLUSIVE = 0x1 << 28,
      WAKEUP = 0x1 << 29,
      ONESHOT = 0x1 << 30,
      ET = 0x1 << 31
    };
    constexpr static std::tuple<IOM_EVENTS, EPOLL_EVENTS> all_event[15] = {
      {       IOM_EVENTS::IN,        EPOLL_EVENTS::EPOLLIN},
      {      IOM_EVENTS::PRI,       EPOLL_EVENTS::EPOLLPRI},
      {      IOM_EVENTS::OUT,       EPOLL_EVENTS::EPOLLOUT},
      {   IOM_EVENTS::RDNORM,    EPOLL_EVENTS::EPOLLRDNORM},
      {   IOM_EVENTS::RDBAND,    EPOLL_EVENTS::EPOLLRDBAND},
      {   IOM_EVENTS::WRNORM,    EPOLL_EVENTS::EPOLLWRNORM},
      {   IOM_EVENTS::WRBAND,    EPOLL_EVENTS::EPOLLWRBAND},
      {      IOM_EVENTS::MSG,       EPOLL_EVENTS::EPOLLMSG},
      {      IOM_EVENTS::ERR,       EPOLL_EVENTS::EPOLLERR},
      {      IOM_EVENTS::HUP,       EPOLL_EVENTS::EPOLLHUP},
      {    IOM_EVENTS::RDHUP,     EPOLL_EVENTS::EPOLLRDHUP},
      {IOM_EVENTS::EXCLUSIVE, EPOLL_EVENTS::EPOLLEXCLUSIVE},
      {   IOM_EVENTS::WAKEUP,    EPOLL_EVENTS::EPOLLWAKEUP},
      {  IOM_EVENTS::ONESHOT,   EPOLL_EVENTS::EPOLLONESHOT},
      {       IOM_EVENTS::ET,        EPOLL_EVENTS::EPOLLET},
    };
    // 0:IOM_EVENTS 1:EPOLL_EVENTS
    template <uint8_t To>
    constexpr uint32_t event_convert0(uint32_t e) {
      uint32_t te{};
      if((e & IOM_EVENTS::IN) > 0)
        te |= get<To>(all_event[0]);
      if((e & IOM_EVENTS::PRI) > 0)
        te |= get<To>(all_event[1]);
      if((e & IOM_EVENTS::OUT) > 0)
        te |= get<To>(all_event[2]);
      if((e & IOM_EVENTS::RDNORM) > 0)
        te |= get<To>(all_event[3]);
      if((e & IOM_EVENTS::RDBAND) > 0)
        te |= get<To>(all_event[4]);
      if((e & IOM_EVENTS::WRNORM) > 0)
        te |= get<To>(all_event[5]);
      if((e & IOM_EVENTS::WRBAND) > 0)
        te |= get<To>(all_event[6]);
      if((e & IOM_EVENTS::MSG) > 0)
        te |= get<To>(all_event[7]);
      if((e & IOM_EVENTS::ERR) > 0)
        te |= get<To>(all_event[8]);
      if((e & IOM_EVENTS::HUP) > 0)
        te |= get<To>(all_event[9]);
      if((e & IOM_EVENTS::RDHUP) > 0)
        te |= get<To>(all_event[10]);
      if((e & IOM_EVENTS::EXCLUSIVE) > 0)
        te |= get<To>(all_event[11]);
      if((e & IOM_EVENTS::WAKEUP) > 0)
        te |= get<To>(all_event[12]);
      if((e & IOM_EVENTS::ONESHOT) > 0)
        te |= get<To>(all_event[13]);
      if((e & IOM_EVENTS::ET) > 0)
        te |= get<To>(all_event[14]);
      return te;
    }
    template <uint8_t To>
    constexpr uint32_t event_convert1(uint32_t e) {
      uint32_t te{};
      if((e & EPOLL_EVENTS::EPOLLIN) > 0)
        te |= get<To>(all_event[0]);
      if((e & EPOLL_EVENTS::EPOLLPRI) > 0)
        te |= get<To>(all_event[1]);
      if((e & EPOLL_EVENTS::EPOLLOUT) > 0)
        te |= get<To>(all_event[2]);
      if((e & EPOLL_EVENTS::EPOLLRDNORM) > 0)
        te |= get<To>(all_event[3]);
      if((e & EPOLL_EVENTS::EPOLLRDBAND) > 0)
        te |= get<To>(all_event[4]);
      if((e & EPOLL_EVENTS::EPOLLWRNORM) > 0)
        te |= get<To>(all_event[5]);
      if((e & EPOLL_EVENTS::EPOLLWRBAND) > 0)
        te |= get<To>(all_event[6]);
      if((e & EPOLL_EVENTS::EPOLLMSG) > 0)
        te |= get<To>(all_event[7]);
      if((e & EPOLL_EVENTS::EPOLLERR) > 0)
        te |= get<To>(all_event[8]);
      if((e & EPOLL_EVENTS::EPOLLHUP) > 0)
        te |= get<To>(all_event[9]);
      if((e & EPOLL_EVENTS::EPOLLRDHUP) > 0)
        te |= get<To>(all_event[10]);
      if((e & EPOLL_EVENTS::EPOLLEXCLUSIVE) > 0)
        te |= get<To>(all_event[11]);
      if((e & EPOLL_EVENTS::EPOLLWAKEUP) > 0)
        te |= get<To>(all_event[12]);
      if((e & EPOLL_EVENTS::EPOLLONESHOT) > 0)
        te |= get<To>(all_event[13]);
      if((e & EPOLL_EVENTS::EPOLLET) > 0)
        te |= get<To>(all_event[14]);
      return te;
    }
    class epoll : public IOmultiplexing {
    public:
      epoll()
        : IOmultiplexing(epoll_create1(0)) {
        if(Fd == -1)
          throw "epoll_create1";
      }
      void add(int lfd, uint32_t events) {
        epoll_event ev{};
        ev.events = event_convert0<1>(events) | EPOLL_EVENTS::EPOLLET;
        ev.data.fd = lfd;
        int ec = epoll_ctl(Fd, EPOLL_CTL_ADD, lfd, &ev);
        if(ec == -1)
          throw "epoll_ctl";
      }
      void refresh(Vec<epoll_event>& ready, int timeout) {
        int n = epoll_pwait(Fd, ready.data(), ready.size(), timeout, NULL);
        if(n == -1)
          throw "epoll_wait";
        ready.resize(n);
      }
    };
    namespace impl_http {

      void assemble_request_line(const char *method, String_View url, String& buf) {
        buf = std::move(buf) + method + ' ' + String(url) + " HTTP/1.1\r\n";
      }
      void assemble_request_header(const MS2S& header, String& buf) {
        for(auto& kv : header)
          buf += kv.first + ": " + kv.second + "\r\n";
        buf.append("\r\n", 2);
      }
      void assemble_request_body(const String& content, String& buf) {
        buf.append(content) += "\r\n";
      }
      void assemble_request_body(const char *content, size_t count, String& buf) {
        buf.append(content, count).append("\r\n", 2);
      }
      namespace Re {
        const static std::regex shp(R"((?:([a-z]+)://)([^:]+)(?::([0-9]+)){0,1})");
        const static std::regex rpl(R"((HTTP/1\.[01]) ([0-9]{3})(?: (.*?)){0,1}\r\n)");
        const static std::regex url("(?:([a-z]+)://)([^:/]+)(?::([0-9]+)){0,1}(.*)");

      }  // namespace Re
      t4sv analysis_url(const char *url) {
        std::cmatch cm{};
        std::regex_search(url, cm, Re::url);
        return {
          String_View{cm[1].first, static_cast<size_t>(cm[1].length())},
          String_View{cm[2].first, static_cast<size_t>(cm[2].length())},
          String_View{cm[3].first, static_cast<size_t>(cm[3].length())},
          String_View{cm[4].first, static_cast<size_t>(cm[4].length())}
        };
      }
      class request {
        // line-0,header-1,body-2,ok-3
      protected:
        request(const String_View& method, const t4sv& shpp, MS2S&& header, const String_View& body,
          const String_View& version)
          : request(method, get<1>(shpp), get<2>(shpp), get<3>(shpp), static_cast<MS2S&&>(header), body,
            get<0>(shpp)[4] == 's', version) {
        }
      public:
        String Method, Host, Port, Path, Version, Body;
        MS2S Header;
        bool Is_ssl;
        request(const String_View& method, const String_View& host, const String_View& port, const String_View& path,
          MS2S header, const String_View& body, bool is_ssl, const String_View& version)
          : Method(method)
          , Host(host)
          , Port(port)
          , Path(path)
          , Header(std::move(header))
          , Body(body)
          , Is_ssl(is_ssl)
          , Version(version) {
          try_add_header("Host", Host + ": " + Port);
        }
        request(const String_View& method, const String_View& url, MS2S header, const String_View& body,
          const String_View& version)
          : request(method, analysis_url(url.data()), static_cast<MS2S&&>(header), body, version) {
        }
        request(request&& ano) = default;
        bool try_add_header(const String_View& k, const String_View& v) {
          return Header.try_emplace(String(k), v).second;
        }
        void ins_or_merge_header(const String_View& k, const String_View& v) {
          auto i = Header.try_emplace(String(k), v);
          if(!i.second)
            i.first->second += ';' + String(v);
        }
        String assemble() {
          auto str = Method + ' ' + Path + ' ' + Version + "\r\n";
          for(auto& kv : Header)
            str += kv.first + ": " + kv.second + "\r\n";
          str.append("\r\n", 2);
          if(!Body.empty())
            str.append(Body.data(), Body.size());
          return std::move(str);
        }
      };
      class response {
        // line-0,header-1,body-2,ok-3
      public:
        String Version = "HTTP/1.1";
        String Description = "OK";
        String Body = "";
        uint16_t Status;
        MS2S Header;
        response()
          : Body(1024 * 8, '\0') {
        }
        response(String&& str)
          : Body(std::move(str)) {
          String_View sv = Body;
          size_t tail = analysis_line(sv);
          sv.remove_prefix(tail);
          tail = analysis_header(sv);
          sv.remove_prefix(tail);
          Body = analysis_body(sv);
        }
        response(response&& ano) = default;
        size_t analysis_line(String_View& buf) {
          std::cmatch cm{};
          if(std::regex_search(buf.data(), cm, Re::rpl)) {
            Version.assign(cm[1].first, cm[1].second);
            Status = std::stoi(cm[2]);
            Description.assign(cm[3].first, cm[3].second);
            return cm.suffix().first - buf.data() + 2;
          }
          return 0;
        }
        size_t analysis_header(String_View& buf) {
          size_t kstart{};  // CRLF
          while(buf[kstart] != '\r') {
            auto str = (const char(*)[10])(const char *)buf.data();
            size_t kend = buf.find(':', kstart + 1);
            size_t vstart = buf.find_first_not_of(' ', kend + 1);
            size_t vend = buf.find("\r\n", vstart);
            auto i = Header.try_emplace(String{buf.data() + kstart, kend - kstart}, buf.data() + vstart, vend - vstart);
            if(!i.second)
              i.first->second.append(";", 1).append(buf.data() + vstart, vend - vstart);
            kstart = vend + 2;
          }
          return kstart + 2;
        }
        String decode(const String_View& buf) {
          auto iter = Header.find(String{"Content-Encoding"});
          if(iter != Header.end()) {
            if(iter->second == "gzip" || iter->second == "deflate") {
              decompressor de{};
              if(!de.is_valid())
                throw "error decompress";
              return std::move(de.decompress(buf));
            }
          }
          return String(buf);
        }
        String analysis_body(const String_View& buf) {
          auto iter = Header.find(String{"Transfer-Encoding"});
          if(iter != Header.end() && iter->second == "chunked") {
            String tmp = decode_Transfer_Encoding_chunked(buf);
            return std::move(decode(tmp));
          } else {
            iter = Header.find(String{"Content-Length"});
            if(iter != Header.end()) {
              return std::move(decode(buf.substr(0, (strtol(iter->second.data(), nullptr, 10)))));
            }
          }
          return String(buf);
        }
      };

    };  // namespace impl_http
    constexpr size_t client_mode_redirect = 0x1;
    template <class IOMM = epoll>
    class client {
      size_t Mode;
      //
      std::map<int, impl_http::response> Ready;
      std::map<int, socket> Wait;
      IOMM Iom;
      Vec<socket> Rest;
      // ADDRINFOT
    public:
      client(size_t mode = 0)
        : Mode(mode) {
      }
      // Scheme,host,port

      int req(const char *method, const char *url, const MS2S& header, const String& body, bool hb = false,
        bool ipe = false /*is percent encoding*/, const String_View& version = "HTTP/1.1") {
        impl_http::request req{method, url, header, body, version};
        // auto path path
        String buf{};
        // return sfd.fd;
      }
      decltype(auto) get(const char *url, MS2S header, bool ipe = false /*is percent encoding*/) {
        return req("GET", url, std::move(header), {}, false, ipe);
      }
      decltype(auto) post(const char *url, MS2S header, const String& body, size_t count, bool ipe = false) {
        return req("GET", url, std::move(header), body, true, ipe);
      }
      const impl_http::response& resp(int id) {
        return Ready.find(id)->second;
      }
      size_t refresh(int timeout) {
        Vec<epoll_event> buf{20};
        buf.resize(10);
        Iom.refresh(buf, timeout);
        size_t n = 0;
        for(auto& e : buf) {
          auto ce = event_convert1<0>(e.events);
          if(ce & IOM_EVENTS::IN) {
            if(ce & IOM_EVENTS::ERR)
              throw "err";
            auto fd = Wait.find(e.data.fd)->second;
            String str(1024, '\0'), buf{};
            do {
              buf = fd.recv();
              str += buf;
            } while(buf.size() != 0 && fd.recv_able(100));
            // auto str = recv(e.data.fd);
            // auto& f = e.data.fd;
            std::cout.write(str.data(), str.size());
            Ready.try_emplace(e.data.fd, std::move(str));
            fd.close();
            Wait.erase(fd.fd);
            ++n;
          }
        }
        return n;
      }
      void close() {
      }
    };
  }  // namespace http
}  // namespace xsl::net
#endif  // !XSL_NET
