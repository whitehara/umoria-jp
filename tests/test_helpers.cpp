// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Minimal assert-based unit test for the phase 1 UTF-8 display width
// helpers (src/helpers.cpp). No external test framework.

#include "headers.h"

#include <cassert>
#include <clocale>
#include <cstdio>

int main() {
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
        assert(truncateToWidth(mixed, 2) == "A" + hiragana_a);

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
