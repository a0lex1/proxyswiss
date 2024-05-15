
#pragma once

#include "common/net/host_address.h"

#include <string>
#include <stdint.h>

namespace common {
namespace net {

template <typename T>
class url_parser_T {
public:
  typedef std::basic_string<T> StringT;

  bool parse(const StringT& url);

  const StringT*            scheme()   const { return pscheme_; }
  const StringT*            username() const { return pusername_; }
  const StringT*            password() const { return ppassword_; }
  const net::host_address&  host()     const { return host_; }
  const uint16_t*           port()     const { return pport_; }
  const StringT&            path()     const { return path_; }

private:
  StringT             scheme_;
  const StringT*      pscheme_;
  StringT             username_;
  StringT             password_;
  const StringT*      pusername_;
  const StringT*      ppassword_;
  net::host_address   host_;
  uint16_t            port_;
  const uint16_t*     pport_;
  StringT             path_;
};

typedef url_parser_T<char> url_parser;
typedef url_parser_T<wchar_t> url_parser_w;

}}
