// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Version history and info, and wizard mode debugging aids.

#include "headers.h"

#include <sstream>

// lets anyone enter wizard mode after a disclaimer... -JEW-
bool enterWizardMode() {
    bool answer = false;

    if (game.noscore == 0) {
        printMessage(_("Wizard mode is for debugging and experimenting."));
        answer = getInputConfirmation(_("The game will not be scored if you enter wizard mode. Are you sure?"));
    }

    if ((game.noscore != 0) || answer) {
        game.noscore |= 0x2;
        game.wizard_mode = true;
        return true;
    }

    return false;
}

void wizardCureAll() {
    (void) spellRemoveCurseFromAllWornItems();
    (void) playerCureBlindness();
    (void) playerCureConfusion();
    (void) playerCurePoison();
    (void) playerRemoveFear();
    (void) playerStatRestore(PlayerAttr::A_STR);
    (void) playerStatRestore(PlayerAttr::A_INT);
    (void) playerStatRestore(PlayerAttr::A_WIS);
    (void) playerStatRestore(PlayerAttr::A_CON);
    (void) playerStatRestore(PlayerAttr::A_DEX);
    (void) playerStatRestore(PlayerAttr::A_CHR);

    if (py.flags.slow > 1) {
        py.flags.slow = 1;
    }
    if (py.flags.image > 1) {
        py.flags.image = 1;
    }
}

// Generate random items
void wizardDropRandomItems() {
    int i;

    if (game.command_count > 0) {
        i = game.command_count;
        game.command_count = 0;
    } else {
        i = 1;
    }
    dungeonPlaceRandomObjectNear(py.pos, i);

    drawDungeonPanel();
}

// Go up/down to specified depth
void wizardJumpLevel() {
    int i;

    if (game.command_count > 0) {
        if (game.command_count > 99) {
            i = 0;
        } else {
            i = game.command_count;
        }
        game.command_count = 0;
    } else {
        i = -1;
        vtype_t input = {0};

        putStringClearToEOL(_("Go to which level (0-99) ? "), Coord_t{0, 0});

        if (getStringInput(input, Coord_t{0, displayWidth(_("Go to which level (0-99) ? "))}, 10)) {
            (void) stringToNumber(input, i);
        }
    }

    if (i >= 0) {
        dg.current_level = (int16_t) i;
        if (dg.current_level > 99) {
            dg.current_level = 99;
        }
        dg.generate_new_level = true;
    } else {
        messageLineClear();
    }
}

// Increase Experience
void wizardGainExperience() {
    if (game.command_count > 0) {
        py.misc.exp = game.command_count;
        game.command_count = 0;
    } else if (py.misc.exp == 0) {
        py.misc.exp = 1;
    } else {
        py.misc.exp = py.misc.exp * 2;
    }
    displayCharacterExperience();
}

// Summon a random monster
void wizardSummonMonster() {
    Coord_t coord = Coord_t{py.pos.y, py.pos.x};

    (void) monsterSummon(coord, true);

    updateMonsters(false);
}

// Light up the dungeon -RAK-
void wizardLightUpDungeon() {
    bool flag;

    flag = !dg.floor[py.pos.y][py.pos.x].permanent_light;

    for (int y = 0; y < dg.height; y++) {
        for (int x = 0; x < dg.width; x++) {
            if (dg.floor[y][x].feature_id <= MAX_CAVE_FLOOR) {
                for (int yy = y - 1; yy <= y + 1; yy++) {
                    for (int xx = x - 1; xx <= x + 1; xx++) {
                        dg.floor[yy][xx].permanent_light = flag;
                        if (!flag) {
                            dg.floor[yy][xx].field_mark = false;
                        }
                    }
                }
            }
        }
    }

    drawDungeonPanel();
}

