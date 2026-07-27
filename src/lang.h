// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Minimal i18n message catalog: a small subset of the gettext .po file
// format (msgid/msgstr pairs, multi-line string concatenation, and
// \\ \" \n \t escapes), with no libintl dependency.

#pragma once

namespace lang {
// Resolves which language code to use, in priority order:
//   1. `cli_lang`, if non-empty (e.g. from the `-l` command line flag)
//   2. the `MORIA_LANG` environment variable, if set and non-empty
//   3. the first two characters of `LANG`, then `LC_ALL`, if set
//   4. "en" (the default; no catalog is loaded for it)
std::string resolveLanguage(const std::string &cli_lang);

// Loads "data/lang/<language>/umoria.po" into the in-memory catalog
// used by _(), replacing whatever catalog was loaded before (if any).
// Sets config::language::current to `language` regardless of whether
// a catalog file was found. Returns true if the file was found and
// parsed; false if it doesn't exist (in which case the catalog is
// left empty, and _() is a pass-through for every msgid).
bool loadCatalog(const std::string &language);

// Returns the currently active language code (config::language::current).
const std::string &currentLanguage();

// Substitutes each "%s" placeholder in `tmpl`, in order, with the
// corresponding string from `args`. Extra args are ignored; if `tmpl`
// has more "%s" placeholders than args, the extra placeholders are left
// as literal "%s". Intended for translated combination templates, e.g.
// lang::format(_("The %s"), {creature_name}) or
// lang::format(_("%s %s"), {name, action}) — a language's msgstr can
// reorder/rejoin without spaces (e.g. ja: "%s"/"%s%s") since the
// substitution is purely positional, not printf-style typed formatting.
std::string format(const std::string &tmpl, std::initializer_list<std::string> args);
} // namespace lang

// Translates `msgid` via the active catalog. Returns `msgid` unchanged
// if there's no catalog entry for it, or if the entry's translation is
// empty (an explicit "not yet translated" marker in a .po file).
const char *_(const char *msgid);
std::string _(const std::string &msgid);

// Translates `msgid` within disambiguation context `ctx` (a gettext-style
// msgctxt, e.g. "flavor" for unidentified-item flavor words) via the
// active catalog. The catalog key for a msgctxt entry is `ctx + '\x04'
// + msgid` (the gettext convention), keeping it distinct from a
// context-free `_(msgid)` lookup of the same msgid text -- this is what
// lets an unrelated msgid collision (e.g. the flavor word "Gold" vs. the
// character sheet's "Gold" label) resolve independently instead of
// overwriting one another's translation. Returns `msgid` unchanged if
// there's no catalog entry for `ctx + '\x04' + msgid`, or if that
// entry's translation is empty.
const char *C_(const char *ctx, const char *msgid);
