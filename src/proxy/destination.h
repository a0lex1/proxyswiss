
#pragma once

#include <boost/asio/ip/tcp.hpp>

#include <string>

namespace proxy {

struct destination {
  std::string                hostname;
  boost::asio::ip::address   ip_address;
  uint16_t                   port;

  // If using_hostname(), |ip_address| is ignored.
  bool using_hostname() const {
    return !hostname.empty();
  }

  std::string to_string() const;
  std::wstring to_wstring() const;

  destination(
    const std::string& _hostname = "",
    boost::asio::ip::address _ip_address = boost::asio::ip::address(),
    uint16_t _port = 0);
};

}
