// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "headers.h"

static const char *stat_names[] = {
    "STR : ", "INT : ", "WIS : ", "DEX : ", "CON : ", "CHR : ",
};

// Track screen changes for inventory commands
bool screen_has_changed = false;

bool message_ready_to_print;            // Set with first message
vtype_t messages[MESSAGE_HISTORY_SIZE]; // Saved message history -CJS-
int16_t last_message_id = 0;            // Index of last message held in saved messages array

// Calculates current boundaries -RAK-
static void panelBounds() {
    dg.panel.top = dg.panel.row * (SCREEN_HEIGHT / 2);
    dg.panel.bottom = dg.panel.top + SCREEN_HEIGHT - 1;
    dg.panel.row_prt = dg.panel.top - 1;
    dg.panel.left = dg.panel.col * (SCREEN_WIDTH / 2);
    dg.panel.right = dg.panel.left + SCREEN_WIDTH - 1;
    dg.panel.col_prt = dg.panel.left - 13;
}

// Given an row (y) and col (x), this routine detects -RAK-
// when a move off the screen has occurred and figures new borders.
// `force` forces the panel bounds to be recalculated, useful for 'W'here.
bool coordOutsidePanel(Coord_t coord, bool force) {
    Coord_t panel = Coord_t{dg.panel.row, dg.panel.col};

    if (force || coord.y < dg.panel.top + 2 || coord.y > dg.panel.bottom - 2) {
        panel.y = (coord.y - SCREEN_HEIGHT / 4) / (SCREEN_HEIGHT / 2);

        if (panel.y > dg.panel.max_rows) {
            panel.y = dg.panel.max_rows;
        } else if (panel.y < 0) {
            panel.y = 0;
        }
    }

    if (force || coord.x < dg.panel.left + 3 || coord.x > dg.panel.right - 3) {
        panel.x = ((coord.x - SCREEN_WIDTH / 4) / (SCREEN_WIDTH / 2));
        if (panel.x > dg.panel.max_cols) {
            panel.x = dg.panel.max_cols;
        } else if (panel.x < 0) {
            panel.x = 0;
        }
    }

    if (panel.y != dg.panel.row || panel.x != dg.panel.col) {
        dg.panel.row = panel.y;
        dg.panel.col = panel.x;
        panelBounds();

        // stop movement if any
        if (config::options::find_bound) {
            playerEndRunning();
        }

        // Yes, the coordinates are beyond the current panel boundary
        return true;
    }

    return false;
}

// Is the given coordinate within the screen panel boundaries -RAK-
bool coordInsidePanel(Coord_t coord) {
    bool valid_y = coord.y >= dg.panel.top && coord.y <= dg.panel.bottom;
    bool valid_x = coord.x >= dg.panel.left && coord.x <= dg.panel.right;

    return valid_y && valid_x;
}

// Prints the map of the dungeon -RAK-
void drawDungeonPanel() {
    int line = 1;

    Coord_t coord = Coord_t{0, 0};

    // Top to bottom
    for (coord.y = dg.panel.top; coord.y <= dg.panel.bottom; coord.y++) {
        eraseLine(Coord_t{line, 13});
        line++;

        // Left to right
        for (coord.x = dg.panel.left; coord.x <= dg.panel.right; coord.x++) {
            char ch = caveGetTileSymbol(coord);
            if (ch != ' ') {
                panelPutTile(ch, coord);
            }
        }
    }
}

// Draws entire screen -RAK-
void drawCavePanel() {
    clearScreen();
    printCharacterStatsBlock();
    drawDungeonPanel();
    printCharacterCurrentDepth();
}

// We need to reset the view of things. -CJS-
void dungeonResetView() {
    Tile_t const &tile = dg.floor[py.pos.y][py.pos.x];

    // Check for new panel
    if (coordOutsidePanel(py.pos, false)) {
        drawDungeonPanel();
    }

    // Move the light source
    dungeonMoveCharacterLight(py.pos, py.pos);

    // A room of light should be lit.
    if (tile.feature_id == TILE_LIGHT_FLOOR) {
        if (py.flags.blind < 1 && !tile.permanent_light) {
            dungeonLightRoom(py.pos);
        }
        return;
    }

    // In doorway of light-room?
    if (tile.perma_lit_room && py.flags.blind < 1) {
        for (int i = py.pos.y - 1; i <= py.pos.y + 1; i++) {
            for (int j = py.pos.x - 1; j <= py.pos.x + 1; j++) {
                if (dg.floor[i][j].feature_id == TILE_LIGHT_FLOOR && !dg.floor[i][j].permanent_light) {
                    dungeonLightRoom(Coord_t{i, j});
                }
            }
        }
    }
}

