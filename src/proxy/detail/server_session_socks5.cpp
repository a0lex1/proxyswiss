
#include "proxy/detail/server_session_socks5.h"
#include "proxy/error.h"

#include <boost/bind/bind.hpp>
#include <boost/array.hpp>

#include <iostream>
#include <assert.h>

// https://www.ietf.org/rfc/rfc1928.txt

using namespace std;
using namespace boost::placeholders;

namespace proxy {
namespace detail {

static const boost::system::error_code kNoError;

server_session_socks5::server_session_socks5(socket& sock)
  :
  server_session(sock), read_addr_(270), puser_dst_(nullptr)
{
}

void server_session_socks5::read_connect_request(
  destination& dst,
  read_request_handler handler)
{
  assert(!user_read_req_handler_);
  assert(nullptr == puser_dst_);

  puser_dst_ = &dst;
  user_read_req_handler_ = handler;

  auth_read_req();
}

void server_session_socks5::auth_read_req() {
  boost::asio::async_read(
    sock_,
    boost::asio::buffer(read_auth_hdr_, 2),
    boost::asio::transfer_exactly(2),
    boost::bind(&server_session_socks5::auth_read_req_handler, this,
      _1, _2));
}

void server_session_socks5::auth_read_req_handler(error_code err,
  size_t num_bytes)
{
  if (err) {
    call_and_clear_handler(user_read_req_handler_, err);
    return;
  }

  if (read_auth_hdr_[0] != 5) { // VER == 5
    call_and_clear_handler(
      user_read_req_handler_,
      proxy::error::make_error_code(proxy::error::protocol_violation));
    return;
  }

  if (read_auth_hdr_[1] == 0) { // NMETHODS != 1
    call_and_clear_handler(user_read_req_handler_,
      proxy::error::make_error_code(proxy::error::protocol_violation));
    return;
  }

  auth_read_req_methods();
}

void server_session_socks5::auth_read_req_methods() {
  uint8_t nmethods = read_auth_hdr_[1];

  read_auth_methods_ = vector<uint8_t>(nmethods);

  boost::asio::async_read(
    sock_,
    boost::asio::buffer(read_auth_methods_, nmethods),
    boost::asio::transfer_exactly(nmethods),
    boost::bind(&server_session_socks5::auth_read_req_methods_handler,
      this, _1, _2));
}

void server_session_socks5::auth_read_req_methods_handler(
  error_code err, size_t num_bytes)
{
  // Move |read_auth_methods_| from heap to stack.
  vector<uint8_t> methods_copy(read_auth_methods_);
  read_auth_methods_.clear();

  if (err) {
    call_and_clear_handler(user_read_req_handler_, err);
    return;
  }

  // 'NO AUTH' method should present
  static const uint8_t no_auth_method = 0;

  auto it = std::find(methods_copy.begin(), methods_copy.end(),
    no_auth_method);

  if (it == methods_copy.end()) {
    call_and_clear_handler(user_read_req_handler_,
      proxy::error::make_error_code(proxy::error::protocol_violation));
    return;
  }

  // Select 'NO AUTH' method.
  auth_write_resp();
}

void server_session_socks5::auth_write_resp() {
  write_auth_resp_[0] = 5; // VER
  write_auth_resp_[1] = 0; // METHOD = NO AUTH

  boost::asio::async_write(sock_,
    boost::asio::buffer(write_auth_resp_),
    boost::bind(&server_session_socks5::auth_write_resp_handler,
      this, _1, _2));
}

void server_session_socks5::auth_write_resp_handler(error_code err,
  size_t num_bytes)
{
  if (err) {
    call_and_clear_handler(user_read_req_handler_, err);
    return;
  }

  conn_read_req();
}

void server_session_socks5::conn_read_req() {
  boost::asio::async_read(
    sock_,
    boost::asio::buffer(read_conn_hdr_, 4),
    boost::asio::transfer_exactly(4),
    boost::bind(&server_session_socks5::conn_read_req_handler,
      this, _1, _2));
}

void server_session_socks5::conn_read_req_handler(error_code err,
  size_t num_bytes)
{
  if (err) {
    call_and_clear_handler(user_read_req_handler_, err);
    return;
  }

  if (read_conn_hdr_[0] != 5) { // VER == 5
    call_and_clear_handler(user_read_req_handler_,
      proxy::error::make_error_code(proxy::error::protocol_violation));
    return;
  }

  if (read_conn_hdr_[1] != 1) { // CMD == CONNECT
    call_and_clear_handler(user_read_req_handler_,
      proxy::error::make_error_code(proxy::error::unsupported_command));
    return;
  }

  switch (read_conn_hdr_[3]) { // ATYP ==
  case 1:
    conn_read_req_addr_ipv4();
    break;
  case 3:
    conn_read_req_addr_domainname();
    break;
  case 4:
    conn_read_req_addr_ipv6();
    break;
  default:
    call_and_clear_handler(user_read_req_handler_,
      proxy::error::make_error_code(proxy::error::protocol_violation));
    return;
  }
}

void server_session_socks5::conn_read_req_addr_ipv4() {
  boost::asio::async_read(
    sock_,
    boost::asio::buffer(read_addr_, 6),
    boost::asio::transfer_exactly(6),
    boost::bind(
      &server_session_socks5::conn_read_req_addr_ipv4_handler,
      this, _1, _2));
}

void server_session_socks5::conn_read_req_addr_ipv4_handler(
  error_code err, size_t num_bytes)
{
  if (err) {
    call_and_clear_handler(user_read_req_handler_, err);
    return;
  }

  uint32_t addr32 = *reinterpret_cast<uint32_t*>(&read_addr_[0]);
  uint16_t port16 = *reinterpret_cast<uint16_t*>(&read_addr_[4]);

  puser_dst_->hostname = "";
  puser_dst_->ip_address = boost::asio::ip::address_v4(htonl(addr32));
  puser_dst_->port = htons(port16);

  assert(!puser_dst_->using_hostname());

  call_and_clear_handler(user_read_req_handler_, kNoError);

  puser_dst_ = nullptr;
}

void server_session_socks5::conn_read_req_addr_domainname() {
  // Read length (1 octet)
  boost::asio::async_read(
    sock_,
    boost::asio::buffer(read_addr_, 1),
    boost::asio::transfer_exactly(1),
    boost::bind(
      &server_session_socks5::conn_read_req_addr_domainname_handler,
      this, _1, _2));
}

void server_session_socks5::conn_read_req_addr_domainname_handler(
  error_code err, size_t num_bytes)
{
  if (err) {
    call_and_clear_handler(user_read_req_handler_, err);
    return;
  }

  conn_read_req_addr_domainname_str();
}

void server_session_socks5::conn_read_req_addr_domainname_str() {
  uint8_t str_len = read_addr_[0];

  // Read length (1 octet)
  boost::asio::async_read(
    sock_,
    boost::asio::buffer(&read_addr_[1], str_len+2), // domainname+port
    boost::asio::transfer_exactly(str_len +2),
    boost::bind(
     &server_session_socks5::conn_read_req_addr_domainname_str_handler,
     this, _1, _2));
}

void server_session_socks5::conn_read_req_addr_domainname_str_handler(
  error_code err, size_t num_bytes)
{
  if (err) {
    call_and_clear_handler(user_read_req_handler_, err);
    return;
  }

  uint8_t str_len = read_addr_[0];

  puser_dst_->hostname = string(reinterpret_cast<char*>(&read_addr_[1]),
    static_cast<size_t>(str_len));

  puser_dst_->port = htons(
    *reinterpret_cast<uint16_t*>(&read_addr_[str_len+1]));

  assert(puser_dst_->using_hostname());

  call_and_clear_handler(user_read_req_handler_, kNoError);

  puser_dst_ = nullptr;
}

void server_session_socks5::conn_read_req_addr_ipv6() {
  boost::asio::async_read(
    sock_,
    boost::asio::buffer(&read_addr_[0], 18), // ipv6+port
    boost::asio::transfer_exactly(18),
    boost::bind(
      &server_session_socks5::conn_read_req_addr_ipv6_handler,
      this, _1, _2));
}

void server_session_socks5::conn_read_req_addr_ipv6_handler(
  error_code err, size_t num_bytes)
{
  if (err) {
    call_and_clear_handler(user_read_req_handler_, err);
    return;
  }

  // No ideas.
  const uint8_t* r = &read_addr_[0];
  std::array<uint8_t, 16> addr_arr = {
    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9], r[10],
    r[11], r[12], r[13], r[14], r[15]
  };

