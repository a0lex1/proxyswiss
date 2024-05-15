
#pragma once

#include <string>

#include <stdint.h>

namespace common {

class bin_writer {
public:
  bin_writer(std::string& buf);
  bin_writer(std::string& buf, size_t max_to_write);
  bin_writer(void* buf_ptr, size_t max_to_write);

  bool write_byte_array(const void* bytes_ptr, uint32_t bytes_len);
  bool write_byte_array16(const void* bytes_ptr, uint16_t bytes_len);

  template <typename I>
  inline bool write_int(I i) {
    return _write_raw(&i, sizeof(i));
  }

  bool write_int64(int64_t v) { return write_int(v); }
  bool write_uint64(uint64_t v) { return write_int(v); }
  bool write_int32(int32_t v) { return write_int(v); }
  bool write_uint32(uint32_t v) { return write_int(v); }
  bool write_int16(int16_t v) { return write_int(v); }
  bool write_uint16(uint16_t v) { return write_int(v); }
  bool write_int8(int8_t v) { return write_int(v); }
  bool write_uint8(uint8_t v) { return write_int(v); }

  bool write_string(const wchar_t* str);
  bool write_string(const char* str);
  bool write_string(const std::wstring& str);
  bool write_string(const std::string& str);
  bool write_char(char c) { return write_int(static_cast<uint8_t>(c)); }
  bool write_char(wchar_t c) { return write_int(static_cast<uint16_t>(c)); }

  size_t space_left() const;

  template <typename C>
  bool write_basic_string(const std::basic_string<C>& str) {
    return _write_array_with_32bit_size(str.c_str(),
      static_cast<uint32_t>(str.length()));
  }

  // ---

  bool _write_raw(const void* raw_ptr, uint32_t raw_len);

  template <typename X>
  bool _write_with_size(const X& x) {
    uint32_t sz = sizeof(x);
    if (space_left() < sz+sizeof(sz)) {
      return false;
    }
    bool written;
    written = _write_raw(&sz, sizeof(sz));
    written = _write_raw(&x, sz);
    return true;
  }

  template <typename X>
  bool _write_array_with_32bit_size(const X* arr, uint32_t num_entries) {
    return _write_array_with_Xzibit_size<X, uint32_t>(arr, num_entries);
  }

private:
  template <typename X, typename SizeType>
  bool _write_array_with_Xzibit_size(const X* arr, SizeType num_entries) {
    SizeType sz = sizeof(X) * num_entries;
    if (space_left() < sz+sizeof(sz)) {
      return false;
    }
    bool written;
    written = _write_raw(&sz, sizeof(sz));
    written = _write_raw(arr, sz);
    return true;
  }

private:
  size_t       cur_pos_;
  size_t       max_to_write_;
  std::string* pstr_buf_;
  union {
    void* as_ptr;
    char* as_char;
  } raw_buf_;
};

}