// Converts stat num into string
// NOTE: this function assumes the stat_string is a max of MORIA_MESSAGE_SIZE
void statsAsString(uint8_t stat, char *stat_string) {
    int percentile = stat - 18;

    if (stat <= 18) {
        (void) snprintf(stat_string, MORIA_MESSAGE_SIZE, "%6d", stat);
    } else if (percentile == 100) {
        (void) strcpy(stat_string, "18/100");
    } else {
        (void) snprintf(stat_string, MORIA_MESSAGE_SIZE, " 18/%02d", percentile);
    }
}

// Print character stat in given row, column -RAK-
void displayCharacterStats(int stat) {
    char text[MORIA_MESSAGE_SIZE];
    statsAsString(py.stats.used[stat], text);
    const char *label = _(stat_names[stat]);
    putString(label, Coord_t{6 + stat, STAT_COLUMN});
    // Placed immediately after the label's own display width (not a
    // hardcoded "+6"), so a translated label of a different display
    // width than the English original never has its last character
    // clobbered by this value — see printCharacterStats() for the same
    // fix, and the phase 5 bug report it addresses.
    putString(text, Coord_t{6 + stat, STAT_COLUMN + displayWidth(label)});
}

// Print character info in given row, column -RAK-
// The longest title is 13 characters, so only pad to 13
static void printCharacterInfoInField(const char *info, Coord_t coord) {
    // Truncate-or-pad to exactly 13 display columns in one call, so a
    // translated (e.g. Japanese) value wider than the English original is
    // clipped to the field's budget instead of overflowing into the
    // dungeon map area drawn immediately to its right.
    putString(padToDisplayWidth(info, 13).c_str(), coord);
}

// Print long number with header at given row, column
//
// `header_width` is the header's *design* display-column budget (the
// length of the untranslated English literal, including its trailing
// padding spaces -- pass e.g. `sizeof("Age          ") - 1`). The
// (possibly translated) header is padded/truncated to that budget before
// the number is appended, so a translated header wider than the English
// original can never push the number past its own fixed field width and
// collide with whatever is drawn next on the same row.
static void printHeaderLongNumber(const char *header, int32_t num, Coord_t coord, int header_width) {
    vtype_t str = {'\0'};
    std::string padded_header = padToDisplayWidth(header, header_width);
    (void) snprintf(str, MORIA_MESSAGE_SIZE, _("%s: %6d"), padded_header.c_str(), num);
    putString(str, coord);
}

// Print long number (7 digits of space) with header at given row, column
static void printHeaderLongNumber7Spaces(const char *header, int32_t num, Coord_t coord, int header_width) {
    vtype_t str = {'\0'};
    std::string padded_header = padToDisplayWidth(header, header_width);
    (void) snprintf(str, MORIA_MESSAGE_SIZE, _("%s: %7d"), padded_header.c_str(), num);
    putString(str, coord);
}

// Print number with header at given row, column -RAK-
static void printHeaderNumber(const char *header, int num, Coord_t coord, int header_width) {
    vtype_t str = {'\0'};
    std::string padded_header = padToDisplayWidth(header, header_width);
    (void) snprintf(str, MORIA_MESSAGE_SIZE, _("%s: %6d"), padded_header.c_str(), num);
    putString(str, coord);
}

// Print long number at given row, column
static void printLongNumber(int32_t num, Coord_t coord) {
    vtype_t str = {'\0'};
    (void) snprintf(str, MORIA_MESSAGE_SIZE, "%6d", num);
    putString(str, coord);
}

// Print number at given row, column -RAK-
static void printNumber(int num, Coord_t coord) {
    vtype_t str = {'\0'};
    (void) snprintf(str, MORIA_MESSAGE_SIZE, "%6d", num);
    putString(str, coord);
}

// Prints title of character -RAK-
void printCharacterTitle() {
    printCharacterInfoInField(playerRankTitle(), Coord_t{4, STAT_COLUMN});
}

// Prints level -RAK-
void printCharacterLevel() {
    printNumber((int) py.misc.level, Coord_t{13, STAT_COLUMN + 6});
}

// Prints players current mana points. -RAK-
void printCharacterCurrentMana() {
    printNumber(py.misc.current_mana, Coord_t{15, STAT_COLUMN + 6});
}

// Prints Max hit points -RAK-
void printCharacterMaxHitPoints() {
    printNumber(py.misc.max_hp, Coord_t{16, STAT_COLUMN + 6});
}

// Prints players current hit points -RAK-
void printCharacterCurrentHitPoints() {
    printNumber(py.misc.current_hp, Coord_t{17, STAT_COLUMN + 6});
}

