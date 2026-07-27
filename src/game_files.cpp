// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Misc code to access files used by Moria

// NOTE: <fstream> must be included before "headers.h": ui.h #defines
// `open` to `topen` (for the setuid tilde-expansion wrapper), and that
// macro corrupts std::basic_filebuf::open() if <fstream> is parsed
// after the #define is active.
#include <fstream>

#include "headers.h"

// This must be included after fcntl.h, which has a prototype for `open' on some
// systems.  Otherwise, the `open' prototype conflicts with the `topen' declaration.

// If a localized version of `default_path` exists at
// "data/lang/<current language>/<basename of default_path>", returns that
// path instead. Otherwise (no active non-English language, or no such file)
// returns `default_path` unchanged.
static std::string resolveLocalizedFilePath(const std::string &default_path) {
    const std::string &language = config::language::current;
    if (language.empty() || language == "en") {
        return default_path;
    }

    size_t slash = default_path.find_last_of('/');
    std::string base_name = (slash == std::string::npos) ? default_path : default_path.substr(slash + 1);

    std::string candidate = "data/lang/" + language + "/" + base_name;

    std::ifstream probe(candidate);
    if (probe.good()) {
        return candidate;
    }

    return default_path;
}

//  initializeScoreFile
//  Open the score file while we still have the setuid privileges.  Later
//  when the score is being written out, you must be sure to flock the file
//  so we don't have multiple people trying to write to it at the same time.
//  Craig Norborg (doc)    Mon Aug 10 16:41:59 EST 1987
bool initializeScoreFile() {
    highscore_fp = fopen(config::files::scores.c_str(), (char *) "rb+");

    return highscore_fp != nullptr;
}

// Attempt to open and print the file containing the intro splash screen text -RAK-
void displaySplashScreen() {
    constexpr int max_line_length = 256;
    char in_line[max_line_length] = {'\0'};

    FILE *screen_file = fopen(resolveLocalizedFilePath(config::files::splash_screen).c_str(), "r");
    if (screen_file != nullptr) {
        clearScreen();
        for (int i = 0; fgets(in_line, max_line_length, screen_file) != CNIL; i++) {
            putString(in_line, Coord_t{i, 0});
        }
        waitForContinueKey(23);

        (void) fclose(screen_file);
    }
}

// Open and display a text help file
// File perusal, primitive, but portable -CJS-
void displayTextHelpFile(const std::string &filename) {
    FILE *file = fopen(resolveLocalizedFilePath(filename).c_str(), "r");
    if (file == nullptr) {
        putStringClearToEOL(_("Can not find help file '") + filename + _("'."), Coord_t{0, 0});
        return;
    }

    terminalSaveScreen();

    constexpr int max_line_length = 256;
    char line_buffer[max_line_length];

    while (feof(file) == 0) {
        clearScreen();

        for (int i = 0; i < 23; i++) {
            if (fgets(line_buffer, max_line_length - 1, file) != CNIL) {
                putString(line_buffer, Coord_t{i, 0});
            }
        }

        putStringClearToEOL(_("[ press any key to continue ]"), Coord_t{23, 23});
        if (getKeyInput() == ESCAPE) {
            break;
        }
    }

    (void) fclose(file);

    terminalRestoreScreen();
}

// Open and display a "death" text file
void displayDeathFile(const std::string &filename) {
    FILE *file = fopen(resolveLocalizedFilePath(filename).c_str(), "r");
    if (file == nullptr) {
        putStringClearToEOL(_("Can not find help file '") + filename + _("'."), Coord_t{0, 0});
        return;
    }

    clearScreen();

    constexpr int max_line_length = 256;
    char line_buffer[max_line_length];

    for (int i = 0; i < 23 && feof(file) == 0; i++) {
        if (fgets(line_buffer, max_line_length - 1, file) != CNIL) {
            putString(line_buffer, Coord_t{i, 0});
        }
    }
    (void) fclose(file);
}

