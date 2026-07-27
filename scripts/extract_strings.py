#!/usr/bin/env python3
# Copyright (c) 1981-86 Robert A. Koeneke
# Copyright (c) 1987-94 James E. Wilson
#
# SPDX-License-Identifier: GPL-3.0-or-later

"""Scans src/*.cpp for `_("...")` catalog lookups and writes
data/lang/umoria.pot: one msgid/msgstr entry per unique translated
string, with `#:` reference comments pointing back to every call site.
Empty msgstr is the "untranslated" marker our .po loader (src/lang.cpp)
already understands, so umoria.pot doubles as a template for new
per-language umoria.po files.

Phase 4 also wraps *reads* of data-table name fields (e.g. `_(creature.name)`,
`_(character_races[i].name)`) rather than literal strings, since those
tables' entries are the actual runtime msgids. Those never appear as
`_("...")` in the source, so this script additionally parses the raw
name-field literals out of the data tables listed in DATA_TABLE_SOURCES
and adds them to the same catalog (with a `#:` reference pointing at the
data-table definition, not a call site).

Phase 10 adds an optional 4th element to a DATA_TABLE_SOURCES entry: a
gettext-style msgctxt context string. This disambiguates a data-table
msgid that would otherwise collide with an unrelated context-free msgid
elsewhere (e.g. the unidentified-item flavor word "Gold" vs. the
character sheet's plain "Gold" label) -- each (context, msgid) pair
becomes its own .pot entry, with a `msgctxt "..."` line emitted before
`msgid` only when a context was given, so every pre-phase-10 entry's
.pot output is byte-for-byte unchanged.

Usage: python3 scripts/extract_strings.py
Writes data/lang/umoria.pot relative to the repository root (this
script's grandparent directory).
"""

import re
import sys
from pathlib import Path

STRING_LIT_RE = re.compile(r'_\(\s*"((?:[^"\\]|\\.)*)"\s*\)')

# Each entry: (file relative to src/, array declaration line prefix,
# "first" to take only the first quoted string per entry line (name
# field followed by other, non-string fields) or "all" to take every
# quoted string on a line (a table that is nothing but a list of names,
# e.g. per-level class rank titles), optional 4th element = msgctxt
# context string (see the phase 10 note in this module's docstring).
DATA_TABLE_SOURCES = [
    ("data_creatures.cpp", "Creature_t creatures_list[", "first"),
    ("data_treasure.cpp", "DungeonObject_t game_objects[", "first"),
    ("data_treasure.cpp", "const char *special_item_names[", "all"),
    ("data_player.cpp", "Race_t character_races[", "first"),
    ("data_player.cpp", "Class_t classes[", "first"),
    ("data_player.cpp", "ClassRankTitle_t class_rank_titles[", "all"),
    ("data_player.cpp", "const char *spell_names[", "all"),
    ("data_store_owners.cpp", "Owner_t store_owners[", "first"),
    ("ui.cpp", "static const char *stat_names[] = {", "all"),
    ("game.cpp", "} game_options[] = {", "first"),
    ("data_recall.cpp", "const char *recall_description_attack_type[25] = {", "all"),
    ("data_recall.cpp", "const char *recall_description_attack_method[20] = {", "all"),
    ("data_recall.cpp", "const char *recall_description_how_much[8] = {", "all"),
    ("data_recall.cpp", "const char *recall_description_move[6] = {", "all"),
    ("data_recall.cpp", "const char *recall_description_spell[15] = {", "all"),
    ("data_recall.cpp", "const char *recall_description_breath[5] = {", "all"),
    ("data_recall.cpp", "const char *recall_description_weakness[6] = {", "all"),
    ("data_store_owners.cpp", "const char *speech_sale_accepted[14] = {", "all"),
    ("data_store_owners.cpp", "const char *speech_selling_haggle_final[3] = {", "all"),
    ("data_store_owners.cpp", "const char *speech_selling_haggle[16] = {", "all"),
    ("data_store_owners.cpp", "const char *speech_buying_haggle_final[3] = {", "all"),
    ("data_store_owners.cpp", "const char *speech_buying_haggle[15] = {", "all"),
    ("data_store_owners.cpp", "const char *speech_insulted_haggling_done[5] = {", "all"),
    ("data_store_owners.cpp", "const char *speech_get_out_of_my_store[5] = {", "all"),
    ("data_store_owners.cpp", "const char *speech_haggling_try_again[10] = {", "all"),
    ("data_store_owners.cpp", "const char *speech_sorry[5] = {", "all"),
    ("data_tables.cpp", "const char *colors[MAX_COLORS] = {", "all", "flavor"),
    ("data_tables.cpp", "const char *mushrooms[MAX_MUSHROOMS] = {", "all", "flavor"),
    ("data_tables.cpp", "const char *woods[MAX_WOODS] = {", "all", "flavor"),
    ("data_tables.cpp", "const char *metals[MAX_METALS] = {", "all", "flavor"),
    ("data_tables.cpp", "const char *rocks[MAX_ROCKS] = {", "all", "flavor"),
    ("data_tables.cpp", "const char *amulets[MAX_AMULETS] = {", "all", "flavor"),
]

