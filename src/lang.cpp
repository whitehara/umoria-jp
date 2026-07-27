// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Minimal i18n message catalog. See lang.h for the public interface.

// NOTE: <fstream> must be included before "headers.h": ui.h #defines
// `open` to `topen` (for the setuid tilde-expansion wrapper), and that
// macro corrupts std::basic_filebuf::open() if <fstream> is parsed
// after the #define is active.
#include <fstream>
#include <unordered_map>

#include "headers.h"

namespace {
std::unordered_map<std::string, std::string> catalog;

// Decodes the small escape subset we support: \\ \" \n \t
std::string decodeEscapes(const std::string &raw) {
    std::string out;
    out.reserve(raw.size());

    for (size_t i = 0; i < raw.size(); i++) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            char next = raw[i + 1];
            if (next == 'n') {
                out += '\n';
                i++;
                continue;
            }
            if (next == 't') {
                out += '\t';
                i++;
                continue;
            }
            if (next == '"' || next == '\\') {
                out += next;
                i++;
                continue;
            }
        }
        out += raw[i];
    }

    return out;
}

// Extracts and decodes the double-quoted string portion of a .po line
// (everything between the first and last '"' on the line). Returns
// false if the line doesn't contain a well-formed quoted string.
bool extractQuoted(const std::string &line, std::string &out) {
    size_t first = line.find('"');
    size_t last = line.rfind('"');

    if (first == std::string::npos || last == std::string::npos || last <= first) {
        return false;
    }

    out = decodeEscapes(line.substr(first + 1, last - first - 1));

    return true;
}

std::string trim(const std::string &str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }

    size_t end = str.find_last_not_of(" \t\r\n");

    return str.substr(start, end - start + 1);
}

bool lineStartsWithKeyword(const std::string &line, const std::string &keyword) {
    if (line.rfind(keyword, 0) != 0) {
        return false;
    }

    // Must be followed by whitespace or a quote, not just be a prefix
    // of a longer identifier.
    return line.size() == keyword.size() || line[keyword.size()] == ' ' || line[keyword.size()] == '"';
}
} // namespace

namespace lang {

std::string resolveLanguage(const std::string &cli_lang) {
    if (!cli_lang.empty()) {
        return cli_lang;
    }

    const char *moria_lang_env = std::getenv("MORIA_LANG");
    if (moria_lang_env != nullptr && moria_lang_env[0] != '\0') {
        return std::string(moria_lang_env);
    }

    for (const char *var : {"LANG", "LC_ALL"}) {
        const char *env = std::getenv(var);
        if (env != nullptr && env[0] != '\0') {
            std::string value(env);
            return value.substr(0, std::min<size_t>(2, value.size()));
        }
    }

    return "en";
}

bool loadCatalog(const std::string &language) {
    config::language::current = language;
    catalog.clear();

    std::ifstream file("data/lang/" + language + "/umoria.po");
    if (!file.is_open()) {
        return false;
    }

    std::string current_msgid;
    std::string current_msgstr;
    std::string current_msgctxt;
    bool have_entry = false; // true once a "msgid" line has been seen for the pending entry
    int continuing = 0;      // which field bare quoted continuation lines append to: 0=none, 1=msgid, 2=msgstr, 3=msgctxt

    auto commitEntry = [&]() {
        if (have_entry && !current_msgid.empty()) {
            // A msgctxt-qualified entry (e.g. flavor words) is keyed as
            // "ctx\x04msgid" -- the gettext convention -- so it can never
            // collide with an unrelated context-free entry that happens
            // to share the same msgid text.
            std::string key = current_msgctxt.empty() ? current_msgid : current_msgctxt + '\x04' + current_msgid;
            catalog[key] = current_msgstr;
        }
        current_msgid.clear();
        current_msgstr.clear();
        current_msgctxt.clear();
        have_entry = false;
        continuing = 0;
    };

    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = trim(line);

        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        if (lineStartsWithKeyword(trimmed, "msgctxt")) {
            // msgctxt (when present) precedes msgid in the same entry,
            // so it starts a new entry just like msgid does.
            if (have_entry) {
                commitEntry();
            }

            std::string value;
            if (extractQuoted(trimmed, value)) {
                current_msgctxt = value;
            }
            continuing = 3;
            continue;
        }

        if (lineStartsWithKeyword(trimmed, "msgid")) {
            // A new msgid starts a new entry; commit whatever was pending.
            if (have_entry) {
                commitEntry();
            }

            std::string value;
            if (extractQuoted(trimmed, value)) {
                current_msgid = value;
            }
            have_entry = true;
            continuing = 1;
            continue;
        }

        if (lineStartsWithKeyword(trimmed, "msgstr")) {
            if (!have_entry) {
                // Orphaned msgstr with no preceding msgid in this entry —
                // ignore it rather than let it leak into the next entry.
                continuing = 0;
                continue;
            }

            std::string value;
            if (extractQuoted(trimmed, value)) {
                current_msgstr = value;
            }
            continuing = 2;
            continue;
        }

        // A bare quoted continuation line appends to whichever field
        // we were last filling in.
        if (trimmed[0] == '"' && continuing != 0) {
            std::string value;
            if (extractQuoted(trimmed, value)) {
                if (continuing == 1) {
                    current_msgid += value;
                } else if (continuing == 3) {
                    current_msgctxt += value;
                } else {
                    current_msgstr += value;
                }
            }
        }
    }

    commitEntry();

    return true;
}

const std::string &currentLanguage() {
    return config::language::current;
}

std::string format(const std::string &tmpl, std::initializer_list<std::string> args) {
    std::string out;
    out.reserve(tmpl.size());

    auto it = args.begin();

    for (size_t i = 0; i < tmpl.size(); i++) {
        if (tmpl[i] == '%' && i + 1 < tmpl.size() && tmpl[i + 1] == 's') {
            if (it != args.end()) {
                out += *it;
                ++it;
            } else {
                out += "%s"; // more placeholders than args: leave as-is.
            }
            i++;
            continue;
        }

        out += tmpl[i];
    }

    return out;
}

} // namespace lang

const char *_(const char *msgid) {
    if (msgid == nullptr) {
        return msgid;
    }

    auto it = catalog.find(msgid);
    if (it == catalog.end() || it->second.empty()) {
        return msgid;
    }

    return it->second.c_str();
}

std::string _(const std::string &msgid) {
    auto it = catalog.find(msgid);
    if (it == catalog.end() || it->second.empty()) {
        return msgid;
    }

    return it->second;
}

const char *C_(const char *ctx, const char *msgid) {
    if (msgid == nullptr) {
        return msgid;
    }

    std::string key = std::string(ctx) + '\x04' + msgid;

    auto it = catalog.find(key);
    if (it == catalog.end() || it->second.empty()) {
        return msgid;
    }

    return it->second.c_str();
}