// Wizard routine for gaining on stats -RAK-
void wizardCharacterAdjustment() {
    int number;
    vtype_t input = {'\0'};

    putStringClearToEOL(_("(3 - 118) Strength     = "), Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, displayWidth(_("(3 - 118) Strength     = "))}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > 2 && number < 119) {
            py.stats.max[PlayerAttr::A_STR] = (uint8_t) number;
            (void) playerStatRestore(PlayerAttr::A_STR);
        }
    } else {
        return;
    }

    putStringClearToEOL(_("(3 - 118) Intelligence = "), Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, displayWidth(_("(3 - 118) Intelligence = "))}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > 2 && number < 119) {
            py.stats.max[PlayerAttr::A_INT] = (uint8_t) number;
            (void) playerStatRestore(PlayerAttr::A_INT);
        }
    } else {
        return;
    }

    putStringClearToEOL(_("(3 - 118) Wisdom       = "), Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, displayWidth(_("(3 - 118) Wisdom       = "))}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > 2 && number < 119) {
            py.stats.max[PlayerAttr::A_WIS] = (uint8_t) number;
            (void) playerStatRestore(PlayerAttr::A_WIS);
        }
    } else {
        return;
    }

    putStringClearToEOL(_("(3 - 118) Dexterity    = "), Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, displayWidth(_("(3 - 118) Dexterity    = "))}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > 2 && number < 119) {
            py.stats.max[PlayerAttr::A_DEX] = (uint8_t) number;
            (void) playerStatRestore(PlayerAttr::A_DEX);
        }
    } else {
        return;
    }

    putStringClearToEOL(_("(3 - 118) Constitution = "), Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, displayWidth(_("(3 - 118) Constitution = "))}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > 2 && number < 119) {
            py.stats.max[PlayerAttr::A_CON] = (uint8_t) number;
            (void) playerStatRestore(PlayerAttr::A_CON);
        }
    } else {
        return;
    }

    putStringClearToEOL(_("(3 - 118) Charisma     = "), Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, displayWidth(_("(3 - 118) Charisma     = "))}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > 2 && number < 119) {
            py.stats.max[PlayerAttr::A_CHR] = (uint8_t) number;
            (void) playerStatRestore(PlayerAttr::A_CHR);
        }
    } else {
        return;
    }

    putStringClearToEOL(_("(1 - 32767) Hit points = "), Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, displayWidth(_("(1 - 32767) Hit points = "))}, 5)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > 0 && number <= SHRT_MAX) {
            py.misc.max_hp = (int16_t) number;
            py.misc.current_hp = (int16_t) number;
            py.misc.current_hp_fraction = 0;
            printCharacterMaxHitPoints();
            printCharacterCurrentHitPoints();
        }
    } else {
        return;
    }

    putStringClearToEOL(_("(0 - 32767) Mana       = "), Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, displayWidth(_("(0 - 32767) Mana       = "))}, 5)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > -1 && number <= SHRT_MAX) {
            py.misc.mana = (int16_t) number;
            py.misc.current_mana = (int16_t) number;
            py.misc.current_mana_fraction = 0;
            printCharacterCurrentMana();
        }
    } else {
        return;
    }

    (void) snprintf(input, MORIA_MESSAGE_SIZE, _("Current=%d  Gold = "), py.misc.au);
    number = displayWidth(input);
    putStringClearToEOL(input, Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, number}, 7)) {
        int new_gold;
        bool valid_number = stringToNumber(input, new_gold);
        if (valid_number && new_gold > -1) {
            py.misc.au = new_gold;
            printCharacterGoldValue();
        }
    } else {
        return;
    }

    (void) snprintf(input, MORIA_MESSAGE_SIZE, _("Current=%d  (0-200) Searching = "), py.misc.chance_in_search);
    number = displayWidth(input);
    putStringClearToEOL(input, Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, number}, 3)) {
        int new_gold;
        bool valid_number = stringToNumber(input, new_gold);
        if (valid_number && number > -1 && number < 201) {
            py.misc.chance_in_search = (int16_t) number;
        }
    } else {
        return;
    }

    (void) snprintf(input, MORIA_MESSAGE_SIZE, _("Current=%d  (-1-18) Stealth = "), py.misc.stealth_factor);
    number = displayWidth(input);
    putStringClearToEOL(input, Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, number}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > -2 && number < 19) {
            py.misc.stealth_factor = (int16_t) number;
        }
    } else {
        return;
    }

    (void) snprintf(input, MORIA_MESSAGE_SIZE, _("Current=%d  (0-200) Disarming = "), py.misc.disarm);
    number = displayWidth(input);
    putStringClearToEOL(input, Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, number}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > -1 && number < 201) {
            py.misc.disarm = (int16_t) number;
        }
    } else {
        return;
    }

    (void) snprintf(input, MORIA_MESSAGE_SIZE, _("Current=%d  (0-100) Save = "), py.misc.saving_throw);
    number = displayWidth(input);
    putStringClearToEOL(input, Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, number}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > -1 && number < 201) {
            py.misc.saving_throw = (int16_t) number;
        }
    } else {
        return;
    }

    (void) snprintf(input, MORIA_MESSAGE_SIZE, _("Current=%d  (0-200) Base to hit = "), py.misc.bth);
    number = displayWidth(input);
    putStringClearToEOL(input, Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, number}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > -1 && number < 201) {
            py.misc.bth = (int16_t) number;
        }
    } else {
        return;
    }

    (void) snprintf(input, MORIA_MESSAGE_SIZE, _("Current=%d  (0-200) Bows/Throwing = "), py.misc.bth_with_bows);
    number = displayWidth(input);
    putStringClearToEOL(input, Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, number}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > -1 && number < 201) {
            py.misc.bth_with_bows = (int16_t) number;
        }
    } else {
        return;
    }

    (void) snprintf(input, MORIA_MESSAGE_SIZE, _("Current=%d  Weight = "), py.misc.weight);
    number = displayWidth(input);
    putStringClearToEOL(input, Coord_t{0, 0});
    if (getStringInput(input, Coord_t{0, number}, 3)) {
        bool valid_number = stringToNumber(input, number);
        if (valid_number && number > -1) {
            py.misc.weight = (uint16_t) number;
        }
    } else {
        return;
    }

    while (getCommand(_("Alter speed? (+/-)"), *input)) {
        if (*input == '+') {
            playerChangeSpeed(-1);
        } else if (*input == '-') {
            playerChangeSpeed(1);
        } else {
            break;
        }
        printCharacterSpeed();
    }
}

