#ifndef OSMIUM_IO_DETAIL_STRING_UTIL_HPP
#define OSMIUM_IO_DETAIL_STRING_UTIL_HPP

/*

This file is part of Osmium (https://osmcode.org/libosmium).

Copyright 2013-2022 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

namespace osmium {

    namespace io {

        namespace detail {

#ifndef _MSC_VER
# define SNPRINTF std::snprintf
#else
# define SNPRINTF _snprintf
#endif

            template <typename... TArgs>
            inline int string_snprintf(std::string& out,
                                       const std::size_t old_size,
                                       const std::size_t max_size,
                                       const char* format,
                                       TArgs... args) {
                out.resize(old_size + max_size);

                return SNPRINTF(max_size ? &out[old_size] : nullptr,
                                max_size, format, args...);
            }

#undef SNPRINTF

            /**
             * This is a helper function for writing printf-like formatted
             * data into a std::string.
             *
             * @param out The data will be appended to this string.
             * @param format A string with formatting instructions a la printf.
             * @param args Any further arguments like in printf.
             * @throws std::bad_alloc If the string needed to grow and there
             *         wasn't enough memory.
             */
            template <typename... TArgs>
            inline void append_printf_formatted_string(std::string& out,
                                                       const char* format,
                                                       TArgs... args) {

                // First try to write string with the max_size, if that doesn't
                // work snprintf will tell us how much space it needs. We
                // reserve that much space and try again. So this will always
                // work, even if the output is larger than the given max_size.
                //
                // Unfortunately this trick doesn't work on Windows, because
                // the _snprintf() function there only returns the length it
                // needs if max_size==0 and the buffer pointer is the null
                // pointer. So we have to take this into account.

#ifndef _MSC_VER
                static const std::size_t max_size = 100;
#else
                static const std::size_t max_size = 0;
#endif

                const std::size_t old_size = out.size();

                const int len = string_snprintf(out,
                                                old_size,
                                                max_size,
                                                format,
                                                args...);
                assert(len > 0);

                if (static_cast<std::size_t>(len) >= max_size) {
#ifndef NDEBUG
                    const int len2 =
#endif
                                     string_snprintf(out,
                                                     old_size,
                                                     std::size_t(len) + 1,
                                                     format,
                                                     args...);
                    assert(len2 == len);
                }

                out.resize(old_size + static_cast<std::size_t>(len));
            }

            inline uint8_t utf8_sequence_length(uint32_t first) noexcept {
                if (first < 0x80U) {
                    return 1U;
                }

                if ((first >> 5U) == 0x6U) {
                    return 2U;
                }

                if ((first >> 4U) == 0xeU) {
                    return 3U;
                }

                if ((first >> 3U) == 0x1eU) {
                    return 4U;
                }

                return 0;
            }

            inline uint32_t next_utf8_codepoint(char const** begin, const char* end) {
                const auto* it = reinterpret_cast<const uint8_t*>(*begin);
                uint32_t cp = 0xffU & *it;
                const auto length = utf8_sequence_length(cp);
                if (length == 0) {
                    throw std::runtime_error{"invalid Unicode codepoint"};
                }
                if (std::distance(it, reinterpret_cast<const uint8_t*>(end)) < length) {
                    throw std::out_of_range{"incomplete Unicode codepoint"};
                }
                switch (length) {
                    case 1:
                        break;
                    case 2:
                        ++it;
                        cp = ((cp << 6U) & 0x7ffU) + ((*it) & 0x3fU);
                        break;
                    case 3:
                        ++it;
                        cp = ((cp << 12U) & 0xffffU) + (((0xffU & *it) << 6U) & 0xfffU);
                        ++it;
                        cp += (*it) & 0x3fU;
                        break;
                    case 4:
                        ++it;
                        cp = ((cp << 18U) & 0x1fffffU) + (((0xffU & *it) << 12U) & 0x3ffffU);
                        ++it;
                        cp += ((0xffU & *it) << 6U) & 0xfffU;
                        ++it;
                        cp += (*it) & 0x3fU;
                        break;
                    default:
                        break;
                }
                ++it;
                *begin = reinterpret_cast<const char*>(it);
                return cp;
            }

            // Write out the value with exactly two hex digits.
            inline void append_2_hex_digits(std::string& out, uint32_t value, const char* const hex_digits) {
                out += hex_digits[(value >> 4U) & 0xfU];
                out += hex_digits[ value        & 0xfU];
            }

            // Write out the value with four or more hex digits.
            inline void append_min_4_hex_digits(std::string& out, uint32_t value, const char* const hex_digits) {
                auto
                v = value & 0xf0000000U; if (v) { out += hex_digits[v >> 28U]; }
                v = value & 0x0f000000U; if (v) { out += hex_digits[v >> 24U]; }
                v = value & 0x00f00000U; if (v) { out += hex_digits[v >> 20U]; }
                v = value & 0x000f0000U; if (v) { out += hex_digits[v >> 16U]; }

                out += hex_digits[(value >> 12U) & 0xfU];
                out += hex_digits[(value >>  8U) & 0xfU];
                out += hex_digits[(value >>  4U) & 0xfU];
                out += hex_digits[ value         & 0xfU];
            }

            inline void append_utf8_encoded_string(std::string& out, const char* data) {
                static const char* lookup_hex = "0123456789abcdef";
                assert(data);
                const char* end_ptr = data + std::strlen(data);

                while (data != end_ptr) {
                    const char* prev = data;
                    const uint32_t c = next_utf8_codepoint(&data, end_ptr);

                    // This is a list of Unicode code points that we let
                    // through instead of escaping them. It is incomplete
                    // and can be extended later.
                    // Generally we don't want to let through any character
                    // that has special meaning in the OPL format such as
                    // space, comma, @, etc. and any non-printing characters.
                    if ((0x0021 <= c && c <= 0x0024) ||
                        (0x0026 <= c && c <= 0x002b) ||
                        (0x002d <= c && c <= 0x003c) ||
                        (0x003e <= c && c <= 0x003f) ||
                        (0x0041 <= c && c <= 0x007e) ||
                        (0x00a1 <= c && c <= 0x00ac) ||
                        (0x00ae <= c && c <= 0x05ff)) {
                        out.append(prev, data);
                    } else {
                        out += '%';
                        if (c <= 0xff) {
                            append_2_hex_digits(out, c, lookup_hex);
                        } else {
                            append_min_4_hex_digits(out, c, lookup_hex);
                        }
                        out += '%';
                    }
                }
            }

            inline void append_xml_encoded_string(std::string& out, const char* data) {
                assert(data);
                for (; *data != '\0'; ++data) {
                    switch (*data) {
                        case '&':  out += "&amp;";  break;
                        case '\"': out += "&quot;"; break;
                        case '\'': out += "&apos;"; break;
                        case '<':  out += "&lt;";   break;
                        case '>':  out += "&gt;";   break;
                        case '\n': out += "&#xA;";  break;
                        case '\r': out += "&#xD;";  break;
                        case '\t': out += "&#x9;";  break;
                        default:   out += *data;    break;
                    }
                }
            }

            inline void append_debug_encoded_string(std::string& out, const char* data, const char* prefix, const char* suffix) {
                static const char* lookup_hex = "0123456789ABCDEF";
                const char* end_ptr = data + std::strlen(data);

                while (data != end_ptr) {
                    const char* prev = data;
                    const uint32_t c = next_utf8_codepoint(&data, end_ptr);

                    // This is a list of Unicode code points that we let
                    // through instead of escaping them. It is incomplete
                    // and can be extended later.
                    // Generally we don't want to let through any
                    // non-printing characters.
                    if ((0x0020 <= c && c <= 0x0021) ||
                        (0x0023 <= c && c <= 0x003b) ||
                        (0x003d == c) ||
                        (0x003f <= c && c <= 0x007e) ||
                        (0x00a1 <= c && c <= 0x00ac) ||
                        (0x00ae <= c && c <= 0x05ff)) {
                        out.append(prev, data);
                    } else {
                        out.append(prefix);
                        out.append("<U+");
                        append_min_4_hex_digits(out, c, lookup_hex);
                        out.append(">");
                        out.append(suffix);
                    }
                }
            }

            template <typename TOutputIterator>
            TOutputIterator append_codepoint_as_utf8(uint32_t cp, TOutputIterator out) {
                if (cp < 0x80UL) {
                    *(out++) = static_cast<char>(cp);
                } else if (cp < 0x800UL) {
                    *(out++) = static_cast<char>( (cp >>  6U)          | 0xc0U);
                    *(out++) = static_cast<char>(( cp         & 0x3fU) | 0x80U);
                } else if (cp < 0x10000UL) {
                    *(out++) = static_cast<char>( (cp >> 12U)          | 0xe0U);
                    *(out++) = static_cast<char>(((cp >>  6U) & 0x3fU) | 0x80U);
                    *(out++) = static_cast<char>(( cp         & 0x3fU) | 0x80U);
                } else {
                    *(out++) = static_cast<char>( (cp >> 18U)          | 0xf0U);
                    *(out++) = static_cast<char>(((cp >> 12U) & 0x3fU) | 0x80U);
                    *(out++) = static_cast<char>(((cp >>  6U) & 0x3fU) | 0x80U);
                    *(out++) = static_cast<char>(( cp         & 0x3fU) | 0x80U);
                }
                return out;
            }

        } // namespace detail

    } // namespace io

} // namespace osmium

#endif // OSMIUM_IO_DETAIL_STRING_UTIL_HPP
