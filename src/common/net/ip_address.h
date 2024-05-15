
#pragma once

#include <windows.h>
#include <inaddr.h>
#include <in6addr.h>

#include <stdint.h>
#include <string>
#include <array>

namespace common {
namespace net {

class ip_address {
public:
  enum type_t {
    eIPv4,
    eIPv6
  };

  ip_address()
  {
    set_ipv4(0xffffffff);
  }

  ip_address(uint32_t v4) { set_ipv4(v4); }
  ip_address(const uint8_t* v6) { set_ipv6(v6); }
  ip_address(in_addr v4) { set_ipv4(v4); }
  ip_address(in6_addr v6) { set_ipv6(v6); }

  bool is_local() const;

  type_t type() const { return type_; }
  int family() const;

  uint32_t ipv4() const;
  in_addr to_in_addr() const;
  const uint8_t* ipv6() const;
  in6_addr to_in6_addr() const;
  std::array<unsigned char, 16> ipv6_array() const;

  void set_ipv4(uint32_t v4);
  void set_ipv6(const uint8_t* v6);
  void set_ipv4(in_addr v4);
  void set_ipv6(in6_addr v6);

  std::string to_sockaddr(uint16_t port) const;

  std::string to_string() const;
  std::wstring to_wstring() const;

  bool from_sockaddr(const sockaddr*, int);

  bool from_string(const std::string&);
  bool from_string_ipv4(const std::string&);
  bool from_string_ipv6(const std::string&);

  bool from_string(const std::wstring&);
  bool from_string_ipv4(const std::wstring&);
  bool from_string_ipv6(const std::wstring&);

  bool operator==(const ip_address&) const;
  bool operator!=(const ip_address& r) const {
    return !operator==(r);
  }

private:
  type_t type_;
  union {
    char unk_[1];
    in_addr a4_;
    in6_addr a6_;
  };
};

}}
