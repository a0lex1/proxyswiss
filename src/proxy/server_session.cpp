
#include "proxy/server_session.h"

#include "proxy/detail/server_session_socks5.h"
#include "proxy/detail/server_session_https.h"

#include <assert.h>

using namespace std;

namespace proxy {

server_session* create_server_session(
  boost::asio::ip::tcp::socket& sock, server_session::proxy_type type,
  const string& dbglog_uid)
{
  server_session* ret = nullptr;
  switch (type) {
  case server_session::eSocks5:
    ret = new detail::server_session_socks5(sock);
    break;
  case server_session::eHttps:
    ret = new detail::server_session_https(sock);
    break;
  default:
    assert(0);
    break;
  }
  ret->dbglog_uid_ = dbglog_uid;
  return ret;
}

void enum_server_session_types(
  map<server_session::proxy_type, wstring>& type_name_map)
{
  type_name_map.clear();
  type_name_map[server_session::eSocks5] = L"socks5";
  type_name_map[server_session::eHttps] = L"https";
}

bool server_session_type_from_string(
  const wstring& str,
  server_session::proxy_type& pt)
{
  map<server_session::proxy_type, wstring> server_types;
  enum_server_session_types(server_types);
  for (auto it=server_types.begin(); it!=server_types.end(); it++) {
    if (it->second == str) {
      pt = it->first;
      return true;
    }
  }
  return false;
}

wstring server_session_type_to_string(server_session::proxy_type pt) {
  map<server_session::proxy_type, wstring> server_types;
  enum_server_session_types(server_types);
  auto it = server_types.find(pt);
  if (it != server_types.end()) {
    return it->second.c_str();
  }
  return nullptr;
}

}
