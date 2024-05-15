
#include "proxy/destination_from_url_parser.h"

#include "common/base/str.h"

namespace proxy {

// Instantiate.

template void destination_from_url_parser<char>(
  proxy::destination& dst,
  const common::net::host_address& host, uint16_t port);

template void destination_from_url_parser<wchar_t>(
  proxy::destination& dst,
  const common::net::host_address& host, uint16_t port);

// ---

template <typename T>
void destination_from_url_parser(proxy::destination& dst,
  const typename common::net::host_address& host, uint16_t port)
{
  typedef typename common::net::url_parser_T<T> UP;

  if (host.type() == common::net::host_address::eIPAddress) {
    if (host.ip_addr().type() == common::net::ip_address::eIPv4) {
      dst.ip_address = boost::asio::ip::address_v4(htonl(host.ip_addr().ipv4()));
      dst.port = port;
      dst.hostname = "";
      assert(!dst.using_hostname());
    }
    else {
      assert(host.ip_addr().type() == common::net::ip_address::eIPv4);
      dst.ip_address = boost::asio::ip::address_v6(host.ip_addr().ipv6_array());
      dst.port = port;
      dst.hostname = "";
      assert(!dst.using_hostname());
    }
  }
  else {
    assert(host.type() == common::net::host_address::eHostname);
    dst.hostname = common::tstr_to_tstr<char>(
      host.hostname());
    dst.port = port;
    assert(dst.using_hostname());
  }
}

}
