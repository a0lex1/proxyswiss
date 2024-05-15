
#pragma once

namespace common {

void* bin_scan(const void* mem, size_t mem_len,
               const void* pat, size_t pat_len);

}
