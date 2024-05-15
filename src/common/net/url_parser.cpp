
#include <ws2tcpip.h>

#include "common/net/url_parser.h"

#include "common/net/inet_pton.h"
#include "common/base/str.h"

using namespace std;

namespace common {
namespace net {

template class url_parser_T<char>;
template class url_parser_T<wchar_t>;

#define str_to_tstr common::str_to_tstr<T>
#define char_to_tchar common::char_to_tchar<T>

template <typename T>
bool url_parser_T<T>::parse(const StringT& url) {
  StringT _scheme;
  const StringT* _pscheme;
  StringT _username;
  StringT _password;
  const StringT* _pusername;
  const StringT* _ppassword;
  net::host_address _host;
  uint16_t _port;
  const uint16_t* _pport;
  StringT _path;

  size_t x = url.find(str_to_tstr("://"));
  if (x != StringT::npos) {
    if (x == 0) {
      // Disallow empty schemes
      return false;
    }
    _scheme = url.substr(0, x);
    _pscheme = &_scheme;
    x += 3;
  }
  else {
    _pscheme = nullptr;
     x = 0;
  }
  StringT uname_pwd;
  size_t y;
  y = url.find(str_to_tstr("@"), x);
  if (y != StringT::npos) {
    uname_pwd = url.substr(x, y-x);
    x = y+1;
    // parse username/password
    size_t z = uname_pwd.find(str_to_tstr(":"));
    if (z != StringT::npos) {
      _username = uname_pwd.substr(0, z);
      _password = uname_pwd.substr(z+1);
      _pusername = &_username;
      _ppassword = &_password;
    }
    else {
      _username = uname_pwd;
      _pusername = &_username;
      _ppassword = nullptr;
    }
  }
  else {
    _pusername = nullptr;
    _ppassword = nullptr;
  }
  StringT host_raw;
  if (url[x] == char_to_tchar('[')) {
    // ipv6 host
    size_t z = url.find(char_to_tchar(']'), x+1);
    if (z == StringT::npos) {
      return false;
    }
    host_raw = url.substr(x+1, z-(x+1));
    x = z+1;
    net::ip_address ip;
    if (!ip.from_string_ipv6(host_raw)) {
      return false;
    }
    _host.set_ip_addr(ip);
    size_t rest = url.find(char_to_tchar(':'), x);
    if (rest == StringT::npos) {
      rest = url.find(char_to_tchar('/'), x);
    }
    if (rest == StringT::npos) {
      rest = url.length();
    }
    if (x != rest) {
      // unexpected characters right after ipv6 address (like [1::1]A)
      return false;
    }
  }
  else {
    // ipv4 or hostname
    size_t z = url.find(char_to_tchar(':'), x);
    if (z == StringT::npos) {
      z = url.find(char_to_tchar('/'), x);
    }
    if (z == StringT::npos) {
      z = url.length();
      host_raw = url.substr(x);
    }
    else {
      host_raw = url.substr(x, z-x);
    }
    net::ip_address ip;
    if (ip.from_string_ipv4(host_raw)) {
      _host.set_ip_addr(ip);
    }
    else {
      // it's a hostname
      if (host_raw.empty()) {
        return false;
      }
      _host.set_hostname(tstr_to_tstr<char, T>(host_raw));
    }
    x += host_raw.length();
  }

  if (url[x] == char_to_tchar(':')) {
    StringT port_raw;
    size_t z = url.find(char_to_tchar('/'), x);
    if (z != StringT::npos) {
      port_raw = url.substr(x+1, z-(x+1));
      x = z;
    }
    else {
      z = url.length();
      port_raw = url.substr(x+1);
      x = url.length();
    }
    unsigned int  port32;
    if (!common::str_to_uint(port_raw, port32, 10)) {
      return false;
    }
    if (port32 > 0xffff) {
      return false;
    }
    _port = static_cast<uint16_t>(port32);
    _pport = &_port;
  }
  else {
    _port = 0;
    _pport = nullptr;
  }

  if (url[x] == char_to_tchar('/')) {
    _path = url.substr(x);
  }
  else {
    _path = str_to_tstr("");
  }

  // succeeded, copy to data members
  scheme_ = _scheme;
  if (_pscheme) {
    pscheme_ = &scheme_;
  }
  else {
    pscheme_ = nullptr;
  }
  username_ = _username;
  password_ = _password;
  if (_pusername) {
    pusername_ = &username_;
  }
  else {
    pusername_ = nullptr;
  }
  if (_ppassword) {
    ppassword_ = &password_;
  } else {
    ppassword_ = nullptr;
  }
  host_ = _host;
  port_ = _port;
  if (_pport) {
    pport_ = &port_;
  }
  else {
    pport_ = nullptr;
  }
  path_ = _path;

  return true;
}

}}
