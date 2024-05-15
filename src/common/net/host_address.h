
#pragma once

#include "common/net/ip_address.h"

#include <string>

namespace common {
namespace net {

class host_address {
public:
  enum type_t {
    eIPAddress,
    eHostname
  };

  host_address()
  {
  }

  host_address(ip_address ip_addr)
  {
    set_ip_addr(ip_addr);
  }

  host_address(const std::string& hostname)
  {
    set_hostname(hostname);
  }

  type_t type() const { return type_; }

  ip_address ip_addr() const;
  std::string hostname() const;

  void set_ip_addr(ip_address);
  void set_hostname(const std::string&);

  std::string to_string() const;
  void from_string(const std::string&);

  bool operator==(const host_address&) const;
  bool operator!=(const host_address& r) const {
    return !operator==(r);
  }

private:
  type_t type_;
  ip_address ip_addr_;
  std::string hostname_;
};

}}
