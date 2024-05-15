
#include "common/base/bin_writer.h"

namespace common {

bin_writer::bin_writer(std::string& buf) {
  cur_pos_ = 0;
  pstr_buf_ = &buf;
  max_to_write_ = -1;
}

bin_writer::bin_writer(std::string& buf, size_t max_to_write) {
  cur_pos_ = 0;
  pstr_buf_ = &buf;
  max_to_write_ = max_to_write;
}

bin_writer::bin_writer(void* buf_ptr, size_t max_to_write) {
  cur_pos_ = 0;
  pstr_buf_ = 0;
  raw_buf_.as_ptr = buf_ptr;
  max_to_write_ = max_to_write;
}

size_t bin_writer::space_left() const {
  if (max_to_write_ == -1) {
    return -1;
  } else {
    return max_to_write_ - cur_pos_;
  }
}

bool bin_writer::_write_raw(const void* raw_ptr, uint32_t raw_len) {
  if (space_left() < raw_len) {
    return false;
  }
  if (pstr_buf_ != 0) {
    pstr_buf_->append(static_cast<const char*>(raw_ptr), raw_len);
  } else {
    memcpy(&raw_buf_.as_char[cur_pos_], raw_ptr, raw_len);
  }
  cur_pos_ += raw_len;
  return true;
}

bool bin_writer::write_byte_array(const void* bytes_ptr, uint32_t bytes_len) {
  const uint8_t* bytes_ptr_uchar = static_cast<const uint8_t*>(bytes_ptr);
  return _write_array_with_32bit_size(bytes_ptr_uchar, bytes_len);
}

bool bin_writer::write_byte_array16(const void* bytes_ptr, uint16_t bytes_len) {
  const uint8_t* bytes_ptr_uchar = static_cast<const uint8_t*>(bytes_ptr);
  return _write_array_with_Xzibit_size<uint8_t, uint16_t>(bytes_ptr_uchar,
                                                          bytes_len);
}

bool bin_writer::write_string(const wchar_t* str) {
  return _write_array_with_32bit_size(str, static_cast<uint32_t>(wcslen(str)));
}
bool bin_writer::write_string(const char* str) {
  return _write_array_with_32bit_size(str, static_cast<uint32_t>(strlen(str)));
}
bool bin_writer::write_string(const std::wstring& str) {
  return write_basic_string(str);
}
bool bin_writer::write_string(const std::string& str) {
  return write_basic_string(str);
}

}