// Prints a list of random objects to a file. -RAK-
// Note that the objects produced is a sampling of objects
// which be expected to appear on that level.
void outputRandomLevelObjectsToFile() {
    obj_desc_t input = {0};

    const char *level_prompt = _("Produce objects on what level?: ");
    putStringClearToEOL(level_prompt, Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, displayWidth(level_prompt)}, 10)) {
        return;
    }

    int level;
    if (!stringToNumber(input, level)) {
        return;
    }

    const char *count_prompt = _("Produce how many objects?: ");
    putStringClearToEOL(count_prompt, Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, displayWidth(count_prompt)}, 10)) {
        return;
    }

    int count;
    if (!stringToNumber(input, count)) {
        return;
    }

    if (count < 1 || level < 0 || level > 1200) {
        putStringClearToEOL(_("Parameters no good."), Coord_t{0, 0});
        return;
    }

    if (count > 10000) {
        count = 10000;
    }

    bool small_objects = getInputConfirmation(_("Small objects only?"));

    const char *file_name_prompt = _("File name: ");
    putStringClearToEOL(file_name_prompt, Coord_t{0, 0});

    vtype_t filename = {0};

    if (!getStringInput(filename, Coord_t{0, displayWidth(file_name_prompt)}, 64)) {
        return;
    }
    if (strlen(filename) == 0) {
        return;
    }

    FILE *file_ptr = fopen(filename, "w");
    if (file_ptr == nullptr) {
        putStringClearToEOL(_("File could not be opened."), Coord_t{0, 0});
        return;
    }

    (void) snprintf(input, MORIA_OBJ_DESC_SIZE, "%d", count);
    putStringClearToEOL(strcat(input, _(" random objects being produced...")), Coord_t{0, 0});

    putQIO();

    (void) fprintf(file_ptr, "%s", _("*** Random Object Sampling:\n"));
    (void) fprintf(file_ptr, _("*** %d objects\n"), count);
    (void) fprintf(file_ptr, _("*** For Level %d\n"), level);
    (void) fprintf(file_ptr, "\n");
    (void) fprintf(file_ptr, "\n");

    int treasure_id = popt();

    for (int i = 0; i < count; i++) {
        int object_id = itemGetRandomObjectId(level, small_objects);
        inventoryItemCopyTo(sorted_objects[object_id], game.treasure.list[treasure_id]);

        magicTreasureMagicalAbility(treasure_id, level);

        Inventory_t &item = game.treasure.list[treasure_id];
        itemIdentifyAsStoreBought(item);

        if (inventoryItemIsCursed(item)) {
            itemAppendToInscription(item, config::identification::ID_DAMD);
        }

        itemDescription(input, item, true);
        (void) fprintf(file_ptr, "%d %s\n", item.depth_first_found, input);
    }

    pusht((uint8_t) treasure_id);

    (void) fclose(file_ptr);

    putStringClearToEOL(_("Completed."), Coord_t{0, 0});
}

// Builds a fixed-display-width "<label>:" cell (matching the
// CHARACTER_SHEET_LABEL_WIDTH-based labels already used on the on-screen
// character sheet, ui.cpp), followed by a display-width-padded value.
// Unlike the original `"%9s %-23s"`-style printf fields (byte-width, so
// a translated label or value of different display width than English
// would shift every subsequent field on the same physical line), this
// is safe for translated text: padToDisplayWidth() always returns
// exactly the requested column count, truncating if necessary.
static std::string characterSheetRow38(const char *label_with_colon, const std::string &value) {
    return " " + padToDisplayWidth(label_with_colon, CHARACTER_SHEET_LABEL_WIDTH) + " " + padToDisplayWidth(value, 23);
}

// Same idea as characterSheetRow38(), for the second (numeric-valued)
// column of the character sheet's top block. The value is always plain
// ASCII digits, so it's formatted with ordinary %6d (no display-width
// concern there); only the label needs display-width-safe padding.
static std::string characterSheetRow22(const char *label, int32_t value) {
    vtype_t number = {'\0'};
    (void) snprintf(number, MORIA_MESSAGE_SIZE, "%6d", value);
    return padToDisplayWidth(std::string(" ") + label, 14) + ":" + " " + number;
}

// For the "+ To Hit / + To Damage / + To AC / Total AC" column of the
// abilities block: same idea, but the label (with its own leading
// space already baked in) is padded to a 13-column budget and the
// value is %6d.
static std::string characterSheetRowPlus(const char *label, int32_t value) {
    vtype_t number = {'\0'};
    (void) snprintf(number, MORIA_MESSAGE_SIZE, "%6d", value);
    return padToDisplayWidth(std::string(" ") + label, 13) + ":" + " " + number;
}

