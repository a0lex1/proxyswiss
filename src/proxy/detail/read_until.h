
#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/function.hpp>

#include <string>

namespace proxy {
namespace detail {

void read_until(
  boost::asio::ip::tcp::socket& sock,
  std::string& buf,
  const std::string& delim,
  size_t max_chars, // can be -1
  boost::function<void(boost::system::error_code)> handler);

}}
