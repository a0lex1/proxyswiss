
#pragma once

#include "proxyswiss/config.h"

#include "proxy/destination.h"
#include "proxy/client_session.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <memory>
#include <vector>

namespace proxyswiss {
namespace detail {

class output {
public:
  typedef boost::asio::io_context io_context;
  typedef boost::asio::ip::tcp::socket socket;
  typedef boost::asio::ip::tcp::endpoint endpoint;
  typedef boost::asio::ip::tcp::resolver resolver;
  typedef boost::system::error_code error_code;

  struct connect_result {
    bool                     success;
    error_code               err;
    size_t                   chain_fail_index;
    proxy::connect_response  conn_resp;

    static const size_t kNoIndex = -1;

    connect_result(
      bool s = false,
      error_code e = error_code(),
      size_t i = kNoIndex,
      proxy::connect_response r = proxy::connect_response());

    std::string to_string() const;
  };

  typedef std::function<void(const connect_result&)> connect_handler;

  output(io_context& ioc, socket& sock, const config::output_t& cfg_output,
    const std::string& dbglog_uid);

  // ---

  void connect_through_chain(const proxy::destination& dst,
    connect_handler handler);

private:
  void create_chain(const std::string&);
  void call_and_clear_handler(connect_result);
  void connect_next(error_code, size_t);

  void handle_resolve(error_code, resolver::iterator, uint16_t, size_t);
  void handle_connect(error_code, size_t);
  void handle_write_connect_request(error_code, size_t);
  void handle_read_connect_response(error_code, size_t);

private:
  io_context& ioc_;
  std::string                                        dbglog_uid_;
  socket&                                            sock_;
  const config::output_t&                            cfg_output_;
  connect_handler                                    user_connect_handler_;
  proxy::destination                                 final_dst_;
  std::vector<std::unique_ptr<proxy::client_session>>  chain_;
  std::unique_ptr<resolver>                          resolver_uptr_;
  size_t                                             cur_proxy_;
  proxy::connect_response                            conn_resp_;
};

}}