// For the "Level / Experience / Max Exp / Exp to Adv" column: a fixed
// 7-column blank gutter (plain ASCII, always safe), then the label
// padded to an 11-column budget, then a %7d value.
static std::string characterSheetRowLevel(const char *label, int32_t value) {
    vtype_t number = {'\0'};
    (void) snprintf(number, MORIA_MESSAGE_SIZE, "%7d", value);
    return "       " + padToDisplayWidth(label, 11) + ":" + " " + number;
}

// Write character sheet to the file
static void writeCharacterSheetToFile(FILE *char_file) {
    putStringClearToEOL(_("Writing character sheet..."), Coord_t{0, 0});
    putQIO();

    const char *blank = " ";

    vtype_t stat_description = {'\0'};

    (void) fprintf(char_file, "%c\n\n", CTRL_KEY('L'));

    (void) fprintf(char_file, "%s", characterSheetRow38(_("Name        :"), py.misc.name).c_str());
    (void) fprintf(char_file, "%s", characterSheetRow22(_("Age          "), (int32_t) py.misc.age).c_str());
    statsAsString(py.stats.used[PlayerAttr::A_STR], stat_description);
    (void) fprintf(char_file, "   %s%s\n", _("STR : "), stat_description);
    (void) fprintf(char_file, "%s", characterSheetRow38(_("Race        :"), _(character_races[py.misc.race_id].name)).c_str());
    (void) fprintf(char_file, "%s", characterSheetRow22(_("Height       "), (int32_t) py.misc.height).c_str());
    statsAsString(py.stats.used[PlayerAttr::A_INT], stat_description);
    (void) fprintf(char_file, "   %s%s\n", _("INT : "), stat_description);
    (void) fprintf(char_file, "%s", characterSheetRow38(_("Sex         :"), playerGetGenderLabel()).c_str());
    (void) fprintf(char_file, "%s", characterSheetRow22(_("Weight       "), (int32_t) py.misc.weight).c_str());
    statsAsString(py.stats.used[PlayerAttr::A_WIS], stat_description);
    (void) fprintf(char_file, "   %s%s\n", _("WIS : "), stat_description);
    (void) fprintf(char_file, "%s", characterSheetRow38(_("Class       :"), _(classes[py.misc.class_id].title)).c_str());
    (void) fprintf(char_file, "%s", characterSheetRow22(_("Social Class "), py.misc.social_class).c_str());
    statsAsString(py.stats.used[PlayerAttr::A_DEX], stat_description);
    (void) fprintf(char_file, "   %s%s\n", _("DEX : "), stat_description);
    (void) fprintf(char_file, "%s", characterSheetRow38(_("Title       :"), playerRankTitle()).c_str());
    (void) fprintf(char_file, "%22s", "");
    statsAsString(py.stats.used[PlayerAttr::A_CON], stat_description);
    (void) fprintf(char_file, "   %s%s\n", _("CON : "), stat_description);
    (void) fprintf(char_file, "%34s", blank);
    (void) fprintf(char_file, "%26s", blank);
    statsAsString(py.stats.used[PlayerAttr::A_CHR], stat_description);
    (void) fprintf(char_file, "   %s%s\n\n", _("CHR : "), stat_description);

    (void) fprintf(char_file, "%s", characterSheetRowPlus(_("+ To Hit    "), py.misc.display_to_hit).c_str());
    (void) fprintf(char_file, "%s", characterSheetRowLevel(_("Level      "), (int32_t) py.misc.level).c_str());
    (void) fprintf(char_file, _("    Max Hit Points : %6d\n"), py.misc.max_hp);
    (void) fprintf(char_file, "%s", characterSheetRowPlus(_("+ To Damage "), py.misc.display_to_damage).c_str());
    (void) fprintf(char_file, "%s", characterSheetRowLevel(_("Experience "), py.misc.exp).c_str());
    (void) fprintf(char_file, _("    Cur Hit Points : %6d\n"), py.misc.current_hp);
    (void) fprintf(char_file, "%s", characterSheetRowPlus(_("+ To AC     "), py.misc.display_to_ac).c_str());
    (void) fprintf(char_file, "%s", characterSheetRowLevel(_("Max Exp    "), py.misc.max_exp).c_str());
    (void) fprintf(char_file, _("    Max Mana       : %6d\n"), py.misc.mana);
    (void) fprintf(char_file, "%s", characterSheetRowPlus(_("  Total AC  "), py.misc.display_ac).c_str());
    if (py.misc.level >= PLAYER_MAX_LEVEL) {
        (void) fprintf(char_file, "       %s: %s", padToDisplayWidth(_("Exp to Adv "), 11).c_str(), "*******");
    } else {
        (void) fprintf(char_file, "%s", characterSheetRowLevel(_("Exp to Adv "), (int32_t) (py.base_exp_levels[py.misc.level - 1] * py.misc.experience_factor / 100)).c_str());
    }
    (void) fprintf(char_file, _("    Cur Mana       : %6d\n"), py.misc.current_mana);
    (void) fprintf(char_file, "%28s%s: %7d\n\n", blank, padToDisplayWidth(_("Gold"), 11).c_str(), py.misc.au);

    int xbth = py.misc.bth + py.misc.plusses_to_hit * BTH_PER_PLUS_TO_HIT_ADJUST + //
               (class_level_adj[py.misc.class_id][PlayerClassLevelAdj::BTH] * py.misc.level);
    int xbthb = py.misc.bth_with_bows + py.misc.plusses_to_hit * BTH_PER_PLUS_TO_HIT_ADJUST + //
                (class_level_adj[py.misc.class_id][PlayerClassLevelAdj::BTHB] * py.misc.level);

    // this results in a range from 0 to 29
    int xfos = 40 - py.misc.fos;
    if (xfos < 0) {
        xfos = 0;
    }
    int xsrh = py.misc.chance_in_search;

    // this results in a range from 0 to 9
    int xstl = py.misc.stealth_factor + 1;
    int xdis = py.misc.disarm + 2 * playerDisarmAdjustment() + playerStatAdjustmentWisdomIntelligence(PlayerAttr::A_INT) + //
               (class_level_adj[py.misc.class_id][PlayerClassLevelAdj::DISARM] * py.misc.level / 3);
    int xsave = py.misc.saving_throw + playerStatAdjustmentWisdomIntelligence(PlayerAttr::A_WIS) + //
                (class_level_adj[py.misc.class_id][PlayerClassLevelAdj::SAVE] * py.misc.level / 3);
    int xdev = py.misc.saving_throw + playerStatAdjustmentWisdomIntelligence(PlayerAttr::A_INT) + //
               (class_level_adj[py.misc.class_id][PlayerClassLevelAdj::DEVICE] * py.misc.level / 3);

    vtype_t xinfra = {'\0'};
    (void) snprintf(xinfra, MORIA_MESSAGE_SIZE, _("%d feet"), py.flags.see_infra * 10);

    // Same technique as characterSheetRow38()/Row22() above: each ability
    // label reuses the exact translated "<Label>:" unit already used on
    // the on-screen character sheet (ui.cpp's printCharacterAbilities()),
    // and both the label and the statRating() value (also translated,
    // e.g. "Very Good"/"Excellent") are padded to a fixed display width
    // rather than the original's byte-width `%-10s`, so neither can
    // overflow into the next column regardless of translated length.
    (void) fprintf(char_file, "%s\n\n", _("(Miscellaneous Abilities)"));
    (void) fprintf(char_file, "%s", (" " + padToDisplayWidth(_("Fighting    :"), 13) + " " + padToDisplayWidth(statRating(Coord_t{12, xbth}), 10)).c_str());
    (void) fprintf(char_file, "%s", ("   " + padToDisplayWidth(_("Stealth     :"), 13) + " " + padToDisplayWidth(statRating(Coord_t{1, xstl}), 10)).c_str());
    (void) fprintf(char_file, "%s\n", ("   " + padToDisplayWidth(_("Perception  :"), 13) + " " + statRating(Coord_t{3, xfos})).c_str());
    (void) fprintf(char_file, "%s", (" " + padToDisplayWidth(_("Bows/Throw  :"), 13) + " " + padToDisplayWidth(statRating(Coord_t{12, xbthb}), 10)).c_str());
    (void) fprintf(char_file, "%s", ("   " + padToDisplayWidth(_("Disarming   :"), 13) + " " + padToDisplayWidth(statRating(Coord_t{8, xdis}), 10)).c_str());
    (void) fprintf(char_file, "%s\n", ("   " + padToDisplayWidth(_("Searching   :"), 13) + " " + statRating(Coord_t{6, xsrh})).c_str());
    (void) fprintf(char_file, "%s", (" " + padToDisplayWidth(_("Saving Throw:"), 13) + " " + padToDisplayWidth(statRating(Coord_t{6, xsave}), 10)).c_str());
    (void) fprintf(char_file, "%s", ("   " + padToDisplayWidth(_("Magic Device:"), 13) + " " + padToDisplayWidth(statRating(Coord_t{6, xdev}), 10)).c_str());
    (void) fprintf(char_file, "%s\n\n", ("   " + padToDisplayWidth(_("Infra-Vision:"), 13) + " " + std::string(xinfra)).c_str());

    // Write out the character's history
    (void) fprintf(char_file, "%s\n", _("Character Background"));
    for (auto &entry : py.misc.history) {
        (void) fprintf(char_file, " %s\n", entry);
    }
}

