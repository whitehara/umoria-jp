// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "headers.h"
#include <cassert>
#include <climits>
#include <cwchar>
#include <vector>

// Returns position of first set bit and clears that bit -RAK-
int getAndClearFirstBit(uint32_t &flag) {
    uint32_t mask = 0x1;

    for (int i = 0; i < (int) sizeof(flag) * 8; i++) {
        if ((flag & mask) != 0u) {
            flag &= ~mask;
            return i;
        }
        mask <<= 1;
    }

    // no one bits found
    return -1;
}

// Insert a long number into a string (was `insert_lnum()` function)
void insertNumberIntoString(char *to_string, const char *from_string, int32_t number, bool show_sign) {
    size_t from_len = strlen(from_string);
    char *to_str_tmp = to_string;
    char *str = nullptr;

    // must be int for strncmp()
    int flag = 1;

    while (flag != 0) {
        str = strchr(to_str_tmp, from_string[0]);
        if (str == nullptr) {
            flag = 0;
        } else {
            flag = strncmp(str, from_string, from_len);
            if (flag != 0) {
                to_str_tmp = str + 1;
            }
        }
    }

    if (str != nullptr) {
        // `to_string` is always an `obj_desc_t` (160 bytes) in practice
        // (see store.cpp's haggle speech callers): a translated (e.g.
        // Japanese, or the pseudolocalization worst case) haggle phrase
        // can exceed 80 bytes, so `vtype_t` locals here -- and capping the
        // final snprintf() at `MORIA_MESSAGE_SIZE` (80) -- were a stack
        // buffer overflow / silent truncation risk, the same class of bug
        // fixed in insertStringIntoString() above.
        obj_desc_t str1 = {'\0'};
        obj_desc_t str2 = {'\0'};

        (void) strncpy(str1, to_string, str - to_string);
        str1[str - to_string] = '\0';
        (void) strcpy(str2, str + from_len);

        if (number >= 0 && show_sign) {
            (void) snprintf(to_string, MORIA_OBJ_DESC_SIZE, "%s+%d%s", str1, number, str2);
        } else {
            (void) snprintf(to_string, MORIA_OBJ_DESC_SIZE, "%s%d%s", str1, number, str2);
        }
    }
}

// Inserts a string into a string
void insertStringIntoString(char *to_string, const char *from_string, const char *str_to_insert) {
    auto from_len = (int) strlen(from_string);
    auto to_len = (int) strlen(to_string);

    char *bound = to_string + to_len - from_len;
    char *pc = nullptr;

    for (pc = to_string; pc <= bound; pc++) {
        char *temp_obj = pc;
        const char *temp_mtc = from_string;

        int i;
        for (i = 0; i < from_len; i++) {
            if (*temp_obj++ != *temp_mtc++) {
                break;
            }
        }
        if (i == from_len) {
            break;
        }
    }

    if (pc <= bound) {
        // `to_string` is always an `obj_desc_t` (160 bytes) in practice
        // (see identification.cpp's `itemDescription()`), not the smaller
        // `vtype_t` (80 bytes): a translated (e.g. Japanese, or the
        // pseudolocalization worst case) item description can exceed 80
        // bytes once the "&"/"~" grammar template is filled in, so a
        // `vtype_t` local here was a stack buffer overflow waiting for
        // wide-enough translated content -- confirmed by a live crash
        // (SIGSEGV in itemDescription()/insertStringIntoString()) when
        // opening the inventory as a Mage under MORIA_LANG=pseudo (the
        // starting spellbook's "& Book~ of Magic Spells %s" description
        // exceeds 80 bytes once pseudo-translated).
        obj_desc_t new_string;

        (void) strncpy(new_string, to_string, (pc - to_string));

        new_string[pc - to_string] = '\0';

        if (str_to_insert != nullptr) {
            (void) strcat(new_string, str_to_insert);
        }

        (void) strcat(new_string, (pc + from_len));
        (void) strcpy(to_string, new_string);
    }
}

bool isVowel(char ch) {
    switch (ch) {
        case 'a':
        case 'e':
        case 'i':
        case 'o':
        case 'u':
        case 'A':
        case 'E':
        case 'I':
        case 'O':
        case 'U':
            return true;
        default:
            return false;
    }
}

// http://rus.har.mn/blog/2014-05-19/strtol-error-checking/
bool stringToNumber(const char *str, int &number) {
    // we need to reset `errno`
    errno = 0;

    char *endptr = nullptr;
    long num = strtol(str, &endptr, 10);

    if (errno == ERANGE) {
        switch (num) {
            case (int) LONG_MIN: // underflow
            case (int) LONG_MAX: // overflow
                break;
            default:
                // impossible
                assert(false);
        }
        return false;
    }

    // something else happened. die die die
    if (errno != 0) {
        return false;
    }

    // garbage at end of string
    if (*endptr != '\0') {
        return false;
    }

    number = (int) num;
    return true;
}

uint32_t getCurrentUnixTime() {
    return static_cast<uint32_t>(time(nullptr));
}