// prints current AC -RAK-
void printCharacterCurrentArmorClass() {
    printNumber(py.misc.display_ac, Coord_t{19, STAT_COLUMN + 6});
}

// Prints current gold -RAK-
void printCharacterGoldValue() {
    printLongNumber(py.misc.au, Coord_t{20, STAT_COLUMN + 6});
}

// Prints depth in stat area -RAK-
void printCharacterCurrentDepth() {
    vtype_t depths = {'\0'};

    int depth = dg.current_level * 50;

    if (depth == 0) {
        (void) strcpy(depths, _("Town level"));
    } else {
        (void) snprintf(depths, MORIA_MESSAGE_SIZE, _("%d feet"), depth);
    }

    putStringClearToEOL(depths, Coord_t{23, 65});
}

// Each of these status-line fields occupies a fixed column span on row 23
// (hunger:0-6, blind:7-12, confused:13-21, fear:22-28, poisoned:29-37,
// movement:38-48, speed:49-57). `padToDisplayWidth()` pads (or blanks) the
// translated text to that field's exact width in display columns, so a
// translated (e.g. Japanese) string of a different length than the
// original English never leaves stray characters behind when the status
// clears, and never runs into the next field.

// Prints status of hunger -RAK-
void printCharacterHungerStatus() {
    std::string text;
    if ((py.flags.status & config::player::status::PY_WEAK) != 0u) {
        text = _("Weak");
    } else if ((py.flags.status & config::player::status::PY_HUNGRY) != 0u) {
        text = _("Hungry");
    }
    putString(padToDisplayWidth(text, 7).c_str(), Coord_t{23, 0});
}

// Prints Blind status -RAK-
void printCharacterBlindStatus() {
    std::string text;
    if ((py.flags.status & config::player::status::PY_BLIND) != 0u) {
        text = _("Blind");
    }
    putString(padToDisplayWidth(text, 6).c_str(), Coord_t{23, 7});
}

// Prints Confusion status -RAK-
void printCharacterConfusedState() {
    std::string text;
    if ((py.flags.status & config::player::status::PY_CONFUSED) != 0u) {
        text = _("Confused");
    }
    putString(padToDisplayWidth(text, 9).c_str(), Coord_t{23, 13});
}

// Prints Fear status -RAK-
void printCharacterFearState() {
    std::string text;
    if ((py.flags.status & config::player::status::PY_FEAR) != 0u) {
        text = _("Afraid");
    }
    putString(padToDisplayWidth(text, 7).c_str(), Coord_t{23, 22});
}

// Prints Poisoned status -RAK-
void printCharacterPoisonedState() {
    std::string text;
    if ((py.flags.status & config::player::status::PY_POISONED) != 0u) {
        text = _("Poisoned");
    }
    putString(padToDisplayWidth(text, 9).c_str(), Coord_t{23, 29});
}

// Prints Searching, Resting, Paralysis, or 'count' status -RAK-
void printCharacterMovementState() {
    py.flags.status &= ~config::player::status::PY_REPEAT;

    std::string text;

    if (py.flags.paralysis > 1) {
        text = _("Paralysed");
        putString(padToDisplayWidth(text, 11).c_str(), Coord_t{23, 38});
        return;
    }

    if ((py.flags.status & config::player::status::PY_REST) != 0u) {
        if (py.flags.rest < 0) {
            text = _("Rest *");
        } else if (config::options::display_counts) {
            char rest_string[16];
            (void) snprintf(rest_string, sizeof(rest_string), _("Rest %-5d"), py.flags.rest);
            text = rest_string;
        } else {
            text = _("Rest");
        }

        putString(padToDisplayWidth(text, 11).c_str(), Coord_t{23, 38});

        return;
    }

    if (game.command_count > 0) {
        if (config::options::display_counts) {
            char repeat_string[16];
            (void) snprintf(repeat_string, sizeof(repeat_string), _("Repeat %-.3d"), game.command_count);
            text = repeat_string;
        } else {
            text = _("Repeat");
        }

        py.flags.status |= config::player::status::PY_REPEAT;

        if ((py.flags.status & config::player::status::PY_SEARCH) != 0u) {
            text = _("Search");
        }

        putString(padToDisplayWidth(text, 11).c_str(), Coord_t{23, 38});

        return;
    }

    if ((py.flags.status & config::player::status::PY_SEARCH) != 0u) {
        text = _("Searching");
    }

    putString(padToDisplayWidth(text, 11).c_str(), Coord_t{23, 38});
}