static const char *equipmentPlacementDescription(int item_id) {
    switch (item_id) {
        case PlayerEquipment::Wield:
            return _("You are wielding");
        case PlayerEquipment::Head:
            return _("Worn on head");
        case PlayerEquipment::Neck:
            return _("Worn around neck");
        case PlayerEquipment::Body:
            return _("Worn on body");
        case PlayerEquipment::Arm:
            return _("Worn on shield arm");
        case PlayerEquipment::Hands:
            return _("Worn on hands");
        case PlayerEquipment::Right:
            return _("Right ring finger");
        case PlayerEquipment::Left:
            return _("Left  ring finger");
        case PlayerEquipment::Feet:
            return _("Worn on feet");
        case PlayerEquipment::Outer:
            return _("Worn about body");
        case PlayerEquipment::Light:
            return _("Light source is");
        case PlayerEquipment::Auxiliary:
            return _("Secondary weapon");
        default:
            return _("*Unknown value*");
    }
}

// Write out the equipment list.
static void writeEquipmentListToFile(FILE *equip_file) {
    (void) fprintf(equip_file, "%s", _("\n  [Character's Equipment List]\n\n"));

    if (py.equipment_count == 0) {
        (void) fprintf(equip_file, "%s", _("  Character has no equipment in use.\n"));
        return;
    }

    obj_desc_t description = {'\0'};
    int item_slot_id = 0;

    for (int i = PlayerEquipment::Wield; i < PLAYER_INVENTORY_SIZE; i++) {
        if (py.inventory[i].category_id == TV_NOTHING) {
            continue;
        }

        itemDescription(description, py.inventory[i], true);
        // padToDisplayWidth (not the original byte-width %-19s) keeps the
        // item description's start column stable even when a translated
        // placement description (e.g. "You are wielding") has a
        // different display width than its English original.
        std::string placement = padToDisplayWidth(equipmentPlacementDescription(i), 19);
        (void) fprintf(equip_file, _("  %c) %s: %s\n"), item_slot_id + 'a', placement.c_str(), description);

        item_slot_id++;
    }

    (void) fprintf(equip_file, "%c\n\n", CTRL_KEY('L'));
}

