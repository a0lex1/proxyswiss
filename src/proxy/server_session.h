
#pragma once

#include "proxy/destination.h"
#include "proxy/connect_response.h"

#include <boost/asio.hpp>

#include <functional>
#include <map>

namespace proxy {

class server_session {
public:
  enum proxy_type {
    eSocks5,
    eHttps
  };

  typedef boost::system::error_code error_code;
  typedef boost::asio::ip::tcp::socket socket;

  // error_code can be one of proxy::error::basic_errors
  typedef std::function<void(error_code)> read_request_handler;
  typedef std::function<void(error_code)> write_response_handler;

  // ---

  virtual void read_connect_request(
    destination& dst,
    read_request_handler handler) = 0;

  virtual void write_connect_response(
    const connect_response& conn_resp,
    write_response_handler handler) = 0;

protected:
  server_session(socket& sock): sock_(sock) {}

protected:
  socket& sock_;

public:
  std::string dbglog_uid_; //< Used to track messages in debug log.
};

server_session* create_server_session(
  boost::asio::ip::tcp::socket& sock, server_session::proxy_type type,
  const std::string& dbglog_uid);

void enum_server_session_types(
  std::map<server_session::proxy_type, std::wstring>& type_name_map);

bool server_session_type_from_string(
  const std::wstring& str,
  server_session::proxy_type& pt);

std::wstring server_session_type_to_string(server_session::proxy_type pt);

}
