
#include "proxy/client_session.h"

#include "proxy/detail/client_session_socks5.h"

#include <assert.h>

using namespace std;

namespace proxy {

client_session* create_client_session(
  boost::asio::ip::tcp::socket& sock,
  client_session::proxy_type type,
  const credentials& creds,
  const string& dbglog_uid)
{
  client_session* ret = nullptr;
  switch (type) {
  case client_session::eSocks5:
    ret = new detail::client_session_socks5(sock, creds);
    break;
  default:
    assert(0);
    break;
  }
  ret->dbglog_uid_ = dbglog_uid;
  return ret;
}

void enum_client_session_types(
  map<client_session::proxy_type, wstring>& type_name_map)
{
  type_name_map.clear();
  type_name_map[client_session::eSocks5] = L"socks5";
}


bool client_session_type_from_string(
  const wstring& str,
  client_session::proxy_type& pt)
{
  map<client_session::proxy_type, wstring> client_types;
  enum_client_session_types(client_types);
  for (auto it=client_types.begin(); it!=client_types.end(); it++) {
    if (it->second == str) {
      pt = it->first;
      return true;
    }
  }
  return false;
}

wstring client_session_type_to_string(client_session::proxy_type pt) {
  map<client_session::proxy_type, wstring> client_types;
  enum_client_session_types(client_types);
  auto it = client_types.find(pt);
  if (it != client_types.end()) {
    return it->second.c_str();
  }
  return nullptr;
}

}
