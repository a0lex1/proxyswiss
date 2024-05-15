
#pragma once

#include <boost/asio/error.hpp>

namespace proxy {

struct connect_response {
  enum major_code {
    eSucceeded,
    eHostUnreachable,
    eConnectionRefused,
    eBadAddressType,
    eUnknownError
  };

  major_code major;

  connect_response(major_code _major = eUnknownError): major(_major)
  {
  }

  static std::string major_code_to_string(major_code mc);
  static std::wstring major_code_to_wstring(major_code mc);

  static bool error_code_to_major(
     boost::system::error_code err, major_code& mc);
};

}
