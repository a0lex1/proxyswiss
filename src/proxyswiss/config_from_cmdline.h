
#include "proxyswiss/config.h"

#include <string>

int config_from_cmdline(int fc, wchar_t* fv[],
  proxyswiss::config& cfg, std::wstring& err_msg);
