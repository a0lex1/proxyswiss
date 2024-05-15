
#include "proxyswiss/detail/input.h"

#include <boost/bind/bind.hpp>

#include <assert.h>

using namespace std;
using namespace boost::placeholders;

namespace proxyswiss {
namespace detail {

input::input(socket& sock, const config::input_t& cfg_input,
  const string& dbglog_uid)
  :
  sock_(sock), cfg_input_(cfg_input)
{
  if (cfg_input_.type == config::eProxyServer) {
    srv_sess_uptr_.reset(proxy::create_server_session(sock_,
      cfg_input_.as_proxy_server.proxy_server_type, dbglog_uid));
  }
}

void input::read_connect_request(proxy::destination& dst,
  read_request_handler handler)
{
  if (cfg_input_.type == config::eTunnel) {
    assert(!srv_sess_uptr_);

    dst = cfg_input_.as_tunnel.destination;
    handler(boost::system::error_code());
  }
  else {
    assert(srv_sess_uptr_);

    srv_sess_uptr_->read_connect_request(dst, handler);
  }
}

void input::write_connect_response(
  const proxy::connect_response& conn_resp,
  write_response_handler handler)
{
  if (cfg_input_.type == config::eTunnel) {
    assert(!srv_sess_uptr_);

    handler(boost::system::error_code());
  }
  else {
    assert(srv_sess_uptr_);

    srv_sess_uptr_->write_connect_response(conn_resp, handler);
  }
}

}}
