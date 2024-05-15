
#include "proxyswiss/detail/session.h"
#include "proxy/error.h"

#include "common/base/str.h"

#include <boost/bind/bind.hpp>

#include <iostream>

#define dbgprint(...) __noop

using namespace std;
using namespace boost::placeholders;

namespace proxyswiss {
namespace detail {

static const size_t kReadBufSize = 4096;

session::session(io_context& ioc, const config& cfg
#ifdef _DEBUG
  , detail::debug_uid_table& dbg_uid_table
#endif
  )
  :
#ifdef _DEBUG
  dbg_uid_(dbg_uid_table),
  dbg_uid_str_(dbg_uid_.to_string()),
#endif
  //ioc_(ioc),
  cfg_(cfg),
  input_sock_(ioc),
  output_sock_(ioc),
  input_(input_sock_, cfg.input, dbg_uid_str_),
  output_(ioc, output_sock_, cfg.output, dbg_uid_str_),
  print_proxy_errors_(false),
  plogfile_(nullptr)
{
}

session::~session() {
  dbgprint("[%s] session closed\n", dbg_uid_str_.c_str());
}

void session::enable_print_proxy_errors(bool enable) {
  print_proxy_errors_ = enable;
}

void session::set_log_file(std::ofstream&   logfile,
                           set<string>&     history)
{
  plogfile_ = &logfile;
  phistory_ = &history;
}

void session::start() {
  input_.read_connect_request(dst_,
    boost::bind(&session::handle_read_connect_request, shared_from_this(),
      _1));
}

void session::close_all() {
  input_sock_.close();
  output_sock_.close();
}

void session::handle_read_connect_request(error_code err) {
  if (err) {
    dbgprint("[%s] error %s.%d\n", dbg_uid_str_.c_str(),
      err.category().name(), err.value());

    close_all();
    return;
  }

  dbgprint("[%s] connecting through chain to {%s}\n", dbg_uid_str_.c_str(),
    dst_.to_string().c_str());

  output_.connect_through_chain(dst_,
    boost::bind(&session::handle_connect_output, shared_from_this(), _1));
}

void session::handle_connect_output(const output::connect_result& conn_res)
{
  output_conn_res_ = conn_res;

  proxy::connect_response prx_resp;
  if (conn_res.success) {

    string id_str;
    id_str = dst_.using_hostname() ? dst_.hostname : dst_.ip_address.to_string();
    id_str += ":" + common::str_from_uint(dst_.port);

    if (phistory_->end() ==
        std::find(phistory_->begin(), phistory_->end(),
                  id_str))
    {
      if (plogfile_) {
        (*plogfile_) << id_str << endl;
      }
      phistory_->insert(id_str);
    }

    prx_resp.major = proxy::connect_response::eSucceeded;
  }
  else {
    if (conn_res.err) {

      // Print proxy-level errors (auth failed, bad auth method, etc)
      if (print_proxy_errors_) {
        if (conn_res.err.category() ==
            proxy::error::get_basic_error_category())
        {
          string fail_proxy_address;
          const size_t idx = conn_res.chain_fail_index;

          if (idx != output::connect_result::kNoIndex) {
            fail_proxy_address =
              common::wstr_to_str(
                proxy::client_session_type_to_string(
                  cfg_.output.proxy_chain[idx].proxy_client_type
                )
              )
              + "://" +
              cfg_.output.proxy_chain[idx].proxy_address.to_string();
          }
          else {
            fail_proxy_address = "?";
          }

          cout << "[PROXY ERROR] " << fail_proxy_address << ": " <<
            conn_res.err.message() << "\n";
        }
      }
      //

      if (!proxy::connect_response::error_code_to_major(conn_res.err,
        prx_resp.major))
      {
        prx_resp.major = proxy::connect_response::eUnknownError;
      }
    }
    else {
      prx_resp = conn_res.conn_resp;
    }
  }

  dbgprint("[%s] {%s} connect_result: %s\n",
    dbg_uid_str_.c_str(), dst_.to_string().c_str(),
    conn_res.to_string().c_str());

  input_.write_connect_response(prx_resp,
    boost::bind(&session::handle_write_connect_response,
      shared_from_this(), _1));
}

void session::handle_write_connect_response(error_code err) {
  if (err) {
    dbgprint("[%s] {%s} error %s.%d\n", dbg_uid_str_.c_str(),
      dst_.to_string().c_str(), err.category().name(), err.value());

    close_all();
    return;
  }

  if (!output_conn_res_.success) {
    dbgprint("[%s] {%s} closing because !conn_res_.success\n",
      dbg_uid_str_.c_str(), dst_.to_string().c_str());

    close_all();
    return;
  }

  dbgprint("[%s] {%s} OK, making tunnel ...\n", dbg_uid_str_.c_str(),
    dst_.to_string().c_str());

  make_tunnel();
}

void session::alloc_read_buffers() {
  input_read_buf_uptr_.reset(new vector<char>(kReadBufSize));
  output_read_buf_uptr_.reset(new vector<char>(kReadBufSize));
}

void session::make_tunnel() {
  alloc_read_buffers();

  begin_input_read();
  begin_output_read();
}

void session::begin_input_read() {
  input_sock_.async_read_some(boost::asio::buffer(*input_read_buf_uptr_),
    boost::bind(&session::handle_input_read, shared_from_this(), _1, _2));
}

void session::begin_output_read() {
  output_sock_.async_read_some(boost::asio::buffer(*output_read_buf_uptr_),
    boost::bind(&session::handle_output_read, shared_from_this(), _1, _2));
}

void session::handle_input_read(error_code err, size_t num_bytes) {
  if (!err) {
    begin_output_write(num_bytes);
  }
  else {
    if (err == boost::asio::error::eof) {
      dbgprint("eof\n");

      error_code ec;
      output_sock_.shutdown(boost::asio::ip::tcp::socket::shutdown_send,
                            ec);
    }
    else {
      dbgprint("hard error, closing\n");
      close_all();
    }
  }
}

void session::handle_output_read(error_code err, size_t num_bytes) {
  if (!err) {
    begin_input_write(num_bytes);
  }
  else {
    if (err == boost::asio::error::eof) {
      dbgprint("eof\n");

      error_code ec;
      input_sock_.shutdown(boost::asio::ip::tcp::socket::shutdown_send,
                           ec);
    }
    else {
      dbgprint("hard error, closing\n");
      close_all();
    }
  }
}

void session::begin_input_write(size_t num_bytes) {
  boost::asio::async_write(input_sock_,
    boost::asio::buffer(*output_read_buf_uptr_, num_bytes),
    boost::bind(&session::handle_input_write, shared_from_this(), _1, _2));
}

void session::begin_output_write(size_t num_bytes) {
  boost::asio::async_write(output_sock_,
    boost::asio::buffer(*input_read_buf_uptr_, num_bytes),
    boost::bind(&session::handle_output_write, shared_from_this(), _1, _2));
}

void session::handle_input_write(error_code err, size_t) {
  if (err) {
    close_all();
    return;
  }
  begin_output_read();
}

void session::handle_output_write(error_code err, size_t) {
  if (err) {
    close_all();
    return;
  }
  begin_input_read();
}

}}