// Write out the character's inventory.
static void writeInventoryToFile(FILE *inv_file) {
    (void) fprintf(inv_file, "%s", _("  [General Inventory List]\n\n"));

    if (py.pack.unique_items == 0) {
        (void) fprintf(inv_file, "%s", _("  Character has no objects in inventory.\n"));
        return;
    }

    obj_desc_t description = {'\0'};

    for (int i = 0; i < py.pack.unique_items; i++) {
        itemDescription(description, py.inventory[i], true);
        (void) fprintf(inv_file, _("%c) %s\n"), i + 'a', description);
    }

    (void) fprintf(inv_file, "%c", CTRL_KEY('L'));
}

// Print the character to a file or device -RAK-
bool outputPlayerCharacterToFile(char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0 && errno == EEXIST) {
        if (getInputConfirmation(_("Replace existing file ") + std::string(filename) + "?")) {
            fd = open(filename, O_WRONLY, 0644);
        }
    }

    FILE *file;
    if (fd >= 0) {
        // on some non-unix machines, fdopen() is not reliable,
        // hence must call close() and then fopen().
        (void) close(fd);
        file = fopen(filename, "w");
    } else {
        file = nullptr;
    }

    if (file == nullptr) {
        if (fd >= 0) {
            (void) close(fd);
        }
        vtype_t msg = {'\0'};
        (void) snprintf(msg, MORIA_MESSAGE_SIZE, _("Can't open file %s:"), filename);
        printMessage(msg);
        return false;
    }

    writeCharacterSheetToFile(file);
    writeEquipmentListToFile(file);
    writeInventoryToFile(file);

    (void) fclose(file);

    putStringClearToEOL(_("Completed."), Coord_t{0, 0});

    return true;
}
