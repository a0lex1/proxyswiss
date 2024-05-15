
#pragma once

#include "proxy/server_session.h"

#include <stdint.h>
#include <string>

namespace proxy {
namespace detail {

class server_session_socks5: public server_session {
public:
  server_session_socks5(socket& sock);

  // server interface

  virtual void read_connect_request(
    destination& dst,
    read_request_handler handler) override;

  virtual void write_connect_response(
    const connect_response& conn_resp,
    write_response_handler handler) override;

private:
  void auth_read_req();
  void auth_read_req_handler(error_code, size_t);
  void auth_read_req_methods();
  void auth_read_req_methods_handler(error_code, size_t);
  void auth_write_resp();
  void auth_write_resp_handler(error_code, size_t);

  void conn_read_req();
  void conn_read_req_handler(error_code, size_t);
  void conn_read_req_addr_ipv4();
  void conn_read_req_addr_ipv4_handler(error_code, size_t);
  void conn_read_req_addr_domainname();
  void conn_read_req_addr_domainname_handler(error_code, size_t);
  void conn_read_req_addr_domainname_str();
  void conn_read_req_addr_domainname_str_handler(error_code, size_t);
  void conn_read_req_addr_ipv6();
  void conn_read_req_addr_ipv6_handler(error_code, size_t);

  void write_connect_response_handler(error_code, size_t);

  void call_and_clear_handler(read_request_handler&, error_code);

  static uint8_t major_code_to_rep(connect_response::major_code);

private:
  destination* puser_dst_;
  read_request_handler user_read_req_handler_;
  write_response_handler user_write_resp_handler_;

  uint8_t read_auth_hdr_[2];
  std::vector<uint8_t> read_auth_methods_;
  uint8_t write_auth_resp_[2];
  uint8_t read_conn_hdr_[4];
  std::vector<uint8_t> read_addr_;
  uint8_t write_conn_resp_[10];
};

}}
