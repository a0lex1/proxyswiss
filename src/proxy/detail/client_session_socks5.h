
#pragma once

#include "proxy/client_session.h"

#include <string>
#include <vector>
#include <stdint.h>

namespace proxy {
namespace detail {

class client_session_socks5: public client_session {
public:
  client_session_socks5(socket& sock, const credentials& creds);

  virtual void write_connect_request(destination dst,
    write_request_handler handler) override;

  virtual void read_connect_response(connect_response& conn_resp,
    read_response_handler handler) override;

private:
  void auth_write_req();
  void auth_write_req_handler(error_code, size_t);
  void auth_read_resp();
  void auth_read_resp_handler(error_code, size_t);
  void auth_write_creds();
  void auth_write_creds_handler(error_code, size_t);
  void auth_read_creds_reply();
  void auth_read_creds_reply_handler(error_code, size_t);

  void conn_write_req();
  void conn_write_req_handler(error_code, size_t);
  void conn_read_resp();
  void conn_read_resp_handler(error_code, size_t);

  void conn_read_resp_addr_ipv4();
  void conn_read_resp_addr_ipv4_handler(error_code, size_t);
  void conn_read_resp_addr_domainname();
  void conn_read_resp_addr_domainname_handler(error_code, size_t);
  void conn_read_resp_addr_domainname_str();
  void conn_read_resp_addr_domainname_str_handler(error_code, size_t);
  void conn_read_resp_addr_ipv6();
  void conn_read_resp_addr_ipv6_handler(error_code, size_t);

  void conn_read_resp_addr_complete(error_code);

  void call_and_clear_handler(write_request_handler&, error_code);

  static connect_response::major_code socks5_rep_to_major_code(uint8_t);

private:
  write_request_handler user_write_req_handler_;
  read_response_handler user_read_resp_handler_;
  destination           user_dst_;
  connect_response*     puser_conn_resp_;

  std::string write_buf_;
  uint8_t auth_read_packet_[2];
  uint8_t conn_read_packet_header_[4];
  std::vector<uint8_t> conn_read_packet_addr_;
};

}}