// Prints the speed of a character. -CJS-
void printCharacterSpeed() {
    int speed = py.flags.speed;

    // Search mode.
    if ((py.flags.status & config::player::status::PY_SEARCH) != 0u) {
        speed--;
    }

    std::string text;
    if (speed > 1) {
        text = _("Very Slow");
    } else if (speed == 1) {
        text = _("Slow");
    } else if (speed == -1) {
        text = _("Fast");
    } else if (speed < -1) {
        text = _("Very Fast");
    }

    putString(padToDisplayWidth(text, 9).c_str(), Coord_t{23, 49});
}

void printCharacterStudyInstruction() {
    py.flags.status &= ~config::player::status::PY_STUDY;

    // Same padToDisplayWidth pattern as the other status-line fields above:
    // blanks or displays the (translatable) text to a fixed display-column
    // width, so a translated "Study" of a different width than the English
    // original never leaves stray characters behind when the field clears.
    std::string text;
    if (py.flags.new_spells_to_learn != 0) {
        text = _("Study");
    }
    putString(padToDisplayWidth(text, 6).c_str(), Coord_t{23, 59});
}

// Prints winner status on display -RAK-
void printCharacterWinner() {
    // Truncated/padded to the sidebar's 13-column budget (same pattern as
    // printCharacterInfoInField() above), so a translated (e.g. Japanese)
    // string wider than the English original never overflows into the
    // dungeon map area drawn immediately to its right.
    if ((game.noscore & 0x2) != 0) {
        if (game.wizard_mode) {
            putString(padToDisplayWidth(_("Is wizard  "), 13).c_str(), Coord_t{22, 0});
        } else {
            putString(padToDisplayWidth(_("Was wizard "), 13).c_str(), Coord_t{22, 0});
        }
    } else if ((game.noscore & 0x1) != 0) {
        putString(padToDisplayWidth(_("Resurrected"), 13).c_str(), Coord_t{22, 0});
    } else if ((game.noscore & 0x4) != 0) {
        putString(padToDisplayWidth(_("Duplicate"), 13).c_str(), Coord_t{22, 0});
    } else if (game.total_winner) {
        putString(padToDisplayWidth(_("*Winner*   "), 13).c_str(), Coord_t{22, 0});
    }
}

// Prints character-screen info -RAK-
void printCharacterStatsBlock() {
    printCharacterInfoInField(_(character_races[py.misc.race_id].name), Coord_t{2, STAT_COLUMN});
    printCharacterInfoInField(_(classes[py.misc.class_id].title), Coord_t{3, STAT_COLUMN});
    printCharacterInfoInField(playerRankTitle(), Coord_t{4, STAT_COLUMN});

    for (int i = 0; i < 6; i++) {
        displayCharacterStats(i);
    }

    printHeaderNumber(_("LEV "), (int) py.misc.level, Coord_t{13, STAT_COLUMN}, (int) sizeof("LEV ") - 1);
    printHeaderLongNumber(_("EXP "), py.misc.exp, Coord_t{14, STAT_COLUMN}, (int) sizeof("EXP ") - 1);
    printHeaderNumber(_("MANA"), py.misc.current_mana, Coord_t{15, STAT_COLUMN}, (int) sizeof("MANA") - 1);
    printHeaderNumber(_("MHP "), py.misc.max_hp, Coord_t{16, STAT_COLUMN}, (int) sizeof("MHP ") - 1);
    printHeaderNumber(_("CHP "), py.misc.current_hp, Coord_t{17, STAT_COLUMN}, (int) sizeof("CHP ") - 1);
    printHeaderNumber(_("AC  "), py.misc.display_ac, Coord_t{19, STAT_COLUMN}, (int) sizeof("AC  ") - 1);
    printHeaderLongNumber(_("GOLD"), py.misc.au, Coord_t{20, STAT_COLUMN}, (int) sizeof("GOLD") - 1);
    printCharacterWinner();

    uint32_t status = py.flags.status;

    if (((config::player::status::PY_HUNGRY | config::player::status::PY_WEAK) & status) != 0u) {
        printCharacterHungerStatus();
    }

    if ((status & config::player::status::PY_BLIND) != 0u) {
        printCharacterBlindStatus();
    }

    if ((status & config::player::status::PY_CONFUSED) != 0u) {
        printCharacterConfusedState();
    }

    if ((status & config::player::status::PY_FEAR) != 0u) {
        printCharacterFearState();
    }

    if ((status & config::player::status::PY_POISONED) != 0u) {
        printCharacterPoisonedState();
    }

    if (((config::player::status::PY_SEARCH | config::player::status::PY_REST) & status) != 0u) {
        printCharacterMovementState();
    }

    // if speed non zero, print it, modify speed if Searching
    int16_t speed = py.flags.speed - (int16_t) ((status & config::player::status::PY_SEARCH) >> 8);
    if (speed != 0) {
        printCharacterSpeed();
    }

    // display the study field
    printCharacterStudyInstruction();
}

