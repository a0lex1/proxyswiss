
#pragma once

namespace common {
namespace net {

int inet_pton(
  int             af,   // AF_INET or AF_INET6
  const char*     src,
  void*           dest  // in_addr or in6_addr
  );

int inet_pton(
  int             af,
  const wchar_t*  src,
  void*           dest
  );

}}
