
#include "proxy/connect_response.h"

#include "common/base/str.h"

using namespace std;

namespace proxy {

string connect_response::major_code_to_string(major_code mc) {
  return common::wstr_to_str(major_code_to_wstring(mc));
}

std::wstring connect_response::major_code_to_wstring(major_code mc) {
  switch (mc) {
  case eSucceeded: return L"eSucceeded";
  case eHostUnreachable: return L"eHostUnreachable";
  case eConnectionRefused: return L"eConnectionRefused";
  case eBadAddressType: return L"eBadAddressType";
  case eUnknownError: return L"eUnknownError";
  default: return nullptr;
  }
}

bool connect_response::error_code_to_major(
  boost::system::error_code err, major_code& mc)
{
  if (err == boost::asio::error::connection_refused) {
    mc = eConnectionRefused;
    return true;
  }
  if (err == boost::asio::error::host_unreachable) {
    mc = eHostUnreachable;
    return true;
  }
  if (err == boost::asio::error::address_family_not_supported) {
    mc = eBadAddressType;
    return true;
  }
  return false;
}

}
