// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Test-only environment variable helpers, shared by test_helpers.cpp and
// test_lang.cpp. Not used by any production code.

#pragma once

#include <cstdlib>

namespace {

// setenv()/unsetenv() are POSIX and not declared by MinGW-w64's UCRT
// <stdlib.h>, which instead provides _putenv_s() (an empty value string
// removes the variable).
#ifdef _WIN32
void setTestEnv(const char *name, const char *value) {
    (void) _putenv_s(name, value);
}

void unsetTestEnv(const char *name) {
    (void) _putenv_s(name, "");
}
#else
void setTestEnv(const char *name, const char *value) {
    (void) setenv(name, value, 1);
}

void unsetTestEnv(const char *name) {
    (void) unsetenv(name);
}
#endif

} // namespace
