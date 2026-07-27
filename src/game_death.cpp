// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Code executed when player dies

#include "headers.h"

// Prints the gravestone of the character -RAK-
static void printTomb() {
    displayDeathFile(config::files::death_tomb);

    std::string text;

    text = std::string(py.misc.name);
    putString(text.c_str(), Coord_t{6, (int) (26 - displayWidth(text) / 2)});

    if (!game.total_winner) {
        text = playerRankTitle();
    } else {
        text = _("Magnificent");
    }
    putString(text.c_str(), Coord_t{8, (int) (26 - displayWidth(text) / 2)});

    if (!game.total_winner) {
        text = _(classes[py.misc.class_id].title);
    } else if (playerIsMale()) {
        text = _("*King*");
    } else {
        text = _("*Queen*");
    }
    putString(text.c_str(), Coord_t{10, (int) (26 - displayWidth(text) / 2)});

    text = std::to_string(py.misc.level);
    putString(text.c_str(), Coord_t{11, 30});

    text = std::to_string(py.misc.exp) + _(" Exp");
    putString(text.c_str(), Coord_t{12, (int) (26 - displayWidth(text) / 2)});

    text = std::to_string(py.misc.au) + _(" Au");
    putString(text.c_str(), Coord_t{13, (int) (26 - displayWidth(text) / 2)});

    text = std::to_string(dg.current_level);
    putString(text.c_str(), Coord_t{14, 34});

    text = std::string(game.character_died_from);
    putString(text.c_str(), Coord_t{16, (int) (26 - displayWidth(text) / 2)});

    char day[11];
    humanDateString(day);
    text = std::string(day);
    putString(text.c_str(), Coord_t{17, (int) (26 - displayWidth(text) / 2)});

retry:
    flushInputBuffer();

    putString(_("(ESC to abort, return to print on screen, or file name)"), Coord_t{23, 0});
    const char *record_prompt = _("Character record?");
    putString(record_prompt, Coord_t{22, 0});

    vtype_t str = {'\0'};
    if (getStringInput(str, Coord_t{22, displayWidth(record_prompt)}, 60)) {
        for (auto &item : py.inventory) {
            itemSetAsIdentified(item.category_id, item.sub_category_id);
            spellItemIdentifyAndRemoveRandomInscription(item);
        }

        playerRecalculateBonuses();

        if (str[0] != 0) {
            if (!outputPlayerCharacterToFile(str)) {
                goto retry;
            }
        } else {
            clearScreen();
            printCharacter();
            putString(_("Type ESC to skip the inventory:"), Coord_t{23, 0});
            if (getKeyInput() != ESCAPE) {
                clearScreen();
                printMessage(_("You are using:"));
                (void) displayEquipment(true, 0);
                printMessage(CNIL);
                printMessage(_("You are carrying:"));
                clearToBottom(1);
                (void) displayInventoryItems(0, py.pack.unique_items - 1, true, 0, CNIL);
                printMessage(CNIL);
            }
        }
    }
}

// Let the player know they did good.
static void printCrown() {
    displayDeathFile(config::files::death_royal);
    if (playerIsMale()) {
        putString(_("King!"), Coord_t{17, 45});
    } else {
        putString(_("Queen!"), Coord_t{17, 45});
    }
    flushInputBuffer();
    waitForContinueKey(23);
}

// Change the player into a King! -RAK-
static void kingly() {
    // Change the character attributes.
    dg.current_level = 0;
    (void) strcpy(game.character_died_from, _("Ripe Old Age"));

    (void) spellRestorePlayerLevels();

    py.misc.level += PLAYER_MAX_LEVEL;
    py.misc.au += 250000L;
    py.misc.max_exp += 5000000L;
    py.misc.exp = py.misc.max_exp;

    printCrown();
}

// What happens upon dying -RAK-
// Handles the gravestone and top-twenty routines -RAK-
void endGame() {
    printMessage(CNIL);

    // flush all input
    flushInputBuffer();

    // If the game has been saved, then save sets turn back to -1,
    // which inhibits the printing of the tomb.
    if (dg.game_turn >= 0) {
        if (game.total_winner) {
            kingly();
        }
        printTomb();
    }

    // Save the memory at least.
    if (game.character_generated && !game.character_saved) {
        (void) saveGame();
    }

    // add score to score file if applicable
    if (game.character_generated) {
        // Clear `game.character_saved`, strange thing to do, but it prevents
        // getKeyInput() from recursively calling endGame() when there has
        // been an eof on stdin detected.
        game.character_saved = false;
        recordNewHighScore();
        showScoresScreen();
    }
    eraseLine(Coord_t{23, 0});

    exitProgram();
}
