
#pragma once

#include "common/base/str.h"

#include <vector>
#include <string>
#include <memory>

#include <stdio.h>

namespace proxyswiss {
namespace detail {

class debug_uid;
class debug_uid_table {
private:
  class core;

  class handle {
  public:
    std::string to_string() const {
      return common::str_from_uint(v_);
    }
  private:
    friend class core;
    handle(unsigned v): v_(v)
    {
    }

    unsigned v_;
  };

  class core {
  public:
    handle alloc() {
      for (unsigned i=0; i<0xffffffff; i++) {
        if (std::find(table_.begin(), table_.end(), i) == table_.end()) {
          table_.push_back(i);
          return handle(i);
        }
      }
      return handle(-1);
    }
    void free(handle h) {
      auto it = std::find(table_.begin(), table_.end(), h.v_);
      if (it != table_.end()) {
        table_.erase(it);
      }
    }

  private:
    std::vector<unsigned> table_;
  };

public:
  debug_uid_table() {
    core_sptr_.reset(new core);
  }

private:
  friend class debug_uid;

  handle alloc() {
    return core_sptr_->alloc();
  }
  void free(handle h) {
    return core_sptr_->free(h);
  }

private:
  std::shared_ptr<core> core_sptr_;
};

}}
