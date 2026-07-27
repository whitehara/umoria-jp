// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Minimal assert-based unit test for the phase 2 i18n catalog
// (src/lang.cpp): language resolution priority and the .po subset parser.
// No external test framework.

// NOTE: <fstream> must be included before "headers.h": ui.h #defines
// `open` to `topen` (for the setuid tilde-expansion wrapper), and that
// macro corrupts std::basic_filebuf::open() if <fstream> is parsed
// after the #define is active.
#include <fstream>

#include "headers.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

int main() {
    // --- resolveLanguage() priority: cli arg > MORIA_LANG > LANG > LC_ALL > "en" ---

    setenv("MORIA_LANG", "fr", 1);
    setenv("LANG", "de_DE.UTF-8", 1);
    assert(lang::resolveLanguage("ja") == "ja"); // explicit cli arg wins over everything

    assert(lang::resolveLanguage("") == "fr"); // no cli arg: MORIA_LANG wins over LANG

    unsetenv("MORIA_LANG");
    assert(lang::resolveLanguage("") == "de"); // falls back to first 2 chars of LANG

    unsetenv("LANG");
    setenv("LC_ALL", "es_ES.UTF-8", 1);
    assert(lang::resolveLanguage("") == "es"); // falls back to LC_ALL when LANG is unset

    unsetenv("LC_ALL");
    assert(lang::resolveLanguage("") == "en"); // nothing set at all: default

    // --- loadCatalog() / _() : missing catalog file is a graceful pass-through ---

    bool loaded_missing = lang::loadCatalog("xx-does-not-exist");
    assert(!loaded_missing);
    assert(lang::currentLanguage() == "xx-does-not-exist");
    assert(std::string(_("hello")) == "hello");

    // --- loadCatalog() / _() : a real, minimal catalog file ---

    (void) system("mkdir -p data/lang/zz");
    {
        std::ofstream out("data/lang/zz/umoria.po");
        out << "# test catalog\n";
        out << "msgid \"hello\"\n";
        out << "msgstr \"bonjour\"\n";
        out << "\n";
        out << "msgid \"\"\n";
        out << "\"multi\"\n";
        out << "\"line\"\n";
        out << "msgstr \"\"\n";
        out << "\"trans-\"\n";
        out << "\"lated\"\n";
        out << "\n";
        out << "msgid \"quoted \\\"word\\\" and\\nbreak\"\n";
        out << "msgstr \"translated \\\"word\\\" and\\nbreak\"\n";
        out << "\n";
        out << "msgid \"untranslated\"\n";
        out << "msgstr \"\"\n";
    }

    bool loaded = lang::loadCatalog("zz");
    assert(loaded);
    assert(lang::currentLanguage() == "zz");

    assert(std::string(_("hello")) == "bonjour");
    assert(_(std::string("multiline")) == "trans-lated");
    assert(_(std::string("quoted \"word\" and\nbreak")) == "translated \"word\" and\nbreak");

    // Empty msgstr => pass-through of msgid, not an empty string.
    assert(_(std::string("untranslated")) == "untranslated");

    // Unknown msgid => pass-through.
    assert(std::string(_("never seen")) == "never seen");

    (void) std::remove("data/lang/zz/umoria.po");
    (void) std::remove("data/lang/zz");

    // --- loadCatalog() : a malformed .po with an orphaned msgstr (no
    // preceding msgid) must not leak its value into the next entry ---

    (void) system("mkdir -p data/lang/qq");
    {
        std::ofstream out("data/lang/qq/umoria.po");
        out << "msgstr \"orphan-value\"\n";
        out << "\n";
        out << "msgid \"real\"\n";
        out << "\n";
    }

    bool loaded_malformed = lang::loadCatalog("qq");
    assert(loaded_malformed);
    // "real" has no msgstr of its own; it must pass through unchanged,
    // not pick up the orphaned msgstr's value.
    assert(std::string(_("real")) == "real");

    (void) std::remove("data/lang/qq/umoria.po");
    (void) std::remove("data/lang/qq");

    // --- loadCatalog() / C_() : msgctxt disambiguation (phase 10) ---
    //
    // "Gold" appears both as a flavor word (context "flavor", e.g. an
    // unidentified wand's metal) and as a context-free label elsewhere
    // (e.g. the character sheet's gold amount) with a different
    // translation. msgctxt keeps these two catalog entries independent
    // instead of one clobbering the other.

    (void) system("mkdir -p data/lang/ctx");
    {
        std::ofstream out("data/lang/ctx/umoria.po");
        out << "msgctxt \"flavor\"\n";
        out << "msgid \"Gold\"\n";
        out << "msgstr \"kin-flavor\"\n";
        out << "\n";
        out << "msgid \"Gold\"\n";
        out << "msgstr \"kin-label\"\n";
        out << "\n";
        out << "msgctxt \"flavor\"\n";
        out << "msgid \"untranslated-in-context\"\n";
        out << "msgstr \"\"\n";
        out << "\n";
        out << "msgid \"plain\"\n";
        out << "msgstr \"plain-translated\"\n";
    }

    bool loaded_ctx = lang::loadCatalog("ctx");
    assert(loaded_ctx);

    // (a) A context-free entry loads and resolves via _() exactly as
    // before msgctxt support existed -- back-compat with all pre-phase-10
    // catalog entries (of which there are 2000+).
    assert(std::string(_("plain")) == "plain-translated");

    // (b) C_() with a context that has no matching msgctxt entry (or an
    // empty translation for one that does) falls back to the bare
    // English msgid, exactly like _() does for an untranslated string.
    assert(std::string(C_("flavor", "untranslated-in-context")) == "untranslated-in-context");
    assert(std::string(C_("no-such-context", "plain")) == "plain"); // no msgctxt "no-such-context" entry at all

    // (c) msgctxt "flavor" + msgid "Gold" and the context-free msgid
    // "Gold" are separate catalog keys: C_("flavor", "Gold") must not
    // see the context-free entry's translation, and _("Gold") must not
    // see the "flavor" one -- this is the actual collision phase 10 set
    // out to resolve (metals[] flavor word "Gold" vs. the character
    // sheet's "Gold" label).
    assert(std::string(C_("flavor", "Gold")) == "kin-flavor");
    assert(std::string(_("Gold")) == "kin-label");

    (void) std::remove("data/lang/ctx/umoria.po");
    (void) std::remove("data/lang/ctx");

    // --- lang::format() : positional "%s" substitution ---

    assert(lang::format("The %s", {"Balrog"}) == "The Balrog");
    assert(lang::format("%s %s", {"The Balrog", "attacks."}) == "The Balrog attacks.");
    // ja-style templates: no space, or reordered/rejoined text.
    assert(lang::format("%s", {"Balrog"}) == "Balrog");
    assert(lang::format("%s%s", {"Balrog", "が現れた。"}) == "Balrogが現れた。");

    // No placeholders at all: template returned verbatim, args ignored.
    assert(lang::format("Hello.", {"unused"}) == "Hello.");
    assert(lang::format("Hello.", {}) == "Hello.");

    // Fewer args than placeholders: extra "%s" left as literal text.
    assert(lang::format("%s %s", {"only-one"}) == "only-one %s");
    assert(lang::format("%s", {}) == "%s");

    // More args than placeholders: extra args are simply ignored.
    assert(lang::format("%s", {"first", "second", "third"}) == "first");

    // A trailing lone '%' (not followed by 's') is copied through as-is,
    // not treated as a placeholder.
    assert(lang::format("100%", {"unused"}) == "100%");
    assert(lang::format("%d not a placeholder", {"unused"}) == "%d not a placeholder");

    std::printf("test_lang: all assertions passed\n");

    return 0;
}