// Prints the following information on the screen. -JWT-
void printCharacterInformation() {
    clearScreen();

    std::string name_label = padToDisplayWidth(_("Name        :"), CHARACTER_SHEET_LABEL_WIDTH);
    std::string race_label = padToDisplayWidth(_("Race        :"), CHARACTER_SHEET_LABEL_WIDTH);
    std::string sex_label = padToDisplayWidth(_("Sex         :"), CHARACTER_SHEET_LABEL_WIDTH);
    std::string class_label = padToDisplayWidth(_("Class       :"), CHARACTER_SHEET_LABEL_WIDTH);

    putString(name_label.c_str(), Coord_t{2, 1});
    putString(race_label.c_str(), Coord_t{3, 1});
    putString(sex_label.c_str(), Coord_t{4, 1});
    putString(class_label.c_str(), Coord_t{5, 1});

    if (!game.character_generated) {
        return;
    }

    // Value columns are always CHARACTER_SHEET_LABEL_WIDTH + 2 (not derived
    // from the label's own possibly-wider display width), because the
    // labels above are already clamped to that same budget -- so a
    // translated (e.g. Japanese) label wider than the English original is
    // truncated to fit rather than pushing the value (and the fixed-column
    // Age/Height/Weight block just to the right of it) out of alignment.
    putString(py.misc.name, Coord_t{2, CHARACTER_SHEET_LABEL_WIDTH + 2});
    putString(_(character_races[py.misc.race_id].name), Coord_t{3, CHARACTER_SHEET_LABEL_WIDTH + 2});
    putString((playerGetGenderLabel()), Coord_t{4, CHARACTER_SHEET_LABEL_WIDTH + 2});
    putString(_(classes[py.misc.class_id].title), Coord_t{5, CHARACTER_SHEET_LABEL_WIDTH + 2});
}

// Prints the following information on the screen. -JWT-
void printCharacterStats() {
    for (int i = 0; i < 6; i++) {
        vtype_t buf = {'\0'};

        statsAsString(py.stats.used[i], buf);
        const char *label = _(stat_names[i]);
        putString(label, Coord_t{2 + i, 61});
        // Placed immediately after the label's own display width, so a
        // translated label (e.g. Japanese, no trailing filler space to
        // safely overwrite) never has its last character clobbered by
        // this value.
        putString(buf, Coord_t{2 + i, 61 + displayWidth(label)});

        if (py.stats.max[i] > py.stats.current[i]) {
            statsAsString(py.stats.max[i], buf);
            putString(buf, Coord_t{2 + i, 73});
        }
    }

    printHeaderNumber(_("+ To Hit    "), py.misc.display_to_hit, Coord_t{9, 1}, (int) sizeof("+ To Hit    ") - 1);
    printHeaderNumber(_("+ To Damage "), py.misc.display_to_damage, Coord_t{10, 1}, (int) sizeof("+ To Damage ") - 1);
    printHeaderNumber(_("+ To AC     "), py.misc.display_to_ac, Coord_t{11, 1}, (int) sizeof("+ To AC     ") - 1);
    printHeaderNumber(_("  Total AC  "), py.misc.display_ac, Coord_t{12, 1}, (int) sizeof("  Total AC  ") - 1);
}

// Returns a rating of x depending on y -JWT-
const char *statRating(Coord_t coord) {
    switch (coord.x / coord.y) {
        case -3:
        case -2:
        case -1:
            return _("Very Bad");
        case 0:
        case 1:
            return _("Bad");
        case 2:
            return _("Poor");
        case 3:
        case 4:
            return _("Fair");
        case 5:
            return _("Good");
        case 6:
            return _("Very Good");
        case 7:
        case 8:
            return _("Excellent");
        default:
            return _("Superb");
    }
}

// Prints age, height, weight, and SC -JWT-
void printCharacterVitalStatistics() {
    printHeaderNumber(_("Age          "), (int) py.misc.age, Coord_t{2, 38}, (int) sizeof("Age          ") - 1);
    printHeaderNumber(_("Height       "), (int) py.misc.height, Coord_t{3, 38}, (int) sizeof("Height       ") - 1);
    printHeaderNumber(_("Weight       "), (int) py.misc.weight, Coord_t{4, 38}, (int) sizeof("Weight       ") - 1);
    printHeaderNumber(_("Social Class "), (int) py.misc.social_class, Coord_t{5, 38}, (int) sizeof("Social Class ") - 1);
}

