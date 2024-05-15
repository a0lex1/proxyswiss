
#include <ws2tcpip.h>

#include "common/net/ip_address.h"
#include "common/net/inet_ntop.h"
#include "common/net/inet_pton.h"
#include "common/base/str.h"

#include <assert.h>

using namespace std;

namespace common {
namespace net {

int ip_address::family() const {
  switch (type()) {
  case eIPv4:
    return AF_INET;
  case eIPv6:
    return AF_INET6;
  default:
    assert(0);
    return 0;
  }
}

uint32_t ip_address::ipv4() const {
  assert(type() == eIPv4);
  return a4_.S_un.S_addr;
}

in_addr ip_address::to_in_addr() const {
  assert(type() == eIPv4);
  return a4_;
}

const uint8_t* ip_address::ipv6() const {
  assert(type() == eIPv6);
  return a6_.u.Byte;
}

in6_addr ip_address::to_in6_addr() const {
  assert(type() == eIPv6);
  return a6_;
}

array<unsigned char, 16> ip_address::ipv6_array() const {
  const unsigned char* x = a6_.u.Byte;
  return array<unsigned char, 16>({
    x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7],
    x[8], x[9], x[10], x[11], x[12], x[13], x[14], x[15]});
}

void ip_address::set_ipv4(uint32_t v4) {
  type_ = eIPv4;
  a4_.S_un.S_addr = v4;
}

void ip_address::set_ipv4(in_addr v4) {
  set_ipv4(v4.S_un.S_addr);
}

void ip_address::set_ipv6(const uint8_t* v6) {
  type_ = eIPv6;
  memcpy(a6_.u.Byte, v6, 16);
}

void ip_address::set_ipv6(in6_addr v6) {
  set_ipv6(v6.u.Byte);
}

string ip_address::to_sockaddr(uint16_t port) const {
  switch (type()) {
  case eIPv4:
    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr = to_in_addr();
    return string(reinterpret_cast<const char*>(&sa), sizeof(sa));
  case eIPv6:
    sockaddr_in6 sa6;
    RtlZeroMemory(&sa6, sizeof(sa6));
    sa6.sin6_family = AF_INET6;
    sa6.sin6_port = htons(port);
    sa6.sin6_addr = to_in6_addr();
    return string(reinterpret_cast<const char*>(&sa6), sizeof(sa6));
  default:
    assert(0);
    return "";
  }
}

string ip_address::to_string() const {
  return common::wstr_to_str(to_wstring());
}

wstring ip_address::to_wstring() const {
  wchar_t buf[128];
  if (!common::net::inet_ntop(
    family(), unk_, buf, sizeof(buf)/sizeof(buf[0])))
  {
    return L"?";
  }
  return buf;
}

bool ip_address::from_sockaddr(const sockaddr* sa, int sa_size) {
  switch (sa->sa_family) {
  case AF_INET:
    if (sa_size < sizeof(sockaddr_in)) {
      return false;
    }
    const sockaddr_in* sa4;
    sa4 = reinterpret_cast<const sockaddr_in*>(sa);
    set_ipv4(sa4->sin_addr);
    return true;
  case AF_INET6:
    if (sa_size < sizeof(sockaddr_in6)) {
      return false;
    }
    const sockaddr_in6* sa6;
    sa6 = reinterpret_cast<const sockaddr_in6*>(sa);
    set_ipv6(sa6->sin6_addr);
    return true;
  default:
    return false;
  }
}

bool ip_address::from_string(const wstring& str) {
  if (from_string_ipv4(str)) {
    return true;
  }
  return from_string_ipv6(str);
}

bool ip_address::from_string_ipv4(const wstring& str) {
  in_addr x;
  if (1 == common::net::inet_pton(AF_INET, str.c_str(), &x)) {
    a4_ = x;
    type_ = eIPv4;
    return true;
  }
  else {
    return false;
  }
}

bool ip_address::from_string_ipv6(const wstring& str) {
  if (str == L"") {
    // Our inet_pton(AF_INET6) will treat empty string as a correct IP.
    return false;
  }
  in6_addr x;
  if (1 == common::net::inet_pton(AF_INET6, str.c_str(), &x)) {
    a6_ = x;
    type_ = eIPv6;
    return true;
  }
  else {
    return false;
  }
}

bool ip_address::from_string(const string& str) {
  return from_string(common::str_to_wstr(str));
}

bool ip_address::from_string_ipv4(const string& str) {
  return from_string_ipv4(common::str_to_wstr(str));
}

bool ip_address::from_string_ipv6(const string& str) {
  return from_string_ipv6(common::str_to_wstr(str));
}

bool ip_address::operator==(const ip_address& r) const {
  if (this->type_ != r.type_) {
    return false;
  }
  switch (this->type_) {
  case eIPv4:
    return !memcmp(&this->a4_, &r.a4_, sizeof(this->a4_));
  case eIPv6:
    return !memcmp(&this->a6_, &r.a6_, sizeof(this->a6_));
  default:
    assert(0);
    return false;
  }
}

bool ip_address::is_local() const {
  switch (type()) {
  case eIPv4:
    return (ipv4() & 0xFF000000) == 0x7F000000;
  case eIPv6:
    in6_addr a;
    a = to_in6_addr();
    return ((a.s6_addr[0] == 0) && (a.s6_addr[1] == 0)
      && (a.s6_addr[2] == 0) && (a.s6_addr[3] == 0)
      && (a.s6_addr[4] == 0) && (a.s6_addr[5] == 0)
      && (a.s6_addr[6] == 0) && (a.s6_addr[7] == 0)
      && (a.s6_addr[8] == 0) && (a.s6_addr[9] == 0)
      && (a.s6_addr[10] == 0) && (a.s6_addr[11] == 0)
      && (a.s6_addr[12] == 0) && (a.s6_addr[13] == 0)
      && (a.s6_addr[14] == 0) && (a.s6_addr[15] == 1));
  default:
    assert(0);
    return 0;
  }
}

}}
