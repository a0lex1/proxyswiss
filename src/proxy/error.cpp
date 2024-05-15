
#include "proxy/error.h"

using namespace std;

namespace proxy {
namespace error {

basic_error_category& get_basic_error_category() {
  static basic_error_category inst;
  return inst;
}

string basic_error_category::message(int ev) const {
  switch (ev) {

  case protocol_violation:      return "Protocol violation";
  case unsupported_command:     return "Unsupported command has been received";
  case auth_failed:             return "Authentication has been failed";
  case bad_auth_method:         return "Bad authentication method";
  case creds_too_long:          return "An element of credentials is too long";
  case hostname_too_long:       return "Hostname is too long";
  case line_too_long:           return "Line is too long";

  default: return string(name()) + " error"; // "proxy.basic error"
  }
}

}}
