
#pragma once

#include "proxyswiss/config.h" 

#include "proxyswiss/detail/session.h"

#ifdef _DEBUG
#include "proxyswiss/detail/debug_uid_table.h"
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>
#include <set>

namespace proxyswiss {

class server {
public:
  typedef boost::asio::io_context io_context;
  typedef boost::asio::ip::tcp::endpoint endpoint;
  typedef boost::system::error_code error_code;

  server(io_context& ioc, const config& cfg);

  void enable_print_proxy_errors(bool enable);
  bool enable_logging(const std::wstring& filename);

  bool open(error_code& err);
  void start();

private:
  void do_accept();
  void handle_accept(error_code);

private:
#ifdef _DEBUG
  detail::debug_uid_table dbg_uid_table_;
#endif

private:
  typedef boost::asio::ip::tcp::acceptor acceptor;
  typedef boost::shared_ptr<detail::session> session_shared_ptr;

  io_context& ioc_;
  acceptor                acpt_;
  const config&           cfg_;
  session_shared_ptr      sess_sptr_;
  bool                    print_proxy_errors_;
  std::ofstream           logfile_;
  std::set<std::string>   history_;
};

}
