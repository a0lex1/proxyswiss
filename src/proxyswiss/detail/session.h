
#pragma once

#include "proxyswiss/detail/input.h"
#include "proxyswiss/detail/output.h"

#ifdef _DEBUG
#include "proxyswiss/detail/debug_uid.h"
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <functional>
#include <memory>
#include <vector>
#include <fstream>
#include <set>

namespace proxyswiss {
namespace detail {

class session: public boost::enable_shared_from_this<session> {
public:
  typedef boost::asio::io_context io_context;
  typedef boost::asio::ip::tcp::socket socket;
  typedef boost::system::error_code error_code;
  typedef boost::asio::ip::tcp::endpoint endpoint;

  session(io_context& ios, const config& cfg
#ifdef _DEBUG
    , detail::debug_uid_table& dbg_uid_table
#endif
    );
  ~session();

  socket& sock() { return input_sock_; }

  void enable_print_proxy_errors(bool enable);
  void set_log_file(std::ofstream& logfile,
                    std::set<std::string>& history);

  void start();

private:
  void alloc_read_buffers();
  void close_all();
  void make_tunnel();
  void handle_read_connect_request(error_code);
  void handle_connect_output(const output::connect_result&);
  void handle_write_connect_response(error_code);

  void begin_input_read();
  void begin_output_read();
  void handle_input_read(error_code, size_t);
  void handle_output_read(error_code, size_t);

  void begin_input_write(size_t);
  void begin_output_write(size_t);
  void handle_input_write(error_code, size_t);
  void handle_output_write(error_code, size_t);

private:
#ifdef _DEBUG
  detail::debug_uid dbg_uid_;
#endif
  std::string dbg_uid_str_;

private:
  const config&                       cfg_;
  socket                              input_sock_;
  socket                              output_sock_;
  input                               input_;
  output                              output_;
  proxy::destination                  dst_;
  output::connect_result              output_conn_res_;
  std::unique_ptr<std::vector<char>>  input_read_buf_uptr_;
  std::unique_ptr<std::vector<char>>  output_read_buf_uptr_;
  bool                                print_proxy_errors_;
  std::ofstream*                      plogfile_;
  std::set<std::string>*              phistory_;
};

}}
