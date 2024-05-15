
#pragma once

#include "proxyswiss/detail/debug_uid_table.h"

namespace proxyswiss {
namespace detail {

class debug_uid {
public:
  debug_uid(debug_uid_table& t)
    : core_sptr_(t.core_sptr_), h_(core_sptr_->alloc())
  {
  }
  ~debug_uid() {
    core_sptr_->free(h_);
  }
  std::string to_string() const {
    return h_.to_string();
  }
private:
  std::shared_ptr<debug_uid_table::core> core_sptr_;
  debug_uid_table::handle h_;
};

}}
