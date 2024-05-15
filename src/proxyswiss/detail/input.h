
#pragma once

#include "proxyswiss/config.h"

#include "proxy/destination.h"
#include "proxy/connect_response.h"

#include <memory>

namespace proxyswiss {
namespace detail {

class input {
public:
  typedef boost::asio::io_context io_context;
  typedef boost::asio::ip::tcp::socket socket;
  typedef boost::system::error_code error_code;

  typedef std::function<void(error_code)> read_request_handler;
  typedef std::function<void(error_code)> write_response_handler;

  input(socket& sock, const config::input_t& cfg_input,
    const std::string& dbglog_uid);

  // ---

  void read_connect_request(
    proxy::destination& dst,
    read_request_handler handler);

  void write_connect_response(
    const proxy::connect_response& conn_resp,
    write_response_handler handler);

private:
  socket&                                 sock_;
  const config::input_t&                  cfg_input_;
  std::unique_ptr<proxy::server_session>  srv_sess_uptr_;
};

}}
