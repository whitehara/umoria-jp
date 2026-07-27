# Translating Umoria

This document describes the technical mechanics behind Umoria's Japanese
localization, for anyone who wants to understand how it works, fix a
translation-related bug, or add another language.

This fork ships one language catalog (`ja`) and is not a general-purpose
i18n framework: a handful of grammar decisions are hardcoded for Japanese
rather than driven by a generic per-language rule engine (see "Hardcoded
language branches" below). Adding a second non-English language is
possible, but expect to touch those call sites too, not just add a new
`.po` file.

## Catalog format

Translations live in `data/lang/<lang>/umoria.po`, a small hand-rolled
subset of the gettext `.po` format (there is no `libintl`/`gettext`
dependency; the parser is `src/lang.cpp`). An entry looks like:

```
msgid "Original English text"
msgstr "翻訳されたテキスト"
```

Supported features:

- **`msgid`/`msgstr` pairs.** An empty `msgstr ""` (or an absent entry)
  means "not yet translated" and falls back to the English `msgid` at
  lookup time.
- **`msgctxt` disambiguation.** A `msgctxt "context"` line immediately
  before `msgid` disambiguates two unrelated strings that happen to share
  the same English text (e.g. the unidentified-item flavor word `"Gold"`
  vs. the character sheet's plain `"Gold"` label). The catalog key becomes
  `context + '\x04' + msgid`, matching the gettext convention. Looked up
  via `C_(ctx, msgid)` instead of the context-free `_(msgid)`.
- **Multi-line string concatenation.** A `msgid`/`msgstr` may be followed
  by additional bare `"..."` lines; they are concatenated in order. Used
  for long strings.
- **Escapes.** `\\`, `\"`, `\n`, `\t` are recognized inside quoted
  strings; nothing else is.

Lookup functions (declared in `src/lang.h`):

- `_(msgid)` — the workhorse. Returns the active catalog's translation,
  or `msgid` unchanged if there's no entry or the entry is empty.
- `C_(ctx, msgid)` — same, but scoped to a `msgctxt`.
- `lang::format(tmpl, {args...})` — positional `%s`-only substitution for
  translated sentence templates (not printf-style; a translation may
  reorder or omit spaces between placeholders, e.g. Japanese often
  concatenates two `%s` with no separator).
- `lang::loadCatalog(language)` / `lang::resolveLanguage(cli_lang)` /
  `lang::currentLanguage()` — catalog loading and the active-language
  state, described next.

## Language resolution

The active language is chosen once at startup, in priority order:

1. The `-l <lang>` command-line flag, if given.
2. The `MORIA_LANG` environment variable, if set and non-empty.
3. The first two characters of the `LANG` environment variable, then
   `LC_ALL`, if either is set.
4. `"en"` (the default; no catalog file is loaded for it, so `_()`
   becomes a pass-through for every `msgid`).

The resolved language name selects `data/lang/<lang>/umoria.po`. If that
file doesn't exist, or a specific `msgid` has no entry (or an empty one)
in it, the English source string is used automatically — there is no
hard failure for missing translations.

Some auxiliary text files (e.g. `help.txt`) also have a per-language
counterpart under `data/lang/<lang>/`; check `src/game.cpp`/`src/ui.cpp`
for exactly which filenames are language-aware if you're adding a new
one.

## Hardcoded language branches

Run this to regenerate the current list (do not rely on line numbers
below — they drift as the code changes):

```
grep -rn 'currentLanguage() == ' src/
```

As of this writing there are 11 call sites, all doing the same kind of
thing: **Japanese has no articles ("a"/"an"/"the") and no plural
inflection**, so the English logic that inserts an article or turns a
`~` template marker into `-s`/`-es` is replaced with a bare-noun form
when the active language is Japanese. This is genuinely
language-specific grammar, not something a translator can fix by editing
`.po` text alone — hence the hardcoding rather than a data-driven rule.

- `src/identification.cpp` — `itemDescription()` — drops the `~`
  plural-count marker unconditionally (Japanese has no plural
  inflection), instead of English's count-dependent `-s`/`-es`
  substitution.
- `src/identification.cpp` — `itemDescription()` — renders `&`-prefixed
  item names without an indefinite article (`"a"`/`"an"`) or the English
  vowel-sound check that picks between them.
- `src/identification.cpp` — `itemDescription()` — the "no more X" empty-
  stack message skips the English-only `"some "`-prefix stripping step
  (Japanese source strings never have that prefix to strip).
- `src/identification.cpp` — `objectBlockedByMonster()` — the "The X is
  in your way!" message uses a bare monster name instead of `"The X"`.
- `src/monster.cpp` — `monsterAttackPlayer()` — the monster-attacks-you
  message uses a bare monster name instead of `"The X "`.
- `src/monster.cpp` — `monsterCastSpell()` — the monster-casts-a-spell
  message uses a bare monster name instead of `"The X "`.
- `src/player.cpp` — `playerDiedFromString()` — the death-cause string
  ("killed by a X") uses a bare monster name instead of choosing between
  `"The X"`/`"a X"`/`"an X"`.
- `src/player.cpp` — `playerAttackMonster()` — the melee-attack message
  uses a bare monster name instead of `"the X"`.
- `src/player_bash.cpp` — `playerBashAttack()` — same as the previous
  entry, for bash attacks.
- `src/spells.cpp` — `printBoltStrikesMonsterMessage()` — the
  bolt-spell-hits-monster message uses a bare monster name (or `"it"`)
  instead of `"the X"`.
- `src/spells.cpp` — `spellGenocide()` — the "X is unaffected" genocide
  message uses a bare monster name instead of `"The X"`.

**If you're adding a new language**: for a language that also has no
articles/plural inflection (many East Asian languages), you likely want
the same bare-noun behavior — either extend each branch's condition
(`lang::currentLanguage() == "ja" || lang::currentLanguage() == "<new>"`)
or, better, generalize the condition to a small per-language trait (e.g.
"has articles") if you're adding more than one such language. For a
language that does use articles/plural inflection like English, add a
new `else if` branch alongside the Japanese one with that language's own
grammar rule — don't fall through to the English `else` branch, since
that produces English grammar with translated words, which reads as
broken.

## Display width helpers

Declared in `src/helpers.h`. Necessary because translated text is
multi-byte UTF-8, and naive C string handling (`strlen`, `%-N.Ns`
printf specifiers, fixed-byte-offset cursor placement) either counts
bytes instead of terminal columns or can slice a multi-byte character in
half.

| Function | Use it when... |
|---|---|
| `displayWidth(str)` | You need the on-screen column width of a string (e.g. to place a cursor after a translated prompt, or to right-align a number after a variable-width label). |
| `truncateToWidth(str, columns)` | You need to cut a string to fit a column budget without ever splitting a multi-byte character. |
| `padToDisplayWidth(str, width)` | You're writing translated text into a fixed-width screen field (e.g. a label column that must be blanked out again when a status expires). This is the safe replacement for a raw `%-N.Ns` printf specifier, which counts bytes and both mis-pads and can corrupt multi-byte text. |
| `truncateToByteCapacity(str, max_bytes)` | You're copying into a fixed-size **byte** buffer (e.g. `vtype_t`), not a screen column — this budgets raw bytes (still always cutting on a character boundary), unlike `truncateToWidth`, which budgets display columns. |
| `replaceLastCharSafely(str, replacement)` | You need to overwrite the last character of a C string in place (e.g. swapping a trailing punctuation mark) and that character might be multi-byte (e.g. a translated `"."` that became the full-width `"。"`). |

A wide-character build of ncurses (`ncursesw`) is required for any of
this to render correctly; the CMake build selects it automatically on
Linux/macOS (see the top-level `README.md` for build instructions).

## Pseudo-locale testing

Before writing real translations for a new language, you can stress-test
every screen's layout logic without any translation data:

```
python3 scripts/make_pseudo_po.py
MORIA_LANG=pseudo umoria
```

`make_pseudo_po.py` derives `data/lang/pseudo/umoria.po` mechanically
from `data/lang/umoria.pot`: every ASCII letter in each `msgid` is
replaced by its Unicode fullwidth form (`"Hello"` → `"Ｈｅｌｌｏ"`),
which occupies 2 display columns per character — a deliberately
worst-case width, similar to Japanese kana/kanji. `printf` format
specifiers, the `&`/`~` item-description template markers, and escape
sequences are left untouched. This generated catalog is not committed to
the repository; regenerate it locally whenever you want to run this
check. Any layout bug this catches (misaligned columns, overwritten
neighboring fields, truncated labels) will also show up with real wide
text, so it's worth running after any UI change, not just when adding a
language.

## Known limitations of `scripts/extract_strings.py`

This script scans `_(...)`-wrapped string literals in `src/*.cpp`
directly, plus the data tables listed in its `DATA_TABLE_SOURCES` list
(monster names, item names, race/class names, class rank titles, spell
names, store owners, store dialogue lines, and the unidentified-item
flavor-word tables, among others), and writes `data/lang/umoria.pot`, the
translation template that per-language `.po` files are built from.

An entry in `DATA_TABLE_SOURCES` is a tuple of (source file under `src/`,
the array declaration line to match, whether to extract the `"first"`
quoted string per entry line or `"all"` of them, and an optional 4th
element giving a `msgctxt` string for tables whose entries need
disambiguation from an unrelated identical `msgid` elsewhere).

**What it does not find**: any table not listed in `DATA_TABLE_SOURCES`.
The notable case is `character_backgrounds[]` in `src/data_player.cpp`
(the `Background_t` array of character-history/backstory paragraphs) —
these are not currently extracted or translated, and would need either a
new `DATA_TABLE_SOURCES` entry (if they're short, template-friendly
strings) or a different approach entirely (they're free-form prose
built by string concatenation, not single swappable strings). If you add
a new hardcoded string table anywhere in `src/`, check whether it should
be added to `DATA_TABLE_SOURCES` rather than assuming `.pot` extraction
covers it automatically.

## Known untranslated categories in the `ja` catalog

`data/lang/ja/umoria.po` does not have entries for every `msgid` in
`data/lang/umoria.pot`. Missing entries fall back to English
automatically (see "Catalog format" above), so this is not a bug, but a
deliberate scoping decision worth knowing about before you go looking
for a translation that "should" be there. As of this writing there are
203 such `msgid`s, all from three sources:

- `src/recall.cpp` and `src/data_recall.cpp` — the "recall" (monster
  memory, `r` command) screen builds its description as natural-language
  sentences assembled from several word-bank arrays concatenated
  together at runtime, not as single swappable strings. Translating the
  word banks in isolation would not produce grammatical Japanese
  sentences; this needs sentence-template work (`lang::format()`-style
  positional substitution, or a rewrite of the assembly logic itself),
  not a plain `.po` entry per array element.
- `src/wizard.cpp` — the wizard/debug command screens (`^W`). Low
  priority: these are developer-only screens not part of normal
  gameplay.

Run `python3 scripts/extract_strings.py` and diff `data/lang/umoria.pot`
against the `msgid`s present in `data/lang/ja/umoria.po` to regenerate
an up-to-date list if you want to pick one of these up.
