// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Generic helper functions used throughout the code

// Eventually we want these helpers to have no external dependencies
// other than the standard library functions.

#pragma once

int getAndClearFirstBit(uint32_t &flag);
void insertNumberIntoString(char *to_string, const char *from_string, int32_t number, bool show_sign);
void insertStringIntoString(char *to_string, const char *from_string, const char *str_to_insert);
bool isVowel(char ch);
bool stringToNumber(const char *str, int &number);
uint32_t getCurrentUnixTime();
void humanDateString(char *day);

// Returns the terminal display width (in columns) of a locale-encoded
// (e.g. UTF-8) string, accounting for multi-byte and wide characters.
// Falls back to the byte length if the string can't be decoded under
// the current locale.
int displayWidth(const std::string &str);

// Returns the longest prefix of a locale-encoded string whose display
// width does not exceed `columns`, always cut on a character boundary.
// Falls back to a plain byte-substring if the string can't be decoded
// under the current locale.
std::string truncateToWidth(const std::string &str, int columns);

// Returns the longest prefix of a locale-encoded string whose byte
// length does not exceed `max_bytes`, always cut on a character
// boundary (unlike truncateToWidth, this budgets raw bytes, not
// display columns — for copying into fixed-size byte buffers such as
// vtype_t). Falls back to a plain byte-substring if the string can't
// be decoded under the current locale.
std::string truncateToByteCapacity(const std::string &str, size_t max_bytes);

// Truncates `str` to at most `width` display columns and pads it with
// trailing spaces up to exactly `width` columns. Unlike a raw `%-N.Ns`
// printf specifier (which counts bytes, not display columns, and can
// cut a multi-byte UTF-8 character in half), this is safe for translated
// (e.g. Japanese) text placed into a fixed-width screen field, including
// fields that must be blanked out again once a status is no longer active.
std::string padToDisplayWidth(const std::string &str, int width);

// Replaces the last display character of a null-terminated, locale-encoded
// (e.g. UTF-8) C string with `replacement` (a single ASCII byte), in
// place. Unlike `str[strlen(str) - 1] = replacement`, this is safe when
// the trailing character is multi-byte (e.g. a translated "." that
// became the full-width "。"): it walks back over UTF-8 continuation
// bytes to find the start of the last character, then overwrites the
// whole character (not just its last byte) and re-terminates the
// string, so no dangling continuation bytes are left behind. No-op on
// an empty string.
void replaceLastCharSafely(char *str, char replacement);