// Prints the following information on the screen. -JWT-
void printCharacterLevelExperience() {
    printHeaderLongNumber7Spaces(_("Level      "), (int32_t) py.misc.level, Coord_t{9, 28}, (int) sizeof("Level      ") - 1);
    printHeaderLongNumber7Spaces(_("Experience "), py.misc.exp, Coord_t{10, 28}, (int) sizeof("Experience ") - 1);
    printHeaderLongNumber7Spaces(_("Max Exp    "), py.misc.max_exp, Coord_t{11, 28}, (int) sizeof("Max Exp    ") - 1);

    if (py.misc.level >= PLAYER_MAX_LEVEL) {
        putStringClearToEOL(_("Exp to Adv.: *******"), Coord_t{12, 28});
    } else {
        printHeaderLongNumber7Spaces(_("Exp to Adv."), (int32_t) (py.base_exp_levels[py.misc.level - 1] * py.misc.experience_factor / 100), Coord_t{12, 28},
                                     (int) sizeof("Exp to Adv.") - 1);
    }

    printHeaderLongNumber7Spaces(_("Gold       "), py.misc.au, Coord_t{13, 28}, (int) sizeof("Gold       ") - 1);
    printHeaderNumber(_("Max Hit Points "), py.misc.max_hp, Coord_t{9, 52}, (int) sizeof("Max Hit Points ") - 1);
    printHeaderNumber(_("Cur Hit Points "), py.misc.current_hp, Coord_t{10, 52}, (int) sizeof("Cur Hit Points ") - 1);
    printHeaderNumber(_("Max Mana       "), py.misc.mana, Coord_t{11, 52}, (int) sizeof("Max Mana       ") - 1);
    printHeaderNumber(_("Cur Mana       "), py.misc.current_mana, Coord_t{12, 52}, (int) sizeof("Cur Mana       ") - 1);
}

// Prints ratings on certain abilities -RAK-
void printCharacterAbilities() {
    clearToBottom(14);

    int xbth = py.misc.bth + py.misc.plusses_to_hit * BTH_PER_PLUS_TO_HIT_ADJUST + (class_level_adj[py.misc.class_id][PlayerClassLevelAdj::BTH] * py.misc.level);
    int xbthb = py.misc.bth_with_bows + py.misc.plusses_to_hit * BTH_PER_PLUS_TO_HIT_ADJUST + (class_level_adj[py.misc.class_id][PlayerClassLevelAdj::BTHB] * py.misc.level);

    // this results in a range from 0 to 29
    int xfos = 40 - py.misc.fos;
    if (xfos < 0) {
        xfos = 0;
    }

    int xsrh = py.misc.chance_in_search;

    // this results in a range from 0 to 9
    int xstl = py.misc.stealth_factor + 1;
    int xdis = py.misc.disarm + 2 * playerDisarmAdjustment() + playerStatAdjustmentWisdomIntelligence(PlayerAttr::A_INT) +
               (class_level_adj[py.misc.class_id][PlayerClassLevelAdj::DISARM] * py.misc.level / 3);
    int xsave =
        py.misc.saving_throw + playerStatAdjustmentWisdomIntelligence(PlayerAttr::A_WIS) + (class_level_adj[py.misc.class_id][PlayerClassLevelAdj::SAVE] * py.misc.level / 3);
    int xdev =
        py.misc.saving_throw + playerStatAdjustmentWisdomIntelligence(PlayerAttr::A_INT) + (class_level_adj[py.misc.class_id][PlayerClassLevelAdj::DEVICE] * py.misc.level / 3);

    vtype_t xinfra = {'\0'};
    (void) snprintf(xinfra, MORIA_MESSAGE_SIZE, _("%d feet"), py.flags.see_infra * 10);

    putString(_("(Miscellaneous Abilities)"), Coord_t{15, 25});

    // Each ability label's value is placed one column after the label's
    // own display width (not a hardcoded column), so a translated label
    // of a different display width than the English original never has
    // its value overlapping or clobbering part of the label. The value
    // itself is then truncated to fit the remaining budget before the
    // *next* column group's fixed start column, so a wide translated
    // rating word (e.g. statRating()'s "Very Good"/"Excellent") can never
    // extend far enough to be clobbered when that next group's label is
    // drawn afterward at its own fixed column.
    const int col1 = 1, col2 = 28, col3 = 55;

    const char *fighting_label = _("Fighting    :");
    int fighting_value_col = col1 + displayWidth(fighting_label) + 1;
    putString(fighting_label, Coord_t{16, col1});
    putString(truncateToWidth(statRating(Coord_t{12, xbth}), col2 - fighting_value_col).c_str(), Coord_t{16, fighting_value_col});

    const char *bows_throw_label = _("Bows/Throw  :");
    int bows_throw_value_col = col1 + displayWidth(bows_throw_label) + 1;
    putString(bows_throw_label, Coord_t{17, col1});
    putString(truncateToWidth(statRating(Coord_t{12, xbthb}), col2 - bows_throw_value_col).c_str(), Coord_t{17, bows_throw_value_col});

    const char *saving_throw_label = _("Saving Throw:");
    int saving_throw_value_col = col1 + displayWidth(saving_throw_label) + 1;
    putString(saving_throw_label, Coord_t{18, col1});
    putString(truncateToWidth(statRating(Coord_t{6, xsave}), col2 - saving_throw_value_col).c_str(), Coord_t{18, saving_throw_value_col});

    const char *stealth_label = _("Stealth     :");
    int stealth_value_col = col2 + displayWidth(stealth_label) + 1;
    putString(stealth_label, Coord_t{16, col2});
    putString(truncateToWidth(statRating(Coord_t{1, xstl}), col3 - stealth_value_col).c_str(), Coord_t{16, stealth_value_col});

    const char *disarming_label = _("Disarming   :");
    int disarming_value_col = col2 + displayWidth(disarming_label) + 1;
    putString(disarming_label, Coord_t{17, col2});
    putString(truncateToWidth(statRating(Coord_t{8, xdis}), col3 - disarming_value_col).c_str(), Coord_t{17, disarming_value_col});

    const char *magic_device_label = _("Magic Device:");
    int magic_device_value_col = col2 + displayWidth(magic_device_label) + 1;
    putString(magic_device_label, Coord_t{18, col2});
    putString(truncateToWidth(statRating(Coord_t{6, xdev}), col3 - magic_device_value_col).c_str(), Coord_t{18, magic_device_value_col});

    // Group 3 has no further column group to its right (just the 79/80
    // column screen edge, which putString() already truncates against
    // safely), so its values don't need a next-column budget clamp.
    const char *perception_label = _("Perception  :");
    putString(perception_label, Coord_t{16, col3});
    putString(statRating(Coord_t{3, xfos}), Coord_t{16, col3 + displayWidth(perception_label) + 1});
    const char *searching_label = _("Searching   :");
    putString(searching_label, Coord_t{17, col3});
    putString(statRating(Coord_t{6, xsrh}), Coord_t{17, col3 + displayWidth(searching_label) + 1});
    const char *infra_vision_label = _("Infra-Vision:");
    putString(infra_vision_label, Coord_t{18, col3});
    putString(xinfra, Coord_t{18, col3 + displayWidth(infra_vision_label) + 1});
}