# A quoted name field, with an optional leading "{" for tables whose
# entries open with `{"Name", ...other fields...}` on one line; tables
# whose entries spread the name onto its own line (e.g. Race_t) have no
# leading "{" on that line, so it's optional, not required.
FIRST_QUOTED_RE = re.compile(r'^\s*\{?\s*"((?:[^"\\]|\\.)*)"')
ALL_QUOTED_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')


def find_repo_root():
    return Path(__file__).resolve().parent.parent


def extract_from_file(path, repo_root):
    entries = []
    with open(path, "r", encoding="utf-8") as f:
        for line_no, line in enumerate(f, start=1):
            for m in STRING_LIT_RE.finditer(line):
                msgid = m.group(1)
                rel_path = path.relative_to(repo_root)
                # Literal `_("...")` call sites are always context-free.
                entries.append((msgid, f"{rel_path}:{line_no}", None))
    return entries


def extract_data_table_names(repo_root):
    """Pulls the literal name-field values out of the big data tables
    (src/data_creatures.cpp, data_treasure.cpp, data_player.cpp's race/
    class/rank-title tables, data_store_owners.cpp) so they become
    catalog entries even though the source never spells them as a
    literal `_("...")` call (the code reads them at runtime instead,
    e.g. `_(creature.name)`)."""
    entries = []
    src_dir = repo_root / "src"

    for source in DATA_TABLE_SOURCES:
        filename, decl_prefix, mode = source[0], source[1], source[2]
        context = source[3] if len(source) > 3 else None
        path = src_dir / filename
        with open(path, "r", encoding="utf-8") as f:
            lines = f.readlines()

        start = None
        for i, line in enumerate(lines):
            if line.startswith(decl_prefix):
                start = i
                break
        if start is None:
            raise RuntimeError(f"Could not find array declaration {decl_prefix!r} in {filename}")

        end = None
        for i in range(start + 1, len(lines)):
            if lines[i].rstrip("\n") == "};":
                end = i
                break
        if end is None:
            raise RuntimeError(f"Could not find closing '}};' for {decl_prefix!r} in {filename}")

        rel_path = path.relative_to(repo_root)
        ref = f"{rel_path}:{start + 1} ({decl_prefix.split()[0]} table)"

        for line in lines[start + 1 : end]:
            if mode == "first":
                m = FIRST_QUOTED_RE.match(line)
                if m:
                    entries.append((m.group(1), ref, context))
            else:  # "all"
                for m in ALL_QUOTED_RE.finditer(line):
                    entries.append((m.group(1), ref, context))

    return entries


def main():
    repo_root = find_repo_root()
    src_dir = repo_root / "src"

    # (context, msgid) -> list of "file:line" references, in first-seen
    # order. context is None for every context-free entry (the vast
    # majority, and all pre-phase-10 entries), so their .pot output is
    # unaffected; only a (context, msgid) pair with a real context string
    # gets a msgctxt line.
    catalog = {}
    order = []

    def add_entry(msgid, ref, context):
        key = (context, msgid)
        if key not in catalog:
            catalog[key] = []
            order.append(key)
        catalog[key].append(ref)

    for path in sorted(src_dir.glob("*.cpp")):
        for msgid, ref, context in extract_from_file(path, repo_root):
            add_entry(msgid, ref, context)

    data_table_entries = extract_data_table_names(repo_root)
    for msgid, ref, context in data_table_entries:
        add_entry(msgid, ref, context)

    out_path = repo_root / "data" / "lang" / "umoria.pot"
    out_path.parent.mkdir(parents=True, exist_ok=True)

    with open(out_path, "w", encoding="utf-8") as out:
        out.write("# Umoria i18n message template. Generated by scripts/extract_strings.py.\n")
        out.write("# Do not edit by hand; regenerate after adding/removing _() call sites.\n")
        out.write("# To translate: copy to data/lang/<lang>/umoria.po and fill in msgstr lines.\n")
        for context, msgid in order:
            out.write("#\n")
            for ref in catalog[(context, msgid)]:
                out.write(f"#: {ref}\n")
            if context is not None:
                out.write(f'msgctxt "{context}"\n')
            out.write(f'msgid "{msgid}"\n')
            out.write('msgstr ""\n')

    total_call_sites = sum(len(refs) for refs in catalog.values())
    print(f"Scanned {len(list(src_dir.glob('*.cpp')))} files in {src_dir.relative_to(repo_root)}")
    print(f"  + {len(data_table_entries)} data-table name entries from {len(DATA_TABLE_SOURCES)} tables")
    print(f"Wrote {out_path.relative_to(repo_root)}: {len(order)} unique entries, {total_call_sites} call sites")

    return 0


if __name__ == "__main__":
    sys.exit(main())
