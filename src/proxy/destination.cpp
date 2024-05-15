
#include "proxy/destination.h"

#include "common/base/str.h"

using namespace std;

namespace proxy {

destination::destination(
  const std::string& _hostname /*= ""*/,
  boost::asio::ip::address _ip_address /*= boost::asio::ip::address()*/,
  uint16_t _port /*= 0*/)
  :
  hostname(_hostname),
  ip_address(_ip_address),
  port(_port)
{
}

string destination::to_string() const {
  return common::wstr_to_str(to_wstring());
}

wstring destination::to_wstring() const {
  if (using_hostname()) {
    return common::str_printf(L"%S:%d",
      hostname.c_str(), port);
  }
  else {
    return common::str_printf(L"%S:%d",
      ip_address.to_string().c_str(), port);
  }
}

}
