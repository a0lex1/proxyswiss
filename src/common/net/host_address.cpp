
#include "common/net/host_address.h"

#include <assert.h>

using namespace std;

namespace common {
namespace net {

common::net::ip_address host_address::ip_addr() const {
  assert(type() == eIPAddress);
  return ip_addr_;
}

string host_address::hostname() const {
  assert(type() == eHostname);
  return hostname_;
}

void host_address::set_ip_addr(ip_address ip_addr) {
  type_ = eIPAddress;
  ip_addr_ = ip_addr;
  hostname_ = "\xff";
}

void host_address::set_hostname(const string& hostname) {
  type_ = eHostname;
  hostname_ = hostname;
  ip_addr_ = ip_address(0xffffffff);
}

string host_address::to_string() const {
  switch (type()) {
  case eIPAddress:
    return ip_addr().to_string();
  case eHostname:
    return hostname();
  default:
    assert(0);
    return "";
  }
}

void host_address::from_string(const string& str) {
  if (ip_addr_.from_string(str)) {
    return;
  }
  hostname_ = str;
}

bool host_address::operator==(const host_address& r) const {
  if (this->type_ != r.type_) {
    return false;
  }
  switch (this->type_) {
  case eIPAddress:
    return ip_addr_ == r.ip_addr_;
  case eHostname:
    return hostname_ == r.hostname_;
  default:
    assert(0);
    return false;
  }
}

}}