// Used to display the character on the screen. -RAK-
void printCharacter() {
    printCharacterInformation();
    printCharacterVitalStatistics();
    printCharacterStats();
    printCharacterLevelExperience();
    printCharacterAbilities();
}

// Gets a name for the character -JWT-
void getCharacterName() {
    putStringClearToEOL(_("Enter your player's name  [press <RETURN> when finished]"), Coord_t{21, 2});

    // Same fixed CHARACTER_SHEET_LABEL_WIDTH-based column as
    // printCharacterInformation() above uses for this same label, so a
    // translated (e.g. Japanese) label wider than the English original is
    // truncated to fit rather than pushing the name-entry field (and the
    // fixed-column Age/Height/Weight block just to its right) out of
    // alignment.
    const int name_column = CHARACTER_SHEET_LABEL_WIDTH + 2;

    putString(padToDisplayWidth("", 23).c_str(), Coord_t{2, name_column});

    if (!getStringInput(py.misc.name, Coord_t{2, name_column}, 23) || py.misc.name[0] == 0) {
        getDefaultPlayerName(py.misc.name);
        putString(py.misc.name, Coord_t{2, name_column});
    }

    clearToBottom(20);
}

// Changes the name of the character -JWT-
void changeCharacterName() {
    vtype_t temp = {'\0'};
    bool flag = false;

    printCharacter();

    while (!flag) {
        putStringClearToEOL(_("<f>ile character description. <c>hange character name."), Coord_t{21, 2});

        switch (getKeyInput()) {
            case 'c':
                getCharacterName();
                flag = true;
                break;
            case 'f': {
                const char *file_name_prompt = _("File name:");
                putStringClearToEOL(file_name_prompt, Coord_t{0, 0});

                if (getStringInput(temp, Coord_t{0, displayWidth(file_name_prompt)}, 60) && (temp[0] != 0)) {
                    if (outputPlayerCharacterToFile(temp)) {
                        flag = true;
                    }
                }
                break;
            }
            case ESCAPE:
            case ' ':
            case '\n':
            case '\r':
                flag = true;
                break;
            default:
                terminalBellSound();
                break;
        }
    }
}

