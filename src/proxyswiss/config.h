
#pragma once

#include "proxy/server_session.h"
#include "proxy/client_session.h"

#include <boost/asio/ip/tcp.hpp>

#include <string>
#include <vector>

namespace proxyswiss {

struct config {
  typedef boost::asio::ip::tcp::endpoint endpoint;

  enum input_type {
    eTunnel,
    eProxyServer
  };

  struct input_t {
    input_type   type;
    endpoint     listen_addr;

    // If |type| is eTunnel
    struct {
      proxy::destination destination;
    } as_tunnel;

    // Otherwise, if |type| is eProxyServer
    struct {
      proxy::server_session::proxy_type proxy_server_type;
    } as_proxy_server;
  };

  struct proxy_client_info {
    proxy::client_session::proxy_type  proxy_client_type;
    proxy::destination                 proxy_address;
    proxy::credentials                 proxy_creds;
  };

  struct output_t {
    std::vector<proxy_client_info>  proxy_chain; // Can be empty
  };

  // ---

  input_t   input;
  output_t  output;
};

}