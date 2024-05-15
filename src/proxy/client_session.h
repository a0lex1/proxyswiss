
#pragma once

#include "proxy/destination.h"
#include "proxy/credentials.h"
#include "proxy/connect_response.h"

#include <boost/asio.hpp>

#include <functional>
#include <map>

namespace proxy {

class client_session {
public:
  enum proxy_type {
    eSocks5
  };

  typedef boost::system::error_code error_code;
  typedef boost::asio::ip::tcp::socket socket;

  // error_code can be one of proxy::error::basic_errors
  typedef std::function<void(error_code)> write_request_handler;
  typedef std::function<void(error_code)> read_response_handler;

  // ---

  // Conditions:
  //  1) no parallel reads and/or writes
  //  2) client_session instance must exist until all handlers are called

  virtual void write_connect_request(destination dst,
    write_request_handler handler) = 0;

  virtual void read_connect_response(connect_response& conn_resp,
    read_response_handler handler) = 0;

protected:
  client_session(socket& sock, const credentials& creds)
    :
    sock_(sock),
    creds_(creds)
  {
  }

protected:
  socket&           sock_;
  credentials       creds_;

public:
  std::string dbglog_uid_; //< Used to track messages in debug log.
};

client_session* create_client_session(
  boost::asio::ip::tcp::socket& sock,
  client_session::proxy_type type,
  const credentials& creds,
  const std::string& dbglog_uid);

void enum_client_session_types(
  std::map<client_session::proxy_type, std::wstring>& type_name_map);

bool client_session_type_from_string(
  const std::wstring& str,
  client_session::proxy_type& pt);

std::wstring client_session_type_to_string(client_session::proxy_type pt);

}