// Print list of spells -RAK-
// if non_consecutive is  -1: spells numbered consecutively from 'a' to 'a'+num
//                       >=0: spells numbered by offset from non_consecutive
void displaySpellsList(const int *spell_ids, int number_of_choices, bool comment, int non_consecutive) {
    int col;
    if (comment) {
        col = 22;
    } else {
        col = 31;
    }

    int consecutive_offset;
    if (classes[py.misc.class_id].class_to_use_mage_spells == config::spells::SPELL_TYPE_MAGE) {
        consecutive_offset = config::spells::NAME_OFFSET_SPELLS;
    } else {
        consecutive_offset = config::spells::NAME_OFFSET_PRAYERS;
    }

    eraseLine(Coord_t{1, col});
    putString(_("Name"), Coord_t{1, col + 5});
    putString(_("Lv Mana Fail"), Coord_t{1, col + 35});

    // only show the first 22 choices
    if (number_of_choices > 22) {
        number_of_choices = 22;
    }

    for (int i = 0; i < number_of_choices; i++) {
        int spell_id = spell_ids[i];
        Spell_t const &spell = magic_spells[py.misc.class_id - 1][spell_id];

        const char *p = nullptr;
        if (!comment) {
            p = "";
        } else if ((py.flags.spells_forgotten & (1L << spell_id)) != 0) {
            p = _(" forgotten");
        } else if ((py.flags.spells_learnt & (1L << spell_id)) == 0) {
            p = _(" unknown");
        } else if ((py.flags.spells_worked & (1L << spell_id)) == 0) {
            p = _(" untried");
        } else {
            p = "";
        }

        // determine whether or not to leave holes in character choices, non_consecutive -1
        // when learning spells, consecutive_offset>=0 when asking which spell to cast.
        char spell_char;
        if (non_consecutive == -1) {
            spell_char = (char) ('a' + i);
        } else {
            spell_char = (char) ('a' + spell_id - non_consecutive);
        }

        // Padded to a fixed *display-column* width (not the byte-width
        // %-30s printf specifier), so a translated (e.g. Japanese) spell
        // name wider than the English original still leaves the Lv/Mana/
        // Fail columns that follow at a fixed, non-overlapping position.
        std::string spell_name = padToDisplayWidth(_(spell_names[spell_id + consecutive_offset]), 30);

        vtype_t out_val = {'\0'};
        (void) snprintf(out_val, MORIA_MESSAGE_SIZE, "  %c) %s%2d %4d %3d%%%s", spell_char, spell_name.c_str(), spell.level_required, spell.mana_required,
                        spellChanceOfSuccess(spell_id), p);
        putStringClearToEOL(out_val, Coord_t{2 + i, col});
    }
}

// Increases hit points and level -RAK-
static void playerGainLevel() {
    py.misc.level++;

    vtype_t msg = {'\0'};
    (void) snprintf(msg, MORIA_MESSAGE_SIZE, _("Welcome to level %d."), (int) py.misc.level);
    printMessage(msg);

    playerCalculateHitPoints();

    int32_t new_exp = py.base_exp_levels[py.misc.level - 1] * py.misc.experience_factor / 100;

    if (py.misc.exp > new_exp) {
        // lose some of the 'extra' exp when gaining several levels at once
        int32_t dif_exp = py.misc.exp - new_exp;
        py.misc.exp = new_exp + (dif_exp / 2);
    }

    printCharacterLevel();
    printCharacterTitle();

    Class_t const &player_class = classes[py.misc.class_id];

    if (player_class.class_to_use_mage_spells == config::spells::SPELL_TYPE_MAGE) {
        playerCalculateAllowedSpellsCount(PlayerAttr::A_INT);
        playerGainMana(PlayerAttr::A_INT);
    } else if (player_class.class_to_use_mage_spells == config::spells::SPELL_TYPE_PRIEST) {
        playerCalculateAllowedSpellsCount(PlayerAttr::A_WIS);
        playerGainMana(PlayerAttr::A_WIS);
    }
}

// Prints experience -RAK-
void displayCharacterExperience() {
    if (py.misc.exp > config::player::PLAYER_MAX_EXP) {
        py.misc.exp = config::player::PLAYER_MAX_EXP;
    }

    while ((py.misc.level < PLAYER_MAX_LEVEL) && (signed) (py.base_exp_levels[py.misc.level - 1] * py.misc.experience_factor / 100) <= py.misc.exp) {
        playerGainLevel();
    }

    if (py.misc.exp > py.misc.max_exp) {
        py.misc.max_exp = py.misc.exp;
    }

    printLongNumber(py.misc.exp, Coord_t{14, STAT_COLUMN + 6});
}
