
#include "proxyswiss/config_from_cmdline.h"

#include "common/net/url_parser.h"
#include "common/base/str.h"

#include <assert.h>
#include <iostream>

using namespace std;
using namespace boost::asio::ip;
using boost::asio::io_context;
using common::net::url_parser_w;
using common::net::host_address;
using common::net::ip_address;

using common::str_printf;

static bool endpoint_from_string(const wstring& str, tcp::endpoint& ep,
  wstring& err_msg)
{
  url_parser_w up;
  // Allow only:
  //  1) IPv4:port
  //  2) IPv6:port
  static const wstring kBadMsgText = L"bad ip:port format";
  if (!up.parse(str)) {
    err_msg = kBadMsgText;
    return false;
  }
  if (up.scheme()) {
    err_msg = kBadMsgText + L", no scheme:// is allowed";
    return false;
  }
  if (!up.path().empty()) {
    err_msg = kBadMsgText + L", no /path is allowed";
    return false;
  }
  if (up.username() || up.password()) {
    err_msg = kBadMsgText + L", no username/password is allowed";
    return false;
  }
  if (!up.port()) {
    err_msg = kBadMsgText + L", port is required";
    return false;
  }
  if (up.host().type() == host_address::eHostname) {
    err_msg = kBadMsgText + L", only ipv4/ipv6, no hostnames";
    return false;
  }
  if (up.host().ip_addr().type() == ip_address::eIPv4) {
    ep = tcp::endpoint(
        address_v4(htonl(up.host().ip_addr().ipv4())),
        *up.port());
  }
  else {
    assert(up.host().ip_addr().type() == ip_address::eIPv6);
    const uint8_t* r = up.host().ip_addr().ipv6();
    std::array<uint8_t, 16> arr = {
      r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9], r[10],
      r[11], r[12], r[13], r[14], r[15]
    };
    ep = tcp::endpoint(address_v6(arr), *up.port());
  }
  return true;
}

static bool proxy_info_from_string(const wstring& str,
  proxy::destination& dst,
  wstring* pproxy_type,       //< if nullptr, disallow using scheme://
  proxy::credentials* pcreds, //< if nullptr, disallow using uname:pwd@
  wstring& err_msg)
{
  url_parser_w up;
  // Allow:
  //  1) IPv4:port
  //  2) IPv6:port
  //  3) hostname:port
  static const wstring kBadMsgText = L"bad format";
  if (!up.parse(str)) {
    err_msg = kBadMsgText;
    return false;
  }
  if (!up.path().empty()) {
    err_msg = kBadMsgText + L", /path is not allowed";
    return false;
  }
  if (pproxy_type && !up.scheme()) {
    err_msg = kBadMsgText + L", proxy_type:// is required";
    return false;
  }
  if (!pproxy_type && up.scheme()) {
    err_msg = kBadMsgText + L", no proxy_type:// is not allowed";
    return false;
  }
  if (!pcreds && (up.username() || up.password())) {
    err_msg = kBadMsgText + L", no username:pass@ is allowed";
    return false;
  }
  if (!up.port()) {
    err_msg = kBadMsgText + L", port is required";
    return false;
  }

  // TODO: replace the following block with proxy::destination_from_url_parser
  if (up.host().type() == host_address::eIPAddress) {
    if (up.host().ip_addr().type() == ip_address::eIPv4) {
      dst.ip_address = address_v4(htonl(up.host().ip_addr().ipv4()));
      dst.port = *up.port();
      dst.hostname = "";
      assert(!dst.using_hostname());
    }
    else {
      assert(up.host().ip_addr().type() == ip_address::eIPv6);
      dst.ip_address = address_v6(up.host().ip_addr().ipv6_array());
      dst.port = *up.port();
      dst.hostname = "";
      assert(!dst.using_hostname());
    }
  }
  else {
    assert(up.host().type() == host_address::eHostname);
    dst.hostname = up.host().hostname();
    dst.port = *up.port();
    assert(dst.using_hostname());
  }
  // ----------------------------------------------------------------------
  if (pproxy_type) {
    *pproxy_type = *up.scheme();
  }
  if (pcreds) {
    if (up.username() && up.password()) {
      pcreds->username = common::wstr_to_str(*up.username());
      pcreds->password = common::wstr_to_str(*up.password());
    }
    else {
      pcreds->username = pcreds->password = "";
    }
  }
  return true;
}

int config_from_cmdline(
  int                  fc,
  wchar_t*             fv[],
  proxyswiss::config&  cfg,
  std::wstring&        err_msg)
{
  if (fc < 2) {
    return 1;
  }

  wstring sub_err_msg;
  wstring proxy_type_str;
  proxy::server_session::proxy_type srv_type;
  proxy::client_session::proxy_type cli_type;
  proxy::destination host;
  proxy::credentials creds;
  int fchain;

  wstring inType(fv[0]);
  if (inType == L"tunnel") {
    if (fc < 3) {
      return 1;
    }
    cfg.input.type = proxyswiss::config::eTunnel;
    if (!endpoint_from_string(fv[1], cfg.input.listen_addr, sub_err_msg)) {
      err_msg = str_printf(L"Can't parse tunIn(%s): %s",
        fv[1],
        sub_err_msg.c_str());
      return -1;
    }
    if (!proxy_info_from_string(fv[2], cfg.input.as_tunnel.destination,
      nullptr, nullptr, //< disallow scheme:// and uname:pwd@
      sub_err_msg))
    {
      err_msg = str_printf(L"Can't parse tunOut(%s): %s",
        fv[2],
        sub_err_msg.c_str());
      return -1;
    }
    fchain = 3;
  }
  else {
    if (inType == L"proxy") {
      cfg.input.type = proxyswiss::config::eProxyServer;
      if (!proxy_info_from_string(fv[1], host, &proxy_type_str, nullptr,
        sub_err_msg))
      {
        err_msg = str_printf(L"Can't parse inProxy(%s): %s",
          fv[1],
          sub_err_msg.c_str());
        return -1;
      }
      if (!proxy::server_session_type_from_string(proxy_type_str,
        srv_type))
      {
        err_msg = L"Bad proxy type in inProxy";
        return -1;
      }
      // Allow IP only
      if (host.using_hostname()) {
        err_msg = L"Only IP addresses are allowed in inProxy";
        return -1;
      }
      cfg.input.as_proxy_server.proxy_server_type = srv_type;
      cfg.input.listen_addr = tcp::endpoint(host.ip_address,
        host.port);
      fchain = 2;
    }
    else {
      err_msg = L"Unknown inType";
      return -1;
    }
  }

  for (int i = fchain; i < fc; i++) {
    if (!proxy_info_from_string(fv[i], host, &proxy_type_str, &creds,
      sub_err_msg))
    {
      err_msg = str_printf(L"Can't parse proxy-chain[%d] (%s): %s", i,
        fv[i],
        sub_err_msg.c_str());
      return -1;
    }
    if (!proxy::client_session_type_from_string(proxy_type_str, cli_type))
    {
      err_msg = str_printf(
        L"Bad proxy type (%s) in proxy-chain[%d]",
        proxy_type_str.c_str(), i);
      return -1;
    }
    // Allow hostnames
    proxyswiss::config::proxy_client_info chain_entry;
    chain_entry.proxy_client_type = cli_type;
    chain_entry.proxy_address = host;
    chain_entry.proxy_creds = creds;
    cfg.output.proxy_chain.push_back(chain_entry);
  }

  return 0;
}
