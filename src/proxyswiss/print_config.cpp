
#include "proxyswiss/print_config.h"
#include "proxy/server_session.h"

#include "common/base/str.h"

#include <assert.h>
#include <iostream>

using namespace std;

#define wstr_to_str common::wstr_to_str
#define str_to_wstr common::str_to_wstr

void print_config(const proxyswiss::config& cfg, wstringstream& output) {

  wstringstream& o(output);

  o << L"Input:\n";
  o << L" Listen address: " <<
    str_to_wstr(cfg.input.listen_addr.address().to_string()) << ":" <<
    cfg.input.listen_addr.port() << "\n";

  switch (cfg.input.type) {
  case proxyswiss::config::eTunnel:
    o << L" Type: Tunnel to " <<
      cfg.input.as_tunnel.destination.to_wstring() << L"\n";
    break;
  case proxyswiss::config::eProxyServer:
    o << L" Type: ProxyServer " <<
      proxy::server_session_type_to_string(
        cfg.input.as_proxy_server.proxy_server_type) << "\n";
    break;
  default:
    assert(0);
    return;
  }

  if (cfg.output.proxy_chain.empty()) {
    o << L"Output proxy chain is empty.\n";
    return;
  }

  o << L"Output proxy chain:\n";

  const proxyswiss::config::proxy_client_info* pci;
  for (size_t i=0; i<cfg.output.proxy_chain.size(); i++) {
    pci = &cfg.output.proxy_chain[i];
    if (pci->proxy_creds.empty()) {
      o << L" #" << dec << i << L". " <<
        proxy::client_session_type_to_string(pci->proxy_client_type)
          << L"://" <<
        pci->proxy_address.to_wstring() << L"\n";
    }
    else {
      o << L" #" << dec << i << L". " <<
        proxy::client_session_type_to_string(pci->proxy_client_type) <<
          L"://" <<
        str_to_wstr(pci->proxy_creds.username) << L":" <<
        str_to_wstr(pci->proxy_creds.password) << L"@" <<
        pci->proxy_address.to_wstring() << L"\n";
    }
  }
}
