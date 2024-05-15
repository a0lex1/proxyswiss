
#pragma once

namespace common {
namespace net {

const char* inet_ntop(
  int            af,                  // AF_INET or AF_INET6
  const void*    src,                 // in_addr or in6_addr
  char*          dest,
  size_t         length,
  unsigned long  scope_id = 0,
  unsigned*      pwin32err = nullptr  // WSAEXXX
  );

const wchar_t* inet_ntop(
  int            af,
  const void*    src,
  wchar_t*       dest,
  size_t         length,
  unsigned long  scope_id = 0,
  unsigned*      pwin32err = nullptr);

}}
