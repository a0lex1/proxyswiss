
#include "proxyswiss/detail/output.h"

#include "proxy/client_session.h"

#include "common/base/str.h"

#include <boost/bind/bind.hpp>

#include <assert.h>
#include <iostream>

using namespace std;
using namespace boost::asio::ip;
using namespace boost::placeholders;

#define dbgprint(...) __noop

namespace proxyswiss {
namespace detail {

output::output(io_context& ioc, socket& sock, const config::output_t& cfg_output,
  const string& dbglog_uid)
  : ioc_(ioc), sock_(sock), cfg_output_(cfg_output), dbglog_uid_(dbglog_uid)
{
  create_chain(dbglog_uid);
}

void output::create_chain(const string& dbglog_uid) {
  for (size_t i = 0; i < cfg_output_.proxy_chain.size(); i++) {
    chain_.push_back(unique_ptr<proxy::client_session>());
    chain_.back().reset(
      proxy::create_client_session(sock_,
        cfg_output_.proxy_chain[i].proxy_client_type,
        cfg_output_.proxy_chain[i].proxy_creds,
        dbglog_uid));
  }
}

void output::call_and_clear_handler(connect_result cr) {
  connect_handler handler_copy = user_connect_handler_;
  user_connect_handler_ = connect_handler();
  handler_copy(cr);
}

void output::connect_through_chain(const proxy::destination& dst,
  connect_handler handler)
{
  assert(!user_connect_handler_);

  final_dst_ = dst;
  user_connect_handler_ = handler;
  cur_proxy_ = 0;

  if (chain_.empty()) {
    if (dst.using_hostname()) {
      resolver_uptr_.reset(new resolver(ioc_));
      tcp::resolver::query query(dst.hostname, "");
      resolver_uptr_->async_resolve(query,
        boost::bind(&output::handle_resolve, this,
          _1, _2, final_dst_.port, connect_result::kNoIndex));
    }
    else {
      sock_.async_connect(tcp::endpoint(dst.ip_address, dst.port),
        boost::bind(&output::handle_connect, this,
          _1, connect_result::kNoIndex));
    }
  }
  else {
    proxy::destination first_proxy(
      cfg_output_.proxy_chain[0].proxy_address);

    if (first_proxy.using_hostname()) {
      resolver_uptr_.reset(new resolver(ioc_));
      tcp::resolver::query query(first_proxy.hostname, "");
      resolver_uptr_->async_resolve(query,
        boost::bind(&output::handle_resolve, this,
          _1, _2, first_proxy.port, 0/*index*/));
    }
    else {
      sock_.async_connect(
        tcp::endpoint(first_proxy.ip_address, first_proxy.port),
        boost::bind(&output::handle_connect, this, _1, 0));
    }
  }
}

void output::handle_resolve(error_code err, resolver::iterator it,
  uint16_t port, size_t index)
{
  resolver_uptr_.reset();

  if (err) {
    dbgprint("[%s] can't resolve, error %s.%d (chain[%d])\n",
      dbglog_uid_.c_str(),
      err.category().name(),
      err.value(),
      index);

    call_and_clear_handler(connect_result(false, err, index));
    return;
  }

  dbgprint("[%s] %s resolved to %s (chain[%d])\n",
    dbglog_uid_.c_str(),
    it->host_name().c_str(),
    it->endpoint().address().to_string().c_str(),
    index);

  sock_.async_connect(
    tcp::endpoint(it->endpoint().address(), port),
    boost::bind(&output::handle_connect, this, _1, index));
}

void output::connect_next(error_code err, size_t index) {
  if (err) {
    call_and_clear_handler(connect_result(false, err, index));
    return;
  }

  if (cur_proxy_ == chain_.size()) {
    call_and_clear_handler(connect_result(true));
    return;
  }

  assert(cur_proxy_ < chain_.size());

  proxy::destination next_dst;
  if (cur_proxy_+1 < chain_.size()) {
    next_dst = cfg_output_.proxy_chain[cur_proxy_+1].proxy_address;
  }
  else {
    next_dst = final_dst_;
  }

  dbgprint("[%s] writing connect request to %s (chain[%d])\n",
    dbglog_uid_.c_str(), next_dst.to_string().c_str(), index);

  chain_[cur_proxy_]->write_connect_request(next_dst,
    boost::bind(&output::handle_write_connect_request, this, _1, index));
}

void output::handle_connect(error_code err, size_t index) {
  if (err) {
    dbgprint("[%s] error %s.%d (chain[%d])\n",
      dbglog_uid_.c_str(),
      err.category().name(),
      err.value(),
      index);
  }
  else {
    dbgprint("[%s] ok (chain[%d])\n", dbglog_uid_.c_str(), index);
  }
  connect_next(err, index);
}

void output::handle_write_connect_request(error_code err, size_t index) {
  if (err) {
    dbgprint("[%s] error %s.%d (chain[%d])\n", dbglog_uid_.c_str(),
      err.category().name(), err.value(), index);

    call_and_clear_handler(connect_result(false, err, index));
    return;
  }

  dbgprint("[%s] ok (chain[%d])\n", dbglog_uid_.c_str(), index);

  chain_[cur_proxy_]->read_connect_response(conn_resp_,
    boost::bind(&output::handle_read_connect_response, this, _1, index));
}

void output::handle_read_connect_response(error_code err, size_t index) {
  if (err) {
    dbgprint("[%s] error %s.%d (chain[%d])\n", dbglog_uid_.c_str(),
      err.category().name(), err.value(), index);

    call_and_clear_handler(connect_result(false, err, index));
    return;
  }
  if (conn_resp_.major != proxy::connect_response::eSucceeded) {
    dbgprint("[%s] proxy responded %s (chain[%d])\n",
      dbglog_uid_.c_str(),
      proxy::connect_response::major_code_to_string(conn_resp_.major),
      index);

    call_and_clear_handler(connect_result(
      false, boost::system::error_code(), index, conn_resp_));

    return;
  }

  dbgprint("[%s] ok (chain[%d])\n", dbglog_uid_.c_str(), index);

  ++cur_proxy_;
  connect_next(boost::system::error_code(), index+1);
}

// ---

output::connect_result::connect_result(
  bool _success, error_code _err, size_t _chain_fail_index,
  proxy::connect_response _conn_resp)
  :
  success(_success),
  err(_err),
  chain_fail_index(_chain_fail_index),
  conn_resp(_conn_resp)
{
}

string output::connect_result::to_string() const {
  if (success) {
    return "success";
  }
  else {
    if (err) {
      return common::str_printf(
        "(fail, (%s.%d (%s)), index %d)",
        err.category().name(),
        err.value(),
        err.message().c_str(),
        chain_fail_index);
    }
    else {
      return common::str_printf(
        "(fail, (), index %d, %s)",
        chain_fail_index,
        proxy::connect_response::major_code_to_string(
          conn_resp.major).c_str());
    }
  }
}

}}