  puser_dst_->hostname = "";
  puser_dst_->ip_address = boost::asio::ip::address_v6(addr_arr);
  puser_dst_->port = *reinterpret_cast<uint16_t*>(&read_addr_[16]);

  assert(!puser_dst_->using_hostname());

  call_and_clear_handler(user_read_req_handler_, kNoError);

  puser_dst_ = nullptr;
}

// ---

void server_session_socks5::write_connect_response(
  const connect_response& conn_resp,
  write_response_handler handler)
{
  user_write_resp_handler_ = handler;

  write_conn_resp_[0] = 5;                                  // VER = 5
  write_conn_resp_[1] = major_code_to_rep(conn_resp.major); // REP
  write_conn_resp_[2] = 0;                                  // RSV
  write_conn_resp_[3] = 1;                                  // ATYP
  write_conn_resp_[4] = 0;                                  // BND.ADDR[0]
  write_conn_resp_[5] = 0;                                  // BND.ADDR[1]
  write_conn_resp_[6] = 0;                                  // BND.ADDR[2]
  write_conn_resp_[7] = 0;                                  // BND.ADDR[3]
  write_conn_resp_[8] = 0;                                  // BND.PORT[0]
  write_conn_resp_[9] = 0;                                  // BND.PORT[1]

  boost::asio::async_write(sock_,
    boost::asio::buffer(write_conn_resp_, 10),
    boost::bind(&server_session_socks5::write_connect_response_handler,
      this, _1, _2));
}

void server_session_socks5::write_connect_response_handler(
  error_code err, size_t num_bytes)
{
  if (err) {
    call_and_clear_handler(user_write_resp_handler_, err);
    return;
  }

  call_and_clear_handler(user_write_resp_handler_, kNoError);
}

uint8_t server_session_socks5::major_code_to_rep(
  connect_response::major_code mc)
{
  switch (mc) {
  case connect_response::eSucceeded: return 0;
  case connect_response::eHostUnreachable: 4;
  case connect_response::eConnectionRefused: return 5;
  case connect_response::eBadAddressType: return 7;
  default:
  case connect_response::eUnknownError: return 1;
  }
}

// ---

void server_session_socks5::call_and_clear_handler(
  read_request_handler& handler, error_code err)
{
  assert(handler);

  auto handler_copy = handler;
  handler = read_request_handler();
  handler_copy(err);
}

}}