void humanDateString(char *day) {
    time_t now = time(nullptr);
    struct tm *datetime = localtime(&now);

#ifdef _WIN32
    strftime(day, 11, "%a %b %d", datetime);
#else
    strftime(day, 11, "%a %b %e", datetime);
#endif
}

int displayWidth(const std::string &str) {
    if (str.empty()) {
        return 0;
    }

    std::vector<wchar_t> wide_buffer(str.size() + 1);

    size_t wide_char_count = mbstowcs(wide_buffer.data(), str.c_str(), wide_buffer.size());
    if (wide_char_count == (size_t) -1) {
        // Not valid multibyte data for the current locale, fall back to byte length.
        return (int) str.size();
    }

    int width = wcswidth(wide_buffer.data(), wide_char_count);
    if (width < 0) {
        // Contains a non-printable wide character, fall back to byte length.
        return (int) str.size();
    }

    return width;
}

std::string truncateToWidth(const std::string &str, int columns) {
    if (columns <= 0) {
        return std::string();
    }

    std::vector<wchar_t> wide_buffer(str.size() + 1);

    size_t wide_char_count = mbstowcs(wide_buffer.data(), str.c_str(), wide_buffer.size());
    if (wide_char_count == (size_t) -1) {
        // Not valid multibyte data for the current locale, fall back to a plain byte cut.
        return str.substr(0, (size_t) columns);
    }

    int used_width = 0;
    size_t accepted_wide_chars = 0;

    for (size_t i = 0; i < wide_char_count; i++) {
        int char_width = wcwidth(wide_buffer[i]);
        if (char_width < 0) {
            char_width = 0; // non-printable, doesn't consume a column.
        }

        if (used_width + char_width > columns) {
            break;
        }

        used_width += char_width;
        accepted_wide_chars++;
    }

    if (accepted_wide_chars == wide_char_count) {
        return str;
    }

    // Re-encode the accepted prefix back into a locale-encoded (e.g. UTF-8)
    // string, so the cut always lands on a character boundary.
    wide_buffer[accepted_wide_chars] = L'\0';

    std::vector<char> multi_byte_buffer((accepted_wide_chars * (size_t) MB_CUR_MAX) + 1);
    size_t byte_count = wcstombs(multi_byte_buffer.data(), wide_buffer.data(), multi_byte_buffer.size());
    if (byte_count == (size_t) -1) {
        return str.substr(0, (size_t) columns);
    }

    return std::string(multi_byte_buffer.data(), byte_count);
}

std::string truncateToByteCapacity(const std::string &str, size_t max_bytes) {
    if (max_bytes == 0) {
        return std::string();
    }

    if (str.size() <= max_bytes) {
        return str;
    }

    std::vector<wchar_t> wide_buffer(str.size() + 1);

    size_t wide_char_count = mbstowcs(wide_buffer.data(), str.c_str(), wide_buffer.size());
    if (wide_char_count == (size_t) -1) {
        // Not valid multibyte data for the current locale, fall back to a plain byte cut.
        return str.substr(0, max_bytes);
    }

    size_t used_bytes = 0;
    size_t accepted_wide_chars = 0;
    char mb_buffer[MB_LEN_MAX];
    std::mbstate_t state{};

    for (size_t i = 0; i < wide_char_count; i++) {
        int char_bytes = (int) wcrtomb(mb_buffer, wide_buffer[i], &state);
        if (char_bytes < 0) {
            char_bytes = 1; // encoding error; conservatively count it as one byte.
            state = std::mbstate_t{};
        }

        if (used_bytes + (size_t) char_bytes > max_bytes) {
            break;
        }

        used_bytes += (size_t) char_bytes;
        accepted_wide_chars++;
    }

    if (accepted_wide_chars == wide_char_count) {
        return str;
    }

    // Re-encode the accepted prefix back into a locale-encoded (e.g. UTF-8)
    // string, so the cut always lands on a character boundary.
    wide_buffer[accepted_wide_chars] = L'\0';

    std::vector<char> multi_byte_buffer((accepted_wide_chars * (size_t) MB_CUR_MAX) + 1);
    size_t byte_count = wcstombs(multi_byte_buffer.data(), wide_buffer.data(), multi_byte_buffer.size());
    if (byte_count == (size_t) -1) {
        return str.substr(0, max_bytes);
    }

    return std::string(multi_byte_buffer.data(), byte_count);
}

std::string padToDisplayWidth(const std::string &str, int width) {
    std::string truncated = truncateToWidth(str, width);
    int pad = width - displayWidth(truncated);
    if (pad > 0) {
        truncated.append((size_t) pad, ' ');
    }
    return truncated;
}

void replaceLastCharSafely(char *str, char replacement) {
    size_t len = strlen(str);
    if (len == 0) {
        return;
    }

    // Walk back from the end while on a UTF-8 continuation byte
    // (10xxxxxx), so we land on the start of the last full character
    // regardless of whether it's 1 byte (ASCII) or multi-byte.
    size_t last_char_start = len - 1;
    while (last_char_start > 0 && ((unsigned char) str[last_char_start] & 0xC0) == 0x80) {
        last_char_start--;
    }

    str[last_char_start] = replacement;
    str[last_char_start + 1] = '\0';
}
