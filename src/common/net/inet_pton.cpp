
#include <ws2tcpip.h>

#include "common/net/inet_pton.h"
#include "common/base/str.h"

using namespace std;

namespace common {
namespace net {

// from boost_1_63_0\boost\asio\detail\impl\socket_ops.ipp
int inet_pton(int af, const char* src, void* dest)
{
  unsigned char* bytes = static_cast<unsigned char*>(dest);
  if (af == AF_INET)
  {
    unsigned int b0, b1, b2, b3;
    if (sscanf_s(src, "%u.%u.%u.%u", &b0, &b1, &b2, &b3) != 4)
    {
      //ec = boost::asio::error::invalid_argument;
      return -1;
    }
    if (b0 > 255 || b1 > 255 || b2 > 255 || b3 > 255)
    {
      //ec = boost::asio::error::invalid_argument;
      return -1;
    }
    bytes[0] = static_cast<unsigned char>(b0);
    bytes[1] = static_cast<unsigned char>(b1);
    bytes[2] = static_cast<unsigned char>(b2);
    bytes[3] = static_cast<unsigned char>(b3);
    //ec = boost::system::error_code();
    return 1;
  }
  else if (af == AF_INET6)
  {
    unsigned char* bytes = static_cast<unsigned char*>(dest);
    std::memset(bytes, 0, 16);
    unsigned char back_bytes[16] = { 0 };
    int num_front_bytes = 0, num_back_bytes = 0;
    const char* p = src;

    enum { fword, fcolon, bword, scope, done } state = fword;
    unsigned long current_word = 0;
    while (state != done)
    {
      if (current_word > 0xFFFF)
      {
        //ec = boost::asio::error::invalid_argument;
        return -1;
      }

      switch (state)
      {
      case fword:
        if (*p >= '0' && *p <= '9')
          current_word = current_word * 16 + *p++ - '0';
        else if (*p >= 'a' && *p <= 'f')
          current_word = current_word * 16 + *p++ - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F')
          current_word = current_word * 16 + *p++ - 'A' + 10;
        else
        {
          if (num_front_bytes == 16)
          {
            //ec = boost::asio::error::invalid_argument;
            return -1;
          }

          bytes[num_front_bytes++] = (current_word >> 8) & 0xFF;
          bytes[num_front_bytes++] = current_word & 0xFF;
          current_word = 0;

          if (*p == ':')
            state = fcolon, ++p;
          else if (*p == '%')
            state = scope, ++p;
          else if (*p == 0)
            state = done;
          else
          {
            //ec = boost::asio::error::invalid_argument;
            return -1;
          }
        }
        break;

      case fcolon:
        if (*p == ':')
          state = bword, ++p;
        else
          state = fword;
        break;

      case bword:
        if (*p >= '0' && *p <= '9')
          current_word = current_word * 16 + *p++ - '0';
        else if (*p >= 'a' && *p <= 'f')
          current_word = current_word * 16 + *p++ - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F')
          current_word = current_word * 16 + *p++ - 'A' + 10;
        else
        {
          if (num_front_bytes + num_back_bytes == 16)
          {
            //ec = boost::asio::error::invalid_argument;
            return -1;
          }

          back_bytes[num_back_bytes++] = (current_word >> 8) & 0xFF;
          back_bytes[num_back_bytes++] = current_word & 0xFF;
          current_word = 0;

          if (*p == ':')
            state = bword, ++p;
          else if (*p == '%')
            state = scope, ++p;
          else if (*p == 0)
            state = done;
          else
          {
            //ec = boost::asio::error::invalid_argument;
            return -1;
          }
        }
        break;

      case scope:
        if (*p >= '0' && *p <= '9')
          current_word = current_word * 10 + *p++ - '0';
        else if (*p == 0) {
          //          *scope_id = current_word, state = done;
        } else
        {
          //ec = boost::asio::error::invalid_argument;
          return -1;
        }
        break;

      default:
        break;
      }
    }

    for (int i = 0; i < num_back_bytes; ++i)
      bytes[16 - num_back_bytes + i] = back_bytes[i];

    //    ec = boost::system::error_code();
    return 1;
  }
  else
  {
    //    ec = boost::asio::error::address_family_not_supported;
    return -1;
  }
}

int inet_pton(int af, const wchar_t* src, void* dest) {
  return inet_pton(af, common::wstr_to_str(src).c_str(), dest);
}

}}