// Request user input to get the array index of the `game_objects[]`
static bool wizardRequestObjectId(int &id, const std::string &label, int start_id, int end_id) {
    std::ostringstream id_str;
    id_str << start_id << "-" << end_id;

    std::string msg = label + _(" ID (") + id_str.str() + _("): ");
    putStringClearToEOL(msg, Coord_t{0, 0});

    vtype_t input = {0};
    if (!getStringInput(input, Coord_t{0, displayWidth(msg)}, 3)) {
        return false;
    }

    int given_id;
    if (!stringToNumber(input, given_id)) {
        return false;
    }

    if (given_id < start_id || given_id > end_id) {
        putStringClearToEOL(_("Invalid ID. Must be ") + id_str.str(), Coord_t{0, 0});
        return false;
    }
    id = given_id;

    return true;
}

// Simplified wizard routine for creating an object
void wizardGenerateObject() {
    int id = 0;
    if (!wizardRequestObjectId(id, _("Dungeon/Store object"), 0, 366)) {
        return;
    }

    Coord_t coord = Coord_t{0, 0};

    for (int i = 0; i < 10; i++) {
        coord.y = py.pos.y - 3 + randomNumber(5);
        coord.x = py.pos.x - 4 + randomNumber(7);

        if (coordInBounds(coord) && dg.floor[coord.y][coord.x].feature_id <= MAX_CAVE_FLOOR && dg.floor[coord.y][coord.x].treasure_id == 0) {
            // delete any object at location, before call popt()
            if (dg.floor[coord.y][coord.x].treasure_id != 0) {
                (void) dungeonDeleteObject(coord);
            }

            // place the object
            int free_treasure_id = popt();
            dg.floor[coord.y][coord.x].treasure_id = (uint8_t) free_treasure_id;
            inventoryItemCopyTo(id, game.treasure.list[free_treasure_id]);
            magicTreasureMagicalAbility(free_treasure_id, dg.current_level);

            // auto identify the item
            itemIdentify(game.treasure.list[free_treasure_id], free_treasure_id);

            i = 9;
        }
    }
}

