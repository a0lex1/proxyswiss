
#include "proxyswiss/server.h"
#include "proxyswiss/config_from_cmdline.h"
#include "proxyswiss/print_config.h"

#include <iostream>

#define dbgprint(...) __noop

using namespace std;
using namespace boost::asio::ip;
using boost::asio::io_context;

static void usage() {

  wstring server_types_str, client_types_str;
  map<proxy::server_session::proxy_type, wstring> server_types;
  map<proxy::client_session::proxy_type, wstring> client_types;

  proxy::enum_server_session_types(server_types);
  proxy::enum_client_session_types(client_types);

  for (auto it = server_types.begin(); it!=server_types.end(); it++) {
    server_types_str += it->second;
    auto it2 = it;
    ++it2;
    if (it2 != server_types.end()) {
      server_types_str += L", ";
    }
  }

  for (auto it = client_types.begin(); it != client_types.end(); it++) {
    client_types_str += it->second;
    auto it2 = it;
    ++it2;
    if (it2 != client_types.end()) {
      server_types_str += L", ";
    }
  }

  cout << "Usage:\n";
  cout << " proxyswiss proxy <inProxy> [proxy-chain]\n";
  cout << "OR\n";
  cout << " proxyswiss tunnel <tunIn> <tunOut> [proxy-chain]\n";
  cout << "\n";
  cout << " inProxy     => proxy-server-type://[uname:pwd@]ip:port\n";
  cout << " tunIn       => ip:port\n";
  cout << " tunOut      => host:port\n";
  cout << " proxy-chain => proxy-client-type://[uname:pwd@]host:port [, ...]\n";
  cout << "\n";
  wcout<<L"  proxy-server-type => " << server_types_str << L"\n";
  wcout<<L"  proxy-client-type => " << client_types_str << L"\n";
  cout << "\n";
  cout << " IPv6 addresses are enclosed in square brackets:\n";
  cout << "   [2001:db8:a0b:12f0::1]\n";
  cout << " ip:port requires IP address\n";
  cout << " host:port allows both host name and IP address\n";
  cout << "\n";
  cout << "Examples:\n";
  cout << "\n";
  cout << " proxyswiss tunnel 127.0.0.1:5500 1.2.3.4:5555\n";
  cout << "\n";
  cout << " proxyswiss tunnel 127.0.0.1:5500 example.com:80\n";
  cout << "   socks5://admin:123@4.5.6.7:1080\n";
  cout << "\n";
  cout << " proxyswiss proxy socks5://0.0.0.0:1080 socks5://proxy1.com:1080\n";
  cout << "   socks5://login:pass@proxy2.com:1080\n";
  cout << "\n";
}

int wmain(int argc, wchar_t* argv[]) {
  proxyswiss::config cfg;

  wstring err_msg;
  int r = config_from_cmdline(argc-1, &argv[1], cfg, err_msg);
  if (r == 1) {
    if (!err_msg.empty()) {
      wcout << L"Error: " << err_msg << L"\n";
    }
    return usage(), 1;
  }
  if (r != 0) {
    wcout << L"Error: " << err_msg << L"\n";
    return r;
  }

  wstringstream ss;
  print_config(cfg, ss);
  wcout << L"Config:\n" << ss.str() << L"\n";

  io_context ioc;
  proxyswiss::server srv(ioc, cfg);

  srv.enable_print_proxy_errors(true);
  //srv.enable_logging(L".\\server_connect_log.txt");

  boost::system::error_code err;
  if (!srv.open(err)) {
    cout << "Can't open server on " <<
      cfg.input.listen_addr.address().to_string() <<
      ":" << cfg.input.listen_addr.port() << "\n";
    return -1;
  }

  cout << "Running...\n";

  srv.start();
  ioc.run();

  return 0;
}
