
#include "proxy/detail/server_session_https.h"
#include "proxy/detail/read_until.h"
#include "proxy/error.h"

#include "proxy/destination_from_url_parser.h"

#include "common/base/str.h"
#include "common/net/url_parser.h"

#include <boost/bind/bind.hpp>
using namespace boost::placeholders;

#include <assert.h>

using namespace std;

namespace proxy {
namespace detail {

static const size_t kMaxLineLen = 2048;
static const boost::system::error_code kNoError;

server_session_https::server_session_https(socket& sock)
  :
  server_session(sock), puser_dst_(nullptr), http_ver_(eHttp10)
{
}

// ---

void server_session_https::read_connect_request(
  destination& dst, read_request_handler handler)
{
  assert(!user_read_req_handler_);
  assert(nullptr == puser_dst_);

  puser_dst_ = &dst;
  user_read_req_handler_ = handler;

  read_first_line();
}

void server_session_https::read_line(
  boost::function<void(error_code)> handler)
{
  read_until(sock_, line_, "\n", kMaxLineLen, handler);
}

void server_session_https::read_first_line() {
  read_line(boost::bind(&server_session_https::handle_read_first_line,
    this, _1));
}

void server_session_https::read_another_line() {
  read_line(boost::bind(&server_session_https::handle_read_another_line,
    this, _1));
}

void server_session_https::handle_read_first_line(error_code err) {
  if (err) {
    call_and_clear_handler(user_read_req_handler_, err);
    return;
  }

  if (!trim_line(line_)) {
    call_and_clear_handler(user_read_req_handler_,
      proxy::error::make_error_code(proxy::error::line_too_long));

    return;
  }

  error_code parse_err = parse_first_line(line_, parsed_dst_, http_ver_);
  if (parse_err) {
    call_and_clear_handler(user_read_req_handler_, parse_err);
    return;
  }

  read_another_line();
}

void server_session_https::handle_read_another_line(error_code err) {
  if (err) {
    call_and_clear_handler(user_read_req_handler_, err);
    return;
  }

  if (!trim_line(line_)) {
    call_and_clear_handler(user_read_req_handler_,
      proxy::error::make_error_code(proxy::error::line_too_long));

    return;
  }

  if (!line_.empty()) {
    error_code verify_err = verify_header_line(line_);
    if (err) {
      call_and_clear_handler(user_read_req_handler_, verify_err);
      return;
    }
    
    read_another_line();
  }
  else {
    *puser_dst_ = parsed_dst_;

    call_and_clear_handler(user_read_req_handler_, kNoError);

    puser_dst_ = nullptr;
  }
}

boost::system::error_code server_session_https::parse_first_line(
  const std::string& line, proxy::destination& dst, http_version& http_ver)
{
  vector<string> parts;
  common::str_split(line, " ", parts);

  if (parts.size() != 3) {
    return proxy::error::make_error_code(proxy::error::protocol_violation);
  }

  if (parts[0] != "CONNECT") {
    return proxy::error::make_error_code(proxy::error::protocol_violation);
  }

  if (parts[2] == "HTTP/1.0") {
    http_ver = eHttp10;
  }
  else {
    if (parts[2] == "HTTP/1.1") {
      http_ver = eHttp11;
    }
    else {
      return proxy::error::make_error_code(
        proxy::error::protocol_violation);
    }
  }

  common::net::url_parser up;
  if (!up.parse(parts[1])) {
    return proxy::error::make_error_code(proxy::error::protocol_violation);
  }

  // Allow only host:port format

  // No scheme://
  if (up.scheme() != nullptr) {
    return proxy::error::make_error_code(proxy::error::protocol_violation);
  }

  // No user:pass@
  if (up.username() != nullptr || up.password() != nullptr) {
    return proxy::error::make_error_code(proxy::error::protocol_violation);
  }

  // Port must present
  if (up.port() == nullptr) {
    return proxy::error::make_error_code(proxy::error::protocol_violation);
  }

  proxy::destination_from_url_parser<char>(dst, up.host(), *up.port());

  return kNoError;
}

boost::system::error_code server_session_https::verify_header_line(
  const std::string& line)
{
  vector<string> parts;
  common::str_split(line, ": ", parts);
  if (parts.size() != 2) {
    return proxy::error::make_error_code(proxy::error::protocol_violation);
  }
  if (parts[0].empty() || parts[1].empty()) {
    return proxy::error::make_error_code(proxy::error::protocol_violation);
  }
  return kNoError;
}

bool server_session_https::trim_line(std::string& line) {
  if (line.empty()) {
    return false;
  }
  const char* p = &line.c_str()[line.length()];
  if (*(p-1) == '\n') {
    if (line.length() >= 2 && *(p-2) == '\r') {
      line.resize(line.length() - 2);
    }
    else {
      line.resize(line.length() - 1);
    }
    return true;
  }
  else {
    return false;
  }
}

// ---

void server_session_https::write_connect_response(
  const connect_response& conn_resp, write_response_handler handler)
{
  const char* banner = http_ver_ == eHttp10 ? "HTTP/1.0" : "HTTP/1.1";
  unsigned code;
  string description;

  if (!major_to_http_code(conn_resp.major, code, description)) {
    code = 502;
    description = "Bad Gateway";
  }

  line_ = common::str_printf(
    "%s %d %s\r\n"
    "Content-Length: 0\r\n"
    "\r\n",
    banner, code, description.c_str());

  boost::asio::async_write(
    sock_,
    boost::asio::buffer(line_),
    boost::bind(handler, _1));
}

bool server_session_https::major_to_http_code(
  connect_response::major_code mc, unsigned& code, string& description)
{
  switch (mc) {
  case connect_response::eSucceeded:
    code = 200;
    description = "Connection established";
    return true;
  default:
    return false;
  }
}

// ---

void server_session_https::call_and_clear_handler(
  read_request_handler& handler, error_code err)
{
  assert(handler);

  auto handler_copy = handler;
  handler = read_request_handler();
  handler_copy(err);
}

}}