// Wizard routine for creating objects -RAK-
void wizardCreateObjects() {
    int number;
    vtype_t input = {0};

    printMessage(_("Warning: This routine can cause a fatal error."));

    Inventory_t forge{};
    Inventory_t &item = forge;

    item.id = config::dungeon::objects::OBJ_WIZARD;
    item.special_name_id = 0;
    itemReplaceInscription(item, _("wizard item"));
    item.identification = config::identification::ID_KNOWN2 | config::identification::ID_STORE_BOUGHT;

    putStringClearToEOL(_("Tval   : "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 9}, 3)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.category_id = (uint8_t) number;
    }

    putStringClearToEOL(_("Tchar  : "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 9}, 1)) {
        return;
    }
    item.sprite = (uint8_t) input[0];

    putStringClearToEOL(_("Subval : "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 9}, 5)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.sub_category_id = (uint8_t) number;
    }

    putStringClearToEOL(_("Weight : "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 9}, 5)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.weight = (uint16_t) number;
    }

    putStringClearToEOL(_("Number : "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 9}, 5)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.items_count = (uint8_t) number;
    }

    putStringClearToEOL(_("Damage (dice): "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 15}, 3)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.damage.dice = (uint8_t) number;
    }

    putStringClearToEOL(_("Damage (sides): "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 16}, 3)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.damage.sides = (uint8_t) number;
    }

    putStringClearToEOL(_("+To hit: "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 9}, 3)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.to_hit = (int16_t) number;
    }

    putStringClearToEOL(_("+To dam: "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 9}, 3)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.to_damage = (int16_t) number;
    }

    putStringClearToEOL(_("AC     : "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 9}, 3)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.ac = (int16_t) number;
    }

    putStringClearToEOL(_("+To AC : "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 9}, 3)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.to_ac = (int16_t) number;
    }

    putStringClearToEOL(_("P1     : "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 9}, 5)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.misc_use = (int16_t) number;
    }

    putStringClearToEOL(_("Flags (In HEX): "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 16}, 8)) {
        return;
    }

    // can't be constant string, this causes problems with
    // the GCC compiler and some scanf routines.
    char pattern[4];

    (void) strcpy(pattern, "%lx");

    int32_t input_number;
    (void) sscanf(input, pattern, &input_number);
    item.flags = (uint32_t) input_number;

    putStringClearToEOL(_("Cost : "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 9}, 8)) {
        return;
    }
    if (stringToNumber(input, input_number)) {
        item.cost = input_number;
    }

    putStringClearToEOL(_("Level : "), Coord_t{0, 0});
    if (!getStringInput(input, Coord_t{0, 10}, 3)) {
        return;
    }
    if (stringToNumber(input, number)) {
        item.depth_first_found = (uint8_t) number;
    }

    if (getInputConfirmation(_("Allocate?"))) {
        // delete object first if any, before call popt()
        Tile_t &tile = dg.floor[py.pos.y][py.pos.x];

        if (tile.treasure_id != 0) {
            (void) dungeonDeleteObject(py.pos);
        }

        number = popt();

        game.treasure.list[number] = forge;
        tile.treasure_id = (uint8_t) number;

        printMessage(_("Allocated."));
    } else {
        printMessage(_("Aborted."));
    }
}
