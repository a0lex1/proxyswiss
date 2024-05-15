
#pragma once

#include "proxy/server_session.h"

#include <boost/function.hpp>

#include <string>

namespace proxy {
namespace detail {

class server_session_https: public server_session {
public:
  server_session_https(socket& sock);

  // server interface

  virtual void read_connect_request(
    destination& dst,
    read_request_handler handler) override;

  virtual void write_connect_response(
    const connect_response& conn_resp,
    write_response_handler handler) override;

private:
  enum http_version {
    eHttp10,
    eHttp11
  };

  void read_line(boost::function<void(error_code)>);
  void read_first_line();
  void handle_read_first_line(error_code);
  void read_another_line();
  void handle_read_another_line(error_code);

  static error_code parse_first_line(const std::string&,
    proxy::destination&, http_version&);

  static error_code verify_header_line(const std::string&);

  static bool trim_line(std::string&);

  static bool major_to_http_code(connect_response::major_code,
    unsigned&, std::string&);

  void call_and_clear_handler(read_request_handler&, error_code);

private:
  destination* puser_dst_;
  read_request_handler user_read_req_handler_;
  std::string line_;
  destination parsed_dst_;
  http_version http_ver_;
};

}}
