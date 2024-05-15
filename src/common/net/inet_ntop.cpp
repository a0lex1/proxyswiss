
#include <ws2tcpip.h>

#include "common/net/inet_ntop.h"
#include "common/base/str.h"

#include <stdio.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

namespace common {
namespace net {

const char* inet_ntop(
  int af, const void* src, char* dest, size_t length,
  unsigned long scope_id, unsigned* pwin32err)
{
  const unsigned char* bytes = static_cast<const unsigned char*>(src);
  if (af == AF_INET)
  {
    sprintf_s(dest, length, "%u.%u.%u.%u",
              bytes[0], bytes[1], bytes[2], bytes[3]);
    return dest;
  }
  else if (af == AF_INET6)
  {
    size_t n = 0, b = 0, z = 0;
    while (n < length && b < 16)
    {
      if (bytes[b] == 0 && bytes[b + 1] == 0 && z == 0)
      {
        do b += 2; while (b < 16 && bytes[b] == 0 && bytes[b + 1] == 0);
        n += sprintf_s(dest + n, length - n, ":%s", b < 16 ? "" : ":"), ++z;
      }
      else
      {
        n += sprintf_s(dest + n, length - n, "%s%x", b ? ":" : "",
          (static_cast<unsigned long>(bytes[b]) << 8) | bytes[b + 1]);
        b += 2;
      }
    }
    if (scope_id)
      n += sprintf_s(dest + n, length - n, "%%%lu", scope_id);
    return dest;
  }
  else
  {
    if (pwin32err) {
      *pwin32err = WSAEAFNOSUPPORT;
    }
    return 0;
  }
}

const wchar_t* inet_ntop(
  int af, const void* src, wchar_t* dest, size_t length,
  unsigned long scope_id, unsigned* pwin32err)
{
  if (!length) {
    return nullptr;
  }
  char* dest_a = new char [length];
  bool ok =
    inet_ntop(af, src, dest_a, length, scope_id, pwin32err)
    != nullptr;
  if (ok) {
    wstring buf_w;
    buf_w = str_to_wstr(dest_a);
    //common::str_mb_to_widechar(buf_w, dest_a, common::str::cp_acp);
#pragma warning(push)
#pragma warning(disable:4996)
    wcscpy(dest, buf_w.c_str());
#pragma warning(pop)
  }
  delete [] dest_a;
  if (ok) {
    return dest;
  }
  else {
    return nullptr;
  }
}

}}
