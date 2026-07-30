// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Minimal assert-based unit test for the phase 1 UTF-8 display width
// helpers (src/helpers.cpp). No external test framework.

#include "headers.h"
#include "test_env.h"

#include <cassert>
#include <clocale>
#include <cstdio>

int main() {
    // consoleMayGarbleWideCharacters() only looks at a fixed whitelist of
    // environment variables, but some CI/dev environments (e.g. VS Code's
    // integrated terminal) already set one of them -- clear them all first
    // for a deterministic baseline.
    unsetTestEnv("WT_SESSION");
    unsetTestEnv("WT_PROFILE_ID");
    unsetTestEnv("ConEmuPID");
    unsetTestEnv("ConEmuANSI");
    unsetTestEnv("WEZTERM_PANE");
    unsetTestEnv("TERM_PROGRAM");

    assert(consoleMayGarbleWideCharacters()); // nothing set: assume unsafe by default

    setTestEnv("WT_SESSION", "1");
    assert(!consoleMayGarbleWideCharacters()); // Windows Terminal
    unsetTestEnv("WT_SESSION");

    setTestEnv("TERM_PROGRAM", "vscode");
    assert(!consoleMayGarbleWideCharacters()); // VS Code's integrated terminal
    unsetTestEnv("TERM_PROGRAM");

    setTestEnv("TERM_PROGRAM", "Apple_Terminal");
    assert(consoleMayGarbleWideCharacters()); // checks the value, not just presence
    unsetTestEnv("TERM_PROGRAM");

    // fallbackWcwidth() is always compiled (used at runtime on Windows,
    // where MinGW-w64's UCRT doesn't declare wcwidth()/wcswidth()), so it
    // is testable directly here regardless of locale/platform.
    assert(fallbackWcwidth(L'\0') == 0);
    assert(fallbackWcwidth(L'A') == 1);
    assert(fallbackWcwidth(L'0') == 1);
    assert(fallbackWcwidth(0x3042) == 2);  // U+3042 HIRAGANA LETTER A
    assert(fallbackWcwidth(0x30A2) == 2);  // U+30A2 KATAKANA LETTER A
    assert(fallbackWcwidth(0x4E00) == 2);  // U+4E00 CJK UNIFIED IDEOGRAPH (一)
    assert(fallbackWcwidth(0xFF21) == 2);  // U+FF21 FULLWIDTH LATIN CAPITAL LETTER A
    assert(fallbackWcwidth(0x0301) == 0);  // U+0301 COMBINING ACUTE ACCENT
    assert(fallbackWcwidth(0x0007) == -1); // BEL, a non-printable control character

    // ASCII behavior must hold regardless of locale.
    assert(displayWidth("") == 0);
    assert(displayWidth("hello") == 5);
    assert(truncateToWidth("hello world", 5) == "hello");
    assert(truncateToWidth("hello", 100) == "hello");
    assert(truncateToWidth("hello", 0).empty());

    assert(truncateToByteCapacity("hello world", 5) == "hello");
    assert(truncateToByteCapacity("hello", 100) == "hello");
    assert(truncateToByteCapacity("hello", 0).empty());

    bool has_utf8_locale = (std::setlocale(LC_ALL, "C.UTF-8") != nullptr) || (std::setlocale(LC_ALL, "C.utf8") != nullptr);

    if (has_utf8_locale) {
        // U+3042 HIRAGANA LETTER A ("あ"): 3 bytes in UTF-8, display width 2.
        const std::string hiragana_a = "\xE3\x81\x82";
        assert(displayWidth(hiragana_a) == 2);

        const std::string two_wide = hiragana_a + hiragana_a; // "ああ": width 4, 6 bytes
        assert(displayWidth(two_wide) == 4);

        // Truncating must always land on a character boundary, never split
        // a multi-byte character in the middle.
        assert(truncateToWidth(two_wide, 2) == hiragana_a);
        assert(truncateToWidth(two_wide, 3) == hiragana_a); // odd budget: only 1 whole wide char fits
        assert(truncateToWidth(two_wide, 4) == two_wide);

        // Mixed ASCII + wide text: "A" (width 1) + "あ" (width 2) + "B" (width 1) = width 4.
        const std::string mixed = "A" + hiragana_a + "B";
        assert(displayWidth(mixed) == 4);
        // Budget 2: "A" (width 1) fits, but adding "あ" (width 2) would make
        // width 3 > 2, so only "A" is accepted.
        assert(truncateToWidth(mixed, 2) == "A");

        // truncateToByteCapacity budgets raw bytes (not display columns),
        // for copying into fixed-size byte buffers like vtype_t. "ああ" is
        // 6 bytes (3 bytes/char); a 4-byte budget must fit only 1 whole
        // character, never split one down the middle.
        assert(truncateToByteCapacity(two_wide, 6) == two_wide);
        assert(truncateToByteCapacity(two_wide, 5) == hiragana_a);
        assert(truncateToByteCapacity(two_wide, 4) == hiragana_a);
        assert(truncateToByteCapacity(two_wide, 3) == hiragana_a);
        assert(truncateToByteCapacity(two_wide, 2).empty());
        // padToDisplayWidth() pads/truncates to an exact display-column
        // width, safe for translated (multi-byte) text placed into a
        // fixed-width screen field (e.g. the status line in ui.cpp).
        assert(padToDisplayWidth("", 7) == "       ");
        assert(padToDisplayWidth("hi", 7) == "hi     ");
        assert(padToDisplayWidth("hello world", 5) == "hello");

        // "ああ" is width 4; padding to 7 columns adds 3 trailing spaces,
        // and the multi-byte characters must remain intact (not corrupted
        // by the padding math).
        assert(padToDisplayWidth(two_wide, 7) == two_wide + "   ");
        // Truncating to a narrower budget must still land on a character
        // boundary, matching truncateToWidth()'s behavior.
        assert(padToDisplayWidth(two_wide, 3) == hiragana_a + " ");
    } else {
        std::printf("test_helpers: warning: no UTF-8 locale available, skipping multi-byte width assertions\n");
    }

    std::printf("test_helpers: all assertions passed\n");

    return 0;
}
