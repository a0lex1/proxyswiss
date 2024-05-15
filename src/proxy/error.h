
#pragma once

#include <boost/system/error_code.hpp>

namespace proxy {
namespace error {

class basic_error_category: public boost::system::error_category {
public:
  virtual ~basic_error_category() {}
  virtual const char* name() const BOOST_NOEXCEPT override {
    return "proxy.basic";
  }
  virtual std::string message(int ev) const override;
};

basic_error_category& get_basic_error_category();

enum basic_errors {
  protocol_violation            = 1,
  unsupported_command           = 2,
  auth_failed                   = 3,
  bad_auth_method               = 4,
  creds_too_long                = 5,
  hostname_too_long             = 6,
  line_too_long                 = 7
};

inline boost::system::error_code make_error_code(basic_errors e) {
  return boost::system::error_code(
    static_cast<int>(e), get_basic_error_category());
}

}}
