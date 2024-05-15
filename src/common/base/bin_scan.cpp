
#include "common/base/bin_scan.h"

#include <memory.h>

namespace common {

void* bin_scan(const void* mem, size_t mem_len,
               const void* pat, size_t pat_len)
{
  if (mem_len < pat_len) {
    return 0;
  }
  const char* memc = static_cast<const char*>(mem);
  size_t max_to_scan = mem_len-pat_len;
  for (size_t i=0; i<=max_to_scan; i++) {
    if (!memcmp(&memc[i], pat, pat_len)) {
      return const_cast<char*>(&memc[i]);
    }
  }
  return 0;
}

}
