
#include "proxyswiss/server.h"

#include <boost/bind/bind.hpp>

#define dbgprint(...) __noop

using namespace std;
using namespace boost::asio::ip;
using boost::asio::io_context;
using namespace boost::placeholders;

namespace proxyswiss {

server::server(io_context& ioc, const config& cfg)
  : ioc_(ioc), acpt_(ioc), cfg_(cfg)
{
}

void server::enable_print_proxy_errors(bool enable) {
  print_proxy_errors_ = enable;
}

bool server::enable_logging(const wstring& filename) {
  logfile_.open(filename, std::ios::out);
  if (!logfile_.is_open()) {
    return false;
  }
  return true;
}

bool server::open(error_code& err) {
  acpt_.open(cfg_.input.listen_addr.protocol(), err);
  if (err) {
    return false;
  }
  if (!err) {
    acpt_.bind(cfg_.input.listen_addr, err);
    if (!err) {
      static const int backlog = boost::asio::socket_base::max_connections;
      acpt_.listen(backlog, err);
      if (!err) {
        return true;
      }
      else {
        dbgprint("acceptor::listen(%d) failed, error %s.%d (%s)\n",
          backlog, err.category().name(), err.value(),
          err.message().c_str());
      }
    }
    else {
      /*dbgprint("acceptor::bind(%s:%d) failed, error %s.%d (%s)\n",
        cfg_.input.listen_addr.address().to_string().c_str(),
        cfg_.input.listen_addr.port(),
        err.category().name(), err.value(), err.message().c_str());*/
    }
  }
  else {
    dbgprint("acceptor::open(family=%d) failed, error %s.%d (%s)\n",
      cfg_.input.listen_addr.protocol().family(),
      err.category().name(), err.value(), err.message().c_str());
  }
  acpt_.close();
  return false;
}

void server::start() {
  do_accept();
}

void server::do_accept() {
  sess_sptr_.reset(new detail::session(ioc_, cfg_
#ifdef _DEBUG
    , dbg_uid_table_
#endif
  ));

  sess_sptr_->enable_print_proxy_errors(print_proxy_errors_);

  acpt_.async_accept(
    sess_sptr_->sock(),
    boost::bind(&server::handle_accept, this, _1));
}

void server::handle_accept(error_code err) {
  if (err) {
    dbgprint("error %s.%d (%s)\n", err.category().name(), err.value(),
      err.message().c_str());
  }
  else {
    dbgprint("accepted\n");

    sess_sptr_->set_log_file(logfile_, history_);
    sess_sptr_->start();
  }
  do_accept();
}

}
