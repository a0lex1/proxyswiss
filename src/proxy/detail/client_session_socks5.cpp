
#include "proxy/detail/client_session_socks5.h"
#include "proxy/error.h"

#include "common/base/bin_writer.h"
#include "common/base/str.h"

#include <boost/bind/bind.hpp>

#include <assert.h>

#define dbgprint(...) __noop

using namespace std;
using namespace boost::placeholders;

// https://www.ietf.org/rfc/rfc1928.txt
// https://www.ietf.org/rfc/rfc1929.txt

namespace proxy {
namespace detail {

static const boost::system::error_code kNoError;

client_session_socks5::client_session_socks5(socket& sock,
  const credentials& creds)
  :
  client_session(sock, creds),
  conn_read_packet_addr_(270),
  puser_conn_resp_(nullptr)
{
}

void client_session_socks5::call_and_clear_handler(
  write_request_handler& handler, error_code err)
{
  assert(handler);

  auto handler_copy = handler;
  handler = write_request_handler();
  handler_copy(err);
}

void client_session_socks5::write_connect_request(
  destination dst,
  write_request_handler handler)
{
  if (creds_.username.length() > 255 || creds_.password.length() > 255) {
    handler(proxy::error::make_error_code(proxy::error::creds_too_long));
    return;
  }

  if (dst.using_hostname() && dst.hostname.length() > 255) {
    handler(proxy::error::make_error_code(proxy::error::hostname_too_long));
    return;
  }

  user_dst_ = dst;
  user_write_req_handler_ = handler;

  auth_write_req();
}

void client_session_socks5::auth_write_req() {
  write_buf_.clear();
  common::bin_writer binw(write_buf_);
  binw.write_uint8(5); // VER
  binw.write_uint8(1); // NMETHODS

  if (creds_.empty()) {
    binw.write_uint8(0); // NO AUTHENTICATION REQUIRED
  }
  else {
    binw.write_uint8(2); // USERNAME/PASSWORD
  }

  boost::asio::async_write(sock_, boost::asio::buffer(write_buf_),
    boost::bind(&client_session_socks5::auth_write_req_handler,
      this, _1, _2));
}

void client_session_socks5::auth_write_req_handler(error_code err, size_t)
{
  write_buf_.clear();

  if (err) {
    dbgprint("[%s] error %s.%d\n", dbglog_uid_.c_str(),
      err.category().name(), err.value());

    call_and_clear_handler(user_write_req_handler_, err);
    return;
  }

  dbgprint("[%s] OK, reading auth resp\n", dbglog_uid_.c_str());

  auth_read_resp();
}

void client_session_socks5::auth_read_resp() {
  boost::asio::async_read(sock_,
    boost::asio::buffer(auth_read_packet_, 2),
    boost::asio::transfer_exactly(2),
    boost::bind(&client_session_socks5::auth_read_resp_handler, this,
      _1, _2));
}

void client_session_socks5::auth_read_resp_handler(error_code err,
  size_t num_bytes)
{
  if (err) {
    dbgprint("[%s] error %s.%d\n", dbglog_uid_.c_str(),
      err.category().name(), err.value());

    call_and_clear_handler(user_write_req_handler_, err);
    return;
  }

  if (auth_read_packet_[0] != 5) { // VER
    dbgprint("[%s] bad proto in auth ({0x%02x)\n",
      dbglog_uid_.c_str(), auth_read_packet_[0]);

    call_and_clear_handler(user_write_req_handler_,
      proxy::error::make_error_code(proxy::error::protocol_violation));
    return;
  }

  if (auth_read_packet_[1] == 0xff) { // METHOD == FAILED
    call_and_clear_handler(user_write_req_handler_,
      proxy::error::make_error_code(proxy::error::bad_auth_method));
    return;
  }

  if (creds_.empty()) {
    if (auth_read_packet_[1] != 0) { // METHOD == NO AUTHENTICATION REQUIRED
      call_and_clear_handler(user_write_req_handler_,
        proxy::error::make_error_code(proxy::error::protocol_violation));
      return;
    }

    dbgprint("[%s] OK, writing connect req\n", dbglog_uid_.c_str());
    conn_write_req();
  }
  else {
    if (auth_read_packet_[1] != 2) { // METHOD == USERNAME/PASSWORD
      call_and_clear_handler(user_write_req_handler_,
        proxy::error::make_error_code(proxy::error::protocol_violation));
      return;
    }

    dbgprint("[%s] OK, writing socks5 creds\n", dbglog_uid_.c_str());
    auth_write_creds();
  }
}

void client_session_socks5::auth_write_creds() {
  write_buf_.clear();
  common::bin_writer binw(write_buf_);

  binw.write_uint8(0x05);                                           // VER  = SOCKS5
  binw.write_uint8(static_cast<uint8_t>(creds_.username.length())); // ULEN
  binw._write_raw(creds_.username.c_str(),                          // UNAME
    static_cast<uint32_t>(creds_.username.length()));
  binw.write_uint8(static_cast<uint8_t>(creds_.password.length())); // PLEN
  binw._write_raw(creds_.password.c_str(),                          // PASSWD
    static_cast<uint32_t>(creds_.password.length()));

  boost::asio::async_write(sock_, boost::asio::buffer(write_buf_),
    boost::bind(&client_session_socks5::auth_write_creds_handler, this,
      _1, _2));
}

void client_session_socks5::auth_write_creds_handler(error_code err,
  size_t)
{
  write_buf_.clear();

  if (err) {
    dbgprint("[%s] error %s.%d\n", dbglog_uid_.c_str(),
      err.category().name(), err.value());

    call_and_clear_handler(user_write_req_handler_, err);
    return;
  }

  auth_read_creds_reply();
}

void client_session_socks5::auth_read_creds_reply() {
  boost::asio::async_read(sock_,
    boost::asio::buffer(auth_read_packet_, 2),
    boost::asio::transfer_exactly(2),
    boost::bind(&client_session_socks5::auth_read_creds_reply_handler,
      this, _1, _2));
}

void client_session_socks5::auth_read_creds_reply_handler(
  error_code err, size_t)
{
  if (err) {
    dbgprint("[%s] error %s.%d\n", dbglog_uid_.c_str(),
      err.category().name(), err.value());

    call_and_clear_handler(user_write_req_handler_, err);
    return;
  }

  if (auth_read_packet_[0] != 5) { // VER == SOCKS5
    call_and_clear_handler(user_write_req_handler_,
      proxy::error::make_error_code(proxy::error::protocol_violation));
    return;
  }

  if (auth_read_packet_[1] != 0) { // STATUS == success
    call_and_clear_handler(user_write_req_handler_,
      proxy::error::make_error_code(proxy::error::auth_failed));
    return;
  }

  dbgprint("[%s] auth succeeded, writing req\n", dbglog_uid_.c_str());
  conn_write_req();
}

void client_session_socks5::conn_write_req()
{
  write_buf_.clear();
  common::bin_writer binw(write_buf_);
  binw.write_uint8(0x05);                                 // VER  = SOCKS5
  binw.write_uint8(0x01);                                 // CMD  = CONNECT
  binw.write_uint8(0x00);                                 // RSV  = 0

  if (user_dst_.using_hostname()) {
    binw.write_uint8(0x03);                               // ATYP = DOMAINNAME
    binw.write_uint8(
      static_cast<uint8_t>(user_dst_.hostname.length())); // len(DST.ADDR)
    binw._write_raw(user_dst_.hostname.c_str(),           // DST.ADDR
      static_cast<uint32_t>(user_dst_.hostname.length()));
  }
  else {
    if (user_dst_.ip_address.is_v4()) {
      binw.write_uint8(0x01);                            // ATYP = IP V4
      binw.write_uint32(
        htonl(user_dst_.ip_address.to_v4().to_ulong())); // DST.ADDR
    }
    else {
      assert(user_dst_.ip_address.is_v6());
      binw.write_uint8(0x04);                            // ATYP = IP
      binw._write_raw(
        user_dst_.ip_address.to_v6().to_bytes().data(),
        16);                                             // DST.ADDR
    }
  }

  binw.write_uint16(htons(user_dst_.port));              // DST.PORT

  boost::asio::async_write(sock_, boost::asio::buffer(write_buf_),
    boost::bind(&client_session_socks5::conn_write_req_handler, this,
      _1, _2));
}

void client_session_socks5::conn_write_req_handler(error_code err,
  size_t)
{
  write_buf_.clear();

  if (err) {
    dbgprint("[%s] error %s.%d\n", dbglog_uid_.c_str(),
      err.category().name(), err.value());

    call_and_clear_handler(user_write_req_handler_, err);
    return;
  }

  dbgprint("[%s] OK, done\n", dbglog_uid_.c_str());

  call_and_clear_handler(user_write_req_handler_, kNoError);
}

// ---

void client_session_socks5::read_connect_response(
  connect_response& conn_resp,
  read_response_handler handler)
{
  assert(!user_read_resp_handler_);
  assert(nullptr == puser_conn_resp_);

  puser_conn_resp_ = &conn_resp;
  user_read_resp_handler_ = handler;

  conn_read_resp();
}

void client_session_socks5::conn_read_resp() {
  boost::asio::async_read(sock_, 
    boost::asio::buffer(conn_read_packet_header_, 4),
    boost::asio::transfer_exactly(4),
    boost::bind(&client_session_socks5::conn_read_resp_handler, this,
      _1, _2));
}

void client_session_socks5::conn_read_resp_handler(error_code err,
  size_t num_bytes)
{
  if (err) {
    dbgprint("[%s] error %s.%d\n", dbglog_uid_.c_str(),
      err.category().name(), err.value());

    call_and_clear_handler(user_read_resp_handler_, err);
    return;
  }

  if (conn_read_packet_header_[0] != 5) { // VER == 5
    dbgprint("[%s] bad proto in conn (ver={0x%02x})\n",
      dbglog_uid_.c_str(), conn_read_packet_header_[0]);

    call_and_clear_handler(user_read_resp_handler_,
      proxy::error::make_error_code(proxy::error::protocol_violation));
    return;
  }

  // Check only if REP is a valid value. The meaning of REP
  // is considered in conn_read_resp_addr_complete().
  //
  if (conn_read_packet_header_[1] > 8) { // REP in 0 .. 8
    dbgprint("[%s] bad proto in conn ({rep=0x%02x})\n",
      dbglog_uid_.c_str(), conn_read_packet_header_[1]);

    call_and_clear_handler(user_read_resp_handler_,
      proxy::error::make_error_code(proxy::error::protocol_violation));
    return;
  }

  // Need to read the rest of response packet, the address.
  //
  switch (conn_read_packet_header_[3]) { // ATYP ==
  case 1: // IP V4
    conn_read_resp_addr_ipv4();
    break;
  case 3: // DOMAINNAME
    conn_read_resp_addr_domainname();
    break;
  case 4: // IP V6
    conn_read_resp_addr_ipv6();
    break;
  default:
    call_and_clear_handler(user_read_resp_handler_,
      proxy::error::make_error_code(proxy::error::protocol_violation));
    return;
  }
}

void client_session_socks5::conn_read_resp_addr_complete(error_code err) {
  // Address has been read (it was either ipv4/v6/domainname).
  // Check REP field and call user's handler.
  //
  if (err) {
    dbgprint("[%s] error %s.%d\n", dbglog_uid_.c_str(),
      err.category().name(), err.value());

    call_and_clear_handler(user_read_resp_handler_, err);
    return;
  }

  if (conn_read_packet_header_[1] != 0) { // REP == succeeded
    dbgprint("[%s] rep!=succeeded (rep==0x%02x)\n", dbglog_uid_.c_str(),
      conn_read_packet_header_[1]);

    *puser_conn_resp_ = connect_response(
      socks5_rep_to_major_code(conn_read_packet_header_[1]));

    call_and_clear_handler(user_read_resp_handler_, kNoError);
    return;
  }

  dbgprint("[%s] OK, done\n", dbglog_uid_.c_str());

  *puser_conn_resp_ = connect_response(connect_response::eSucceeded);
  call_and_clear_handler(user_read_resp_handler_, kNoError);

  puser_conn_resp_ = nullptr;
}

void client_session_socks5::conn_read_resp_addr_ipv4() {
  boost::asio::async_read(sock_,
    boost::asio::buffer(&conn_read_packet_addr_[0], 6), // ipv4+port
    boost::asio::transfer_exactly(6),
    boost::bind(&client_session_socks5::conn_read_resp_addr_ipv4_handler,
      this, _1, _2));
}

void client_session_socks5::conn_read_resp_addr_ipv4_handler(
  error_code err, size_t num_bytes)
{
  if (err) {
    dbgprint("[%s] error %s.%d\n", dbglog_uid_.c_str(),
      err.category().name(), err.value());
  }
  else {
    dbgprint("[%s] OK\n", dbglog_uid_.c_str());
  }
  conn_read_resp_addr_complete(err);
}

void client_session_socks5::conn_read_resp_addr_domainname() {
  boost::asio::async_read(sock_,
    boost::asio::buffer(&conn_read_packet_addr_[0], 1), // len
    boost::asio::transfer_exactly(1),
    boost::bind(
      &client_session_socks5::conn_read_resp_addr_domainname_handler,
      this, _1, _2));
}

void client_session_socks5::conn_read_resp_addr_domainname_handler(
  error_code err, size_t num_bytes)
{
  if (err) {
    dbgprint("[%s] error %s.%d\n", dbglog_uid_.c_str(),
      err.category().name(), err.value());

    call_and_clear_handler(user_read_resp_handler_, err);
    return;
  }
  // The length of DOMAINNAME string has been read, now read the
  // string itself.
  dbgprint("[%s] OK, reading string\n", dbglog_uid_.c_str());
  conn_read_resp_addr_domainname_str();
}

void client_session_socks5::conn_read_resp_addr_domainname_str() {
  uint8_t str_len = conn_read_packet_addr_[0];

  boost::asio::async_read(sock_,
    boost::asio::buffer(&conn_read_packet_addr_[1], str_len+2),
    boost::asio::transfer_exactly(str_len+2),
    boost::bind(
      &client_session_socks5::conn_read_resp_addr_domainname_str_handler,
      this, _1, _2));
}

void client_session_socks5::conn_read_resp_addr_domainname_str_handler(
  error_code err, size_t num_bytes)
{
  if (err) {
    dbgprint("[%s] error %s.%d\n", dbglog_uid_.c_str(),
      err.category().name(), err.value());
  } else {
    dbgprint("[%s] OK\n", dbglog_uid_.c_str());
  }
  conn_read_resp_addr_complete(err);
}

void client_session_socks5::conn_read_resp_addr_ipv6() {
  boost::asio::async_read(sock_,
    boost::asio::buffer(&conn_read_packet_addr_[0], 18), // ipv6+port
    boost::asio::transfer_exactly(18),
    boost::bind(&client_session_socks5::conn_read_resp_addr_ipv6_handler,
      this, _1, _2));
}

void client_session_socks5::conn_read_resp_addr_ipv6_handler(
  error_code err, size_t num_bytes)
{
  if (err) {
    dbgprint("[%s] error %s.%d\n", dbglog_uid_.c_str(),
      err.category().name(), err.value());
  } else {
    dbgprint("[%s] OK\n", dbglog_uid_.c_str());
  }
  conn_read_resp_addr_complete(err);
}

connect_response::major_code
client_session_socks5::socks5_rep_to_major_code(
  uint8_t rep)
{
  // Some socks5 errors ('rep's) map to abstract eUnknownError.
  //
  switch (rep) {
  case 0: return connect_response::eSucceeded;
  case 1: return connect_response::eUnknownError;
  case 2: return connect_response::eUnknownError;
  case 3: return connect_response::eUnknownError;
  case 4: return connect_response::eHostUnreachable;
  case 5: return connect_response::eConnectionRefused;
  case 6: return connect_response::eUnknownError;
  case 7: return connect_response::eUnknownError;
  case 8: return connect_response::eBadAddressType;
  default:
    return connect_response::eUnknownError;
  }
}

}}
