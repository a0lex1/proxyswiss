#include "common/base/str.h"
#include "common/base/bin_scan.h"
#include "common/common.h"

#include <sstream>
#include <iomanip>
#include <limits>

#include <assert.h>
#include <windows.h>
#include <strsafe.h>

using namespace std;

namespace common {

namespace detail {
static const size_t kPrintfBufSize = 4096 * 4;
}

// Instantiate.
//
template wstring tstr_to_tstr<wchar_t, wchar_t>(const wstring& src);
template string tstr_to_tstr<char, wchar_t>(const wstring& src);
template wstring tstr_to_tstr<wchar_t, char>(const string& src);
template string tstr_to_tstr<char, char>(const string& src);

template <typename Char>
static size_t strlen_T(const Char* str) {
  switch (sizeof(Char)) {
    case sizeof(char):
      return strlen(reinterpret_cast<const char*>(str));
    case sizeof(wchar_t):
      return wcslen(reinterpret_cast<const wchar_t*>(str));
    default:
      NOT_REACHED();
  }
}



string str_printf(const char* fmt, ...) {
  vector<char> buf(detail::kPrintfBufSize);
  va_list vl;
  va_start(vl, fmt);
  HRESULT hr;
  hr = ::StringCchVPrintfA(&buf[0], detail::kPrintfBufSize, fmt, vl);
  va_end(vl);
  return &buf[0];
}

string str_printf(const char* fmt, va_list vl) {
  vector<char> buf(detail::kPrintfBufSize);
  HRESULT hr;
  hr = ::StringCchVPrintfA(&buf[0], detail::kPrintfBufSize, fmt, vl);
  return &buf[0];
}

wstring str_printf(const wchar_t* fmt, ...) {
  vector<wchar_t> buf(detail::kPrintfBufSize);
  va_list vl;
  va_start(vl, fmt);
  HRESULT hr;
  hr = ::StringCchVPrintfW(&buf[0], detail::kPrintfBufSize, fmt, vl);
  va_end(vl);
  return &buf[0];
}

wstring str_printf(const wchar_t* fmt, va_list vl) {
  vector<wchar_t> buf(detail::kPrintfBufSize);
  HRESULT hr;
  hr = ::StringCchVPrintfW(&buf[0], detail::kPrintfBufSize, fmt, vl);
  return &buf[0];
}

string str_printf(const string fmt, ...) {
  va_list vl;
  va_start(vl, fmt);
  return str_printf(fmt.c_str(), vl);
}

wstring str_printf(const wstring fmt, ...) {
  va_list vl;
  va_start(vl, fmt);
  return str_printf(fmt.c_str(), vl);
}

template <typename CharType>
static void _str_split_T(const CharType* subj,
                        size_t subj_len,
                        const CharType* delim,
                        size_t delim_len,
                        vector<basic_string<CharType>>& parts)
{
  assert(delim_len);
  parts.clear();
  if (!delim_len) {
    return;
  }
  size_t start = 0, end = 0;
  for (;;) {
    const CharType* pend =
      static_cast<const CharType*>(
        common::bin_scan(&subj[start], subj_len-start, delim, delim_len));
    if (!pend) {
      break;
    }
    end = pend - &subj[0];
    parts.push_back(basic_string<CharType>(&subj[start], end - start));
    start = end + delim_len;
  }
  parts.push_back(basic_string<CharType>(&subj[start], subj_len-start));
}

void str_split(const char* subj, size_t subj_len, const char* delim,
               size_t delim_len,
               std::vector<std::string>& parts)
{
  return _str_split_T(subj, subj_len, delim, delim_len, parts);
}

void str_split(const wchar_t* subj, size_t subj_len, const wchar_t* delim,
               size_t delim_len,
               std::vector<std::wstring>& parts)
{
  return _str_split_T(subj, subj_len, delim, delim_len, parts);
}

template <typename CharType>
static void str_split_T(const basic_string<CharType>& subject,
                 const basic_string<CharType>& delimiter,
                 vector<basic_string<CharType>>& parts)
{
  return _str_split_T(subject.c_str(), subject.length(), delimiter.c_str(),
                      delimiter.length(), parts);
}

void str_split(
  const string& subject,
  const string& delimiter,
  vector<string>& parts)
{
  return
    str_split_T(subject, delimiter, parts);
}

void str_split(
  const wstring& subject,
  const wstring& delimiter,
  vector<wstring>& parts)
{
  return
    str_split_T(subject, delimiter, parts);
}

}
