#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

#include "common/common.h"

namespace common {

namespace str {
// TODO: Remove unused in str::
static const size_t kPrintfBufSize;
static const unsigned kRadixDec10 = 10;
static const unsigned kRadixHex16 = 16;
enum code_page {
  cp_acp = 0,       // CP_ACP, default to ANSI code page
  cp_utf8 = 65001   // CP_UTF8, UTF-8 translation
};
static const size_t kGuidStrLen = 38;
static const size_t kGuidStrBufLen = kGuidStrLen+1;
} // namespace str

// Convertion.
template <typename DestChar, typename SourceChar>
static inline DestChar tchar_to_tchar(SourceChar ansi_char) {
  return static_cast<DestChar>(ansi_char);
}
static inline char wchar_to_char(wchar_t c) {
  return tchar_to_tchar<char, wchar_t>(c);
}
static inline wchar_t char_to_wchar(char c) {
  return tchar_to_tchar<wchar_t, char>(c);
}

template <typename DestChar, typename SourceChar>
static std::basic_string<DestChar> tstr_to_tstr(const std::basic_string<SourceChar>& str) {
  std::basic_string <DestChar> ret;
  std::transform(str.begin(), str.end(), std::back_inserter(ret), [](wchar_t c) {
    return (char)c;
    });

  return ret;
}
static inline std::wstring str_to_wstr(const std::string& subject) {
  return tstr_to_tstr<wchar_t, char>(subject);
}
static inline std::string wstr_to_str(const std::wstring& subject) {
  return tstr_to_tstr<char, wchar_t>(subject);
}

template <typename C> static C char_to_tchar(char c) {
  return tchar_to_tchar<C, char>(c);
}
template <typename C> static C wchar_to_tchar(wchar_t c) {
  return tchar_to_tchar<C, wchar_t>(c);
}

template <typename C> static std::basic_string<C> str_to_tstr(
  const std::string& s)
{
  return tstr_to_tstr<C, char>(s);
}
template <typename C> static std::basic_string<C> wstr_to_tstr(
  const std::wstring& s)
{
  return tstr_to_tstr<C, wchar_t>(s);
}

template <typename Char>
static size_t strlen_T(const Char* str);

inline static bool is_ascii_graphic_char(wchar_t c) {
  return c >= 0x20 /* space */ && c <= 0x7e /* ~ */;
}
inline static bool is_ascii_graphic_char(char c) {
  return is_ascii_graphic_char(static_cast<wchar_t>(c));
}

// Integers.

// to int
bool str_to_int(const std::string& subject, int& val, unsigned radix = str::kRadixDec10);
bool str_to_int(const std::wstring& subject, int& val, unsigned radix = str::kRadixDec10);
bool str_is_int(const std::string& subject, unsigned radix = str::kRadixDec10);
bool str_is_int(const std::wstring& subject, unsigned radix = str::kRadixDec10);

// to uint
static bool str_to_uint(const std::string& subject, unsigned int& val, unsigned radix = str::kRadixDec10, bool allow_negative = false) {
  DCHECK(radix == 10 || radix == 16);
  char* endptr;
  uint32_t l = strtoul(subject.c_str(), &endptr, radix);
  if (endptr != &subject[strlen(subject.c_str())]) {
    return false;
  }
  val = l;
  return true;
}
static bool str_to_uint(const std::wstring& subject, unsigned int& val, unsigned radix = str::kRadixDec10, bool allow_negative = false) {
  DCHECK(radix == 10 || radix == 16);
  wchar_t* endptr;
  uint32_t l = wcstoul(subject.c_str(), &endptr, radix);
  if (endptr != &subject[wcslen(subject.c_str())]) {
    return false;
  }
  val = l;
  return true;
}
bool str_is_uint(const std::string& subject, unsigned radix = str::kRadixDec10, bool allow_negative = false);
bool str_is_uint(const std::wstring& subject, unsigned radix = str::kRadixDec10, bool allow_negative = false);

// to int64
bool str_to_int64(const std::string& subject, long long& val, unsigned radix = str::kRadixDec10);
bool str_to_int64(const std::wstring& subject, long long& val, unsigned radix = str::kRadixDec10);
bool str_is_int64(const std::string& subject, unsigned radix = str::kRadixDec10);
bool str_is_int64(const std::wstring& subject, unsigned radix = str::kRadixDec10);

// to uint64
bool str_to_uint64(const std::string& subject, unsigned long long& val, unsigned radix = str::kRadixDec10, bool allow_negative = false);
bool str_to_uint64(const std::wstring& subject, unsigned long long& val, unsigned radix = str::kRadixDec10, bool allow_negative = false);
bool str_is_uint64(const std::string& subject, unsigned radix = str::kRadixDec10, bool allow_negative = false);
bool str_is_uint64(const std::wstring& subject, unsigned radix = str::kRadixDec10, bool allow_negative = false);

// --------------------------------------------------------

// Format print.
std::string  str_printf(const char* fmt, ...);
std::string  str_printf(const char* fmt, va_list vl);
std::wstring str_printf(const wchar_t* fmt, ...);
std::wstring str_printf(const wchar_t* fmt, va_list vl);

std::string  str_printf(const std::string fmt, ...);
std::wstring str_printf(const std::wstring fmt, ...);

static __inline std::string str_printf(const std::string fmt, va_list vl) {
  return str_printf(fmt.c_str(), vl);
}

static __inline std::wstring str_printf(const std::wstring fmt, va_list vl) {
  return str_printf(fmt.c_str(), vl);
}


// from int
std::string str_from_int(int val, unsigned radix = str::kRadixDec10);
std::wstring wstr_from_int(int val, unsigned radix = str::kRadixDec10);

// from uint
static std::string str_from_uint(unsigned int val, unsigned radix = str::kRadixDec10) {
  if (radix == 10) {
    return str_printf("%d", val);
  }
  else if (radix == 16) {
    return str_printf("%I64u", val);
  }
  else { NOTREACHED(); }
}
std::wstring wstr_from_uint(unsigned int val, unsigned radix = str::kRadixDec10);

// from int64
std::string str_from_int64(long long val, unsigned radix = str::kRadixDec10);
std::wstring wstr_from_int64(long long val, unsigned radix = str::kRadixDec10);

// from uint64
std::string str_from_uint64(unsigned long long val, unsigned radix = str::kRadixDec10);
std::wstring wstr_from_uint64(unsigned long long val, unsigned radix = str::kRadixDec10);



void str_split(const std::string& subject, const std::string& delimiter, std::vector<std::string>& parts);
void str_split(const std::wstring& subject, const std::wstring& delimiter, std::vector<std::wstring>& parts);

void str_split(const char* subj, size_t subj_len, const char* delim, size_t delim_len, std::vector<std::string>& parts);
void str_split(const wchar_t* subj, size_t subj_len, const wchar_t* delim, size_t delim_len, std::vector<std::wstring>& parts);



}
