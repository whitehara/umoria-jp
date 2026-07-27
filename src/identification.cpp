// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Handle object identification and descriptions
// ...mostly string handling code

#include "headers.h"

char magic_item_titles[MAX_TITLES][10];

// Identified objects flags
uint8_t objects_identified[OBJECT_IDENT_SIZE];

static const char *objectDescription(char command) {
    // every printing ASCII character is listed here, in the
    // order in which they appear in the ASCII character set.
    switch (command) {
        case ' ':
            return _("  - An open pit.");
        case '!':
            return _("! - A potion.");
        case '"':
            return _("\" - An amulet, periapt, or necklace.");
        case '#':
            return _("# - A stone wall.");
        case '$':
            return _("$ - Treasure.");
        case '%':
            if (!config::options::highlight_seams) {
                return _("% - Not used.");
            }
            return _("% - A magma or quartz vein.");
        case '&':
            return _("& - Treasure chest.");
        case '\'':
            return _("' - An open door.");
        case '(':
            return _("( - Soft armor.");
        case ')':
            return _(") - A shield.");
        case '*':
            return _("* - Gems.");
        case '+':
            return _("+ - A closed door.");
        case ',':
            return _(", - Food or mushroom patch.");
        case '-':
            return _("- - A wand");
        case '.':
            return _(". - Floor.");
        case '/':
            return _("/ - A pole weapon.");
            //        case '0':
            //            return _("0 - Not used.");
        case '1':
            return _("1 - Entrance to General Store.");
        case '2':
            return _("2 - Entrance to Armory.");
        case '3':
            return _("3 - Entrance to Weaponsmith.");
        case '4':
            return _("4 - Entrance to Temple.");
        case '5':
            return _("5 - Entrance to Alchemy shop.");
        case '6':
            return _("6 - Entrance to Magic-Users store.");
            // case '7':
            //     return _("7 - Not used.");
            // case '8':
            //     return _("8 - Not used.");
            // case '9':
            //     return _("9 - Not used.");
        case ':':
            return _(": - Rubble.");
        case ';':
            return _("; - A loose rock.");
        case '<':
            return _("< - An up staircase.");
        case '=':
            return _("= - A ring.");
        case '>':
            return _("> - A down staircase.");
        case '?':
            return _("? - A scroll.");
        case '@':
            return py.misc.name;
        case 'A':
            return _("A - Giant Ant Lion.");
        case 'B':
            return _("B - The Balrog.");
        case 'C':
            return _("C - Gelatinous Cube.");
        case 'D':
            return _("D - An Ancient Dragon (Beware).");
        case 'E':
            return _("E - Elemental.");
        case 'F':
            return _("F - Giant Fly.");
        case 'G':
            return _("G - Ghost.");
        case 'H':
            return _("H - Hobgoblin.");
            // case 'I':
            //     return _("I - Invisible Stalker.");
        case 'J':
            return _("J - Jelly.");
        case 'K':
            return _("K - Killer Beetle.");
        case 'L':
            return _("L - Lich.");
        case 'M':
            return _("M - Mummy.");
            // case 'N':
            //     return _("N - Not used.");
        case 'O':
            return _("O - Ooze.");
        case 'P':
            return _("P - Giant humanoid.");
        case 'Q':
            return _("Q - Quylthulg (Pulsing Flesh Mound).");
        case 'R':
            return _("R - Reptile.");
        case 'S':
            return _("S - Giant Scorpion.");
        case 'T':
            return _("T - Troll.");
        case 'U':
            return _("U - Umber Hulk.");
        case 'V':
            return _("V - Vampire.");
        case 'W':
            return _("W - Wight or Wraith.");
        case 'X':
            return _("X - Xorn.");
        case 'Y':
            return _("Y - Yeti.");
            // case 'Z':
            //     return _("Z - Not used.");
        case '[':
            return _("[ - Hard armor.");
        case '\\':
            return _("\\ - A hafted weapon.");
        case ']':
            return _("] - Misc. armor.");
        case '^':
            return _("^ - A trap.");
        case '_':
            return _("_ - A staff.");
            // case '`':
            //     return _("` - Not used.");
        case 'a':
            return _("a - Giant Ant.");
        case 'b':
            return _("b - Giant Bat.");
        case 'c':
            return _("c - Giant Centipede.");
        case 'd':
            return _("d - Dragon.");
        case 'e':
            return _("e - Floating Eye.");
        case 'f':
            return _("f - Giant Frog.");
        case 'g':
            return _("g - Golem.");
        case 'h':
            return _("h - Harpy.");
        case 'i':
            return _("i - Icky Thing.");
        case 'j':
            return _("j - Jackal.");
        case 'k':
            return _("k - Kobold.");
        case 'l':
            return _("l - Giant Louse.");
        case 'm':
            return _("m - Mold.");
        case 'n':
            return _("n - Naga.");
        case 'o':
            return _("o - Orc or Ogre.");
        case 'p':
            return _("p - Person (Humanoid).");
        case 'q':
            return _("q - Quasit.");
        case 'r':
            return _("r - Rodent.");
        case 's':
            return _("s - Skeleton.");
        case 't':
            return _("t - Giant Tick.");
            // case 'u':
            //     return _("u - Not used.");
            // case 'v':
            //     return _("v - Not used.");
        case 'w':
            return _("w - Worm or Worm Mass.");
            // case 'x':
            //     return _("x - Not used.");
        case 'y':
            return _("y - Yeek.");
        case 'z':
            return _("z - Zombie.");
        case '{':
            return _("{ - Arrow, bolt, or bullet.");
        case '|':
            return _("| - A sword or dagger.");
        case '}':
            return _("} - Bow, crossbow, or sling.");
        case '~':
            return _("~ - Miscellaneous item.");
        default:
            return _("Not Used.");
    }
}

void identifyGameObject() {
    char itemId; // being an ASCII character representing the item/monster tile, e.g. `+` = Door.

    if (!getTileCharacter(_("Enter character to be identified :"), itemId)) {
        return;
    }

    putStringClearToEOL(objectDescription(itemId), Coord_t{0, 0});
    recallMonsterAttributes(itemId);
}

// Initialize all Potions, wands, staves, scrolls, etc.
void magicInitializeItemNames() {
    int id;

    seedSet(game.magic_seed);

    // The first 3 entries for colors are fixed, (slime & apple juice, water)
    for (int i = 3; i < MAX_COLORS; i++) {
        id = randomNumber(MAX_COLORS - 3) + 2;
        const char *color = colors[i];
        colors[i] = colors[id];
        colors[id] = color;
    }

    for (auto &w : woods) {
        id = randomNumber(MAX_WOODS) - 1;
        const char *wood = w;
        w = woods[id];
        woods[id] = wood;
    }

    for (auto &m : metals) {
        id = randomNumber(MAX_METALS) - 1;
        const char *metal = m;
        m = metals[id];
        metals[id] = metal;
    }

    for (auto &r : rocks) {
        id = randomNumber(MAX_ROCKS) - 1;
        const char *rock = r;
        r = rocks[id];
        rocks[id] = rock;
    }

    for (auto &a : amulets) {
        id = randomNumber(MAX_AMULETS) - 1;
        const char *amulet = a;
        a = amulets[id];
        amulets[id] = amulet;
    }

    for (auto &m : mushrooms) {
        id = randomNumber(MAX_MUSHROOMS) - 1;
        const char *mushroom = m;
        m = mushrooms[id];
        mushrooms[id] = mushroom;
    }

    int k;
    vtype_t title = {'\0'};

    for (auto &item_title : magic_item_titles) {
        title[0] = '\0';
        k = randomNumber(2) + 1;

        for (int i = 0; i < k; i++) {
            for (int s = randomNumber(2); s > 0; s--) {
                (void) strcat(title, syllables[randomNumber(MAX_SYLLABLES) - 1]);
            }
            if (i < k - 1) {
                (void) strcat(title, _(" "));
            }
        }

        if (title[8] == ' ') {
            title[8] = '\0';
        } else {
            title[9] = '\0';
        }

        (void) strcpy(item_title, title);
    }

    seedResetToOldSeed();
}

int16_t objectPositionOffset(int category_id, int sub_category_id) {
    switch (category_id) {
        case TV_AMULET:
            return 0;
        case TV_RING:
            return 1;
        case TV_STAFF:
            return 2;
        case TV_WAND:
            return 3;
        case TV_SCROLL1:
        case TV_SCROLL2:
            return 4;
        case TV_POTION1:
        case TV_POTION2:
            return 5;
        case TV_FOOD:
            if ((sub_category_id & (ITEM_SINGLE_STACK_MIN - 1)) < MAX_MUSHROOMS) {
                return 6;
            }
            return -1;
        default:
            return -1;
    }
}

static void clearObjectTriedFlag(int16_t id) {
    objects_identified[id] &= ~config::identification::OD_TRIED;
}

static void setObjectTriedFlag(int16_t id) {
    objects_identified[id] |= config::identification::OD_TRIED;
}

static bool isObjectKnown(int16_t id) {
    return (objects_identified[id] & config::identification::OD_KNOWN1) != 0u;
}

// Remove "Secret" symbol for identity of object
void itemSetAsIdentified(int category_id, int sub_category_id) {
    int16_t id = objectPositionOffset(category_id, sub_category_id);

    if (id < 0) {
        return;
    }

    id <<= 6;
    id += (uint8_t) (sub_category_id & (ITEM_SINGLE_STACK_MIN - 1));

    objects_identified[id] |= config::identification::OD_KNOWN1;

    // clear the tried flag, since it is now known
    clearObjectTriedFlag(id);
}

// Remove an automatically generated inscription. -CJS-
static void unsample(Inventory_t &item) {
    // this also used to clear config::identification::ID_DAMD flag, but I think it should remain set
    item.identification &= ~(config::identification::ID_MAGIK | config::identification::ID_EMPTY);

    int16_t id = objectPositionOffset(item.category_id, item.sub_category_id);

    if (id < 0) {
        return;
    }

    id <<= 6;
    id += (uint8_t) (item.sub_category_id & (ITEM_SINGLE_STACK_MIN - 1));

    // clear the tried flag, since it is now known
    clearObjectTriedFlag(id);
}

// Remove "Secret" symbol for identity of plusses
void spellItemIdentifyAndRemoveRandomInscription(Inventory_t &item) {
    unsample(item);
    item.identification |= config::identification::ID_KNOWN2;
}

bool spellItemIdentified(Inventory_t const &item) {
    return (item.identification & config::identification::ID_KNOWN2) != 0;
}

void spellItemRemoveIdentification(Inventory_t &item) {
    item.identification &= ~config::identification::ID_KNOWN2;
}

void itemIdentificationClearEmpty(Inventory_t &item) {
    item.identification &= ~config::identification::ID_EMPTY;
}

void itemIdentifyAsStoreBought(Inventory_t &item) {
    item.identification |= config::identification::ID_STORE_BOUGHT;
    spellItemIdentifyAndRemoveRandomInscription(item);
}

static bool itemStoreBought(int identification) {
    return (identification & config::identification::ID_STORE_BOUGHT) != 0;
}

// Items which don't have a 'color' are always known / itemSetAsIdentified(),
// so that they can be carried in order in the inventory.
bool itemSetColorlessAsIdentified(int category_id, int sub_category_id, int identification) {
    int16_t id = objectPositionOffset(category_id, sub_category_id);

    if (id < 0) {
        return config::identification::OD_KNOWN1 != 0u;
    }
    if (itemStoreBought(identification)) {
        return config::identification::OD_KNOWN1 != 0u;
    }

    id <<= 6;
    id += (uint8_t) (sub_category_id & (ITEM_SINGLE_STACK_MIN - 1));

    return isObjectKnown(id);
}

// Somethings been sampled -CJS-
void itemSetAsTried(Inventory_t const &item) {
    int16_t id = objectPositionOffset(item.category_id, item.sub_category_id);

    if (id < 0) {
        return;
    }

    id <<= 6;
    id += (uint8_t) (item.sub_category_id & (ITEM_SINGLE_STACK_MIN - 1));

    setObjectTriedFlag(id);
}

// Somethings been identified.
// Extra complexity by CJS so that it can merge store/dungeon objects when appropriate.
void itemIdentify(Inventory_t &item, int &item_id) {
    if (inventoryItemIsCursed(item)) {
        itemAppendToInscription(item, config::identification::ID_DAMD);
    }

    if (itemSetColorlessAsIdentified(item.category_id, item.sub_category_id, item.identification)) {
        return;
    }

    itemSetAsIdentified(item.category_id, item.sub_category_id);

    // no merging possible
    if (!inventoryItemSingleStackable(item)) {
        return;
    }

    int j;

    for (int i = 0; i < py.pack.unique_items; i++) {
        Inventory_t const &t_ptr = py.inventory[i];

        bool matching_cat = t_ptr.category_id == item.category_id;
        bool matching_sub_cat = t_ptr.sub_category_id == item.sub_category_id;
        int total_items_count = (int) t_ptr.items_count + item.items_count;

        if (matching_cat && matching_sub_cat && i != item_id && total_items_count < 256) {
            // make *item_id the smaller number
            if (item_id > i) {
                j = item_id;
                item_id = i;
                i = j;
            }

            printMessage(_("You combine similar objects from the shop and dungeon."));

            py.inventory[item_id].items_count += py.inventory[i].items_count;
            py.pack.unique_items--;

            for (j = i; j < py.pack.unique_items; j++) {
                py.inventory[j] = py.inventory[j + 1];
            }

            inventoryItemCopyTo(config::dungeon::objects::OBJ_NOTHING, py.inventory[j]);
        }
    }
}

// If an object has lost magical properties,
// remove the appropriate portion of the name. -CJS-
void itemRemoveMagicNaming(Inventory_t &item) {
    item.special_name_id = SpecialNameIds::SN_NULL;
}

int bowDamageValue(int16_t misc_use) {
    if (misc_use == 1 || misc_use == 2) {
        return 2;
    }
    if (misc_use == 3 || misc_use == 5) {
        return 3;
    }
    if (misc_use == 4 || misc_use == 6) {
        return 4;
    }
    return -1;
}

// determines how the `item.misc_use` field is printed
enum class ItemMiscUse {
    Ignored,
    Charges,
    Plusses,
    Light,
    Flags,
    ZPlusses,
};

// Set the `description` for an inventory item.
// The `add_prefix` param indicates that an article must be added.
// Note that since out_val can easily exceed 80 characters, itemDescription
// must always be called with a obj_desc_t as the first parameter.
void itemDescription(obj_desc_t description, Inventory_t const &item, bool add_prefix) {
    int indexx = item.sub_category_id & (ITEM_SINGLE_STACK_MIN - 1);

    // base name, modifier string
    // Translated with the "&"/"~" grammar markers kept in place (msgid is
    // the raw English template, e.g. "& Mushroom~"); the marker-position
    // logic below operates on whatever the active catalog returns.
    const char *basenm = _(game_objects[item.id].name);
    const char *modstr = CNIL;

    vtype_t damstr = {'\0'};
    damstr[0] = '\0';

    bool append_name = false;
    bool modify = !itemSetColorlessAsIdentified(item.category_id, item.sub_category_id, item.identification);
    ItemMiscUse misc_type = ItemMiscUse::Ignored;

    switch (item.category_id) {
        case TV_MISC:
        case TV_CHEST:
            break;
        case TV_SLING_AMMO:
        case TV_BOLT:
        case TV_ARROW:
            (void) snprintf(damstr, MORIA_MESSAGE_SIZE, " (%dd%d)", item.damage.dice, item.damage.sides);
            break;
        case TV_LIGHT:
            misc_type = ItemMiscUse::Light;
            break;
        case TV_SPIKE:
            break;
        case TV_BOW:
            (void) snprintf(damstr, MORIA_MESSAGE_SIZE, " (x%d)", bowDamageValue(item.misc_use));
            break;
        case TV_HAFTED:
        case TV_POLEARM:
        case TV_SWORD:
            (void) snprintf(damstr, MORIA_MESSAGE_SIZE, " (%dd%d)", item.damage.dice, item.damage.sides);
            misc_type = ItemMiscUse::Flags;
            break;
        case TV_DIGGING:
            misc_type = ItemMiscUse::ZPlusses;
            (void) snprintf(damstr, MORIA_MESSAGE_SIZE, " (%dd%d)", item.damage.sides, item.damage.sides);
            break;
        case TV_BOOTS:
        case TV_GLOVES:
        case TV_CLOAK:
        case TV_HELM:
        case TV_SHIELD:
        case TV_HARD_ARMOR:
        case TV_SOFT_ARMOR:
            break;
        case TV_AMULET:
            if (modify) {
                basenm = _("& %s Amulet");
                modstr = C_("flavor", amulets[indexx]);
            } else {
                basenm = _("& Amulet");
                append_name = true;
            }
            misc_type = ItemMiscUse::Plusses;
            break;
        case TV_RING:
            if (modify) {
                basenm = _("& %s Ring");
                modstr = C_("flavor", rocks[indexx]);
            } else {
                basenm = _("& Ring");
                append_name = true;
            }
            misc_type = ItemMiscUse::Plusses;
            break;
        case TV_STAFF:
            if (modify) {
                basenm = _("& %s Staff");
                modstr = C_("flavor", woods[indexx]);
            } else {
                basenm = _("& Staff");
                append_name = true;
            }
            misc_type = ItemMiscUse::Charges;
            break;
        case TV_WAND:
            if (modify) {
                basenm = _("& %s Wand");
                modstr = C_("flavor", metals[indexx]);
            } else {
                basenm = _("& Wand");
                append_name = true;
            }
            misc_type = ItemMiscUse::Charges;
            break;
        case TV_SCROLL1:
        case TV_SCROLL2:
            if (modify) {
                basenm = _("& Scroll~ titled \"%s\"");
                modstr = magic_item_titles[indexx];
            } else {
                basenm = _("& Scroll~");
                append_name = true;
            }
            break;
        case TV_POTION1:
        case TV_POTION2:
            if (modify) {
                basenm = _("& %s Potion~");
                modstr = C_("flavor", colors[indexx]);
            } else {
                basenm = _("& Potion~");
                append_name = true;
            }
            break;
        case TV_FLASK:
            break;
        case TV_FOOD:
            if (modify) {
                if (indexx <= 15) {
                    basenm = _("& %s Mushroom~");
                } else if (indexx <= 20) {
                    basenm = _("& Hairy %s Mold~");
                }
                if (indexx <= 20) {
                    modstr = C_("flavor", mushrooms[indexx]);
                }
            } else {
                append_name = true;
                if (indexx <= 15) {
                    basenm = _("& Mushroom~");
                } else if (indexx <= 20) {
                    basenm = _("& Hairy Mold~");
                } else {
                    // Ordinary food does not have a name appended.
                    append_name = false;
                }
            }
            break;
        case TV_MAGIC_BOOK:
            modstr = basenm;
            basenm = _("& Book~ of Magic Spells %s");
            break;
        case TV_PRAYER_BOOK:
            modstr = basenm;
            basenm = _("& Holy Book~ of Prayers %s");
            break;
        case TV_OPEN_DOOR:
        case TV_CLOSED_DOOR:
        case TV_SECRET_DOOR:
        case TV_RUBBLE:
            break;
        case TV_GOLD:
        case TV_INVIS_TRAP:
        case TV_VIS_TRAP:
        case TV_UP_STAIR:
        case TV_DOWN_STAIR:
            (void) strcpy(description, _(game_objects[item.id].name));
            (void) strcat(description, _("."));
            return;
        case TV_STORE_DOOR:
            (void) snprintf(description, MORIA_OBJ_DESC_SIZE, _("the entrance to the %s."), _(game_objects[item.id].name));
            return;
        default:
            (void) strcpy(description, _("Error in objdes()"));
            return;
    }

    obj_desc_t tmp_val = {'\0'};

    if (modstr != CNIL) {
        (void) snprintf(tmp_val, MORIA_OBJ_DESC_SIZE, basenm, modstr);
    } else {
        (void) strcpy(tmp_val, basenm);
    }

    if (append_name) {
        (void) strcat(tmp_val, _(" of "));
        (void) strcat(tmp_val, _(game_objects[item.id].name));
    }

    if (lang::currentLanguage() == "ja") {
        // Japanese has no plural inflection; the "~" marker is simply
        // dropped regardless of count (unlike English, which turns it
        // into "s"/"ches" for counts other than 1).
        insertStringIntoString(tmp_val, "ch~", CNIL);
        insertStringIntoString(tmp_val, "~", CNIL);
    } else if (item.items_count != 1) {
        insertStringIntoString(tmp_val, "ch~", "ches");
        insertStringIntoString(tmp_val, "~", "s");
    } else {
        insertStringIntoString(tmp_val, "~", CNIL);
    }

    if (!add_prefix) {
        if (strncmp("some", tmp_val, 4) == 0) {
            (void) strcpy(description, &tmp_val[5]);
        } else if (tmp_val[0] == '&') {
            // eliminate the '& ' at the beginning
            (void) strcpy(description, &tmp_val[2]);
        } else {
            (void) strcpy(description, tmp_val);
        }
        return;
    }

    // TODO(cook): `spellItemIdentified()` is called several times in this
    // function, but `item` is immutable, so we should be able to call and
    // assign it once, then use that value everywhere below.
    if (item.special_name_id != SpecialNameIds::SN_NULL && spellItemIdentified(item)) {
        (void) strcat(tmp_val, _(" "));
        (void) strcat(tmp_val, _(special_item_names[item.special_name_id]));
    }

    if (damstr[0] != '\0') {
        (void) strcat(tmp_val, damstr);
    }

    vtype_t tmp_str = {'\0'};

    if (spellItemIdentified(item)) {
        auto abs_to_hit = (int) std::abs((std::intmax_t) item.to_hit);
        auto abs_to_damage = (int) std::abs((std::intmax_t) item.to_damage);

        if ((item.identification & config::identification::ID_SHOW_HIT_DAM) != 0) {
            (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, " (%c%d,%c%d)", (item.to_hit < 0) ? '-' : '+', abs_to_hit, (item.to_damage < 0) ? '-' : '+', abs_to_damage);
        } else if (item.to_hit != 0) {
            (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, " (%c%d)", (item.to_hit < 0) ? '-' : '+', abs_to_hit);
        } else if (item.to_damage != 0) {
            (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, " (%c%d)", (item.to_damage < 0) ? '-' : '+', abs_to_damage);
        } else {
            tmp_str[0] = '\0';
        }
        (void) strcat(tmp_val, tmp_str);
    }

    // Crowns have a zero base AC, so make a special test for them.
    auto abs_to_ac = (int) std::abs((std::intmax_t) item.to_ac);
    if (item.ac != 0 || item.category_id == TV_HELM) {
        (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, " [%d", item.ac);
        (void) strcat(tmp_val, tmp_str);
        if (spellItemIdentified(item)) {
            // originally used %+d, but several machines don't support it
            (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, ",%c%d", (item.to_ac < 0) ? '-' : '+', abs_to_ac);
            (void) strcat(tmp_val, tmp_str);
        }
        (void) strcat(tmp_val, _("]"));
    } else if (item.to_ac != 0 && spellItemIdentified(item)) {
        // originally used %+d, but several machines don't support it
        (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, " [%c%d]", (item.to_ac < 0) ? '-' : '+', abs_to_ac);
        (void) strcat(tmp_val, tmp_str);
    }

    // override defaults, check for `misc_type` flags in the `item.identification` field
    if ((item.identification & config::identification::ID_NO_SHOW_P1) != 0) {
        misc_type = ItemMiscUse::Ignored;
    } else if ((item.identification & config::identification::ID_SHOW_P1) != 0) {
        misc_type = ItemMiscUse::ZPlusses;
    }

    tmp_str[0] = '\0';

    if (misc_type == ItemMiscUse::Light) {
        (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, _(" with %d turns of light"), item.misc_use);
    } else if (misc_type == ItemMiscUse::Ignored) {
        // NOOP
    } else if (spellItemIdentified(item)) {
        auto abs_misc_use = (int) std::abs((std::intmax_t) item.misc_use);

        if (misc_type == ItemMiscUse::ZPlusses) {
            // originally used %+d, but several machines don't support it
            (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, " (%c%d)", (item.misc_use < 0) ? '-' : '+', abs_misc_use);
        } else if (misc_type == ItemMiscUse::Charges) {
            (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, _(" (%d charges)"), item.misc_use);
        } else if (item.misc_use != 0) {
            if (misc_type == ItemMiscUse::Plusses) {
                (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, " (%c%d)", (item.misc_use < 0) ? '-' : '+', abs_misc_use);
            } else if (misc_type == ItemMiscUse::Flags) {
                if ((item.flags & config::treasure::flags::TR_STR) != 0u) {
                    (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, _(" (%c%d to STR)"), (item.misc_use < 0) ? '-' : '+', abs_misc_use);
                } else if ((item.flags & config::treasure::flags::TR_STEALTH) != 0u) {
                    (void) snprintf(tmp_str, MORIA_MESSAGE_SIZE, _(" (%c%d to stealth)"), (item.misc_use < 0) ? '-' : '+', abs_misc_use);
                }
            }
        }
    }
    (void) strcat(tmp_val, tmp_str);

    // ampersand is always the first character
    if (tmp_val[0] == '&' && lang::currentLanguage() == "ja") {
        // Japanese has no indefinite articles ("a"/"an"): render a bare
        // quantity prefix for counts other than 1, and nothing at all
        // (just the noun, dropping the "& ") for exactly one item.
        if (item.items_count > 1) {
            (void) snprintf(description, MORIA_OBJ_DESC_SIZE, "%d%.*s", (unsigned int) item.items_count, (int) MORIA_OBJ_DESC_SIZE - 4, &tmp_val[1]);
        } else if (item.items_count < 1) {
            (void) snprintf(description, MORIA_OBJ_DESC_SIZE, "%s%.*s", _("no more"), (int) MORIA_OBJ_DESC_SIZE - 8, &tmp_val[1]);
        } else {
            (void) snprintf(description, MORIA_OBJ_DESC_SIZE, "%.*s", (int) MORIA_OBJ_DESC_SIZE - 1, &tmp_val[2]);
        }
    } else if (tmp_val[0] == '&') {
        // use &tmp_val[1], so that & does not appear in output
        if (item.items_count > 1) {
            (void) snprintf(description, MORIA_OBJ_DESC_SIZE, "%d%.*s", (unsigned int) item.items_count, (int) MORIA_OBJ_DESC_SIZE - 4, &tmp_val[1]);
        } else if (item.items_count < 1) {
            (void) snprintf(description, MORIA_OBJ_DESC_SIZE, "%s%.*s", _("no more"), (int) MORIA_OBJ_DESC_SIZE - 8, &tmp_val[1]);
        } else if (isVowel(tmp_val[2])) {
            (void) snprintf(description, MORIA_OBJ_DESC_SIZE, "an%.*s", (int) MORIA_OBJ_DESC_SIZE - 3, &tmp_val[1]);
        } else {
            (void) snprintf(description, MORIA_OBJ_DESC_SIZE, "a%.*s", (int) MORIA_OBJ_DESC_SIZE - 2, &tmp_val[1]);
        }
    } else if (item.items_count < 1) {
        // handle 'no more' case specially

        int max_width = MORIA_OBJ_DESC_SIZE - sizeof("no more ");

        if (lang::currentLanguage() == "ja") {
            // Japanese has no "some"-prefix article to strip (that's an
            // English-only convention from the branch above), so there's
            // nothing to special-case here beyond substituting the
            // translated "no more" phrase.
            (void) snprintf(description, MORIA_OBJ_DESC_SIZE, "%s %.*s", _("no more"), max_width, tmp_val);
        } else if (strncmp("some", tmp_val, 4) == 0) {
            // check for "some" at start
            (void) snprintf(description, MORIA_OBJ_DESC_SIZE, _("no more %.*s"), max_width, &tmp_val[5]);
        } else {
            // here if no article
            (void) snprintf(description, MORIA_OBJ_DESC_SIZE, _("no more %.*s"), max_width, tmp_val);
        }
    } else {
        (void) strcpy(description, tmp_val);
    }

    tmp_str[0] = '\0';

    if ((indexx = objectPositionOffset(item.category_id, item.sub_category_id)) >= 0) {
        indexx <<= 6;
        indexx += (item.sub_category_id & (ITEM_SINGLE_STACK_MIN - 1));

        // don't print tried string for store bought items
        if (((objects_identified[indexx] & config::identification::OD_TRIED) != 0) && !itemStoreBought(item.identification)) {
            (void) strcat(tmp_str, _("tried "));
        }
    }

    if ((item.identification & (config::identification::ID_MAGIK | config::identification::ID_EMPTY | config::identification::ID_DAMD)) != 0) {
        if ((item.identification & config::identification::ID_MAGIK) != 0) {
            (void) strcat(tmp_str, _("magik "));
        }
        if ((item.identification & config::identification::ID_EMPTY) != 0) {
            (void) strcat(tmp_str, _("empty "));
        }
        if ((item.identification & config::identification::ID_DAMD) != 0) {
            (void) strcat(tmp_str, _("damned "));
        }
    }

    if (item.inscription[0] != '\0') {
        (void) strcat(tmp_str, item.inscription);
    } else if ((indexx = (int) strlen(tmp_str)) > 0) {
        // remove the extra blank at the end
        tmp_str[indexx - 1] = '\0';
    }

    if (tmp_str[0] != 0) {
        (void) snprintf(tmp_val, MORIA_OBJ_DESC_SIZE, " {%s}", tmp_str);
        (void) strcat(description, tmp_val);
    }

    (void) strcat(description, _("."));
}

// Describe number of remaining charges. -RAK-
void itemChargesRemainingDescription(int item_id) {
    if (!spellItemIdentified(py.inventory[item_id])) {
        return;
    }

    int rem_num = py.inventory[item_id].misc_use;

    vtype_t out_val = {'\0'};
    (void) snprintf(out_val, MORIA_MESSAGE_SIZE, _("You have %d charges remaining."), rem_num);
    printMessage(out_val);
}

// Describe amount of item remaining. -RAK-
void itemTypeRemainingCountDescription(int item_id) {
    Inventory_t &item = py.inventory[item_id];

    item.items_count--;

    obj_desc_t tmp_str = {'\0'};
    itemDescription(tmp_str, item, true);

    item.items_count++;

    // the string already has a dot at the end.
    obj_desc_t out_val = {'\0'};
    (void) snprintf(out_val, MORIA_OBJ_DESC_SIZE, _("You have %s"), tmp_str);
    printMessage(out_val);
}

// Add a comment to an object description. -CJS-
void itemInscribe() {
    if (py.pack.unique_items == 0 && py.equipment_count == 0) {
        printMessage(_("You are not carrying anything to inscribe."));
        return;
    }

    int item_id;
    if (!inventoryGetInputForItemId(item_id, _("Which one? "), 0, PLAYER_INVENTORY_SIZE, CNIL, CNIL)) {
        return;
    }

    obj_desc_t msg = {'\0'};
    itemDescription(msg, py.inventory[item_id], true);

    obj_desc_t inscription = {'\0'};
    (void) snprintf(inscription, MORIA_OBJ_DESC_SIZE, _("Inscribing %s"), msg);

    printMessage(inscription);

    if (py.inventory[item_id].inscription[0] != '\0') {
        (void) snprintf(inscription, MORIA_OBJ_DESC_SIZE, _("Replace %s New inscription:"), py.inventory[item_id].inscription);
    } else {
        (void) strcpy(inscription, _("Inscription: "));
    }

    // Column budget, not byte count: `msg` (the item description) can
    // contain translated (Japanese) monster/item names, where multi-byte
    // characters make strlen() overcount relative to display columns.
    int msg_len = 78 - displayWidth(msg);
    if (msg_len > 12) {
        msg_len = 12;
    }

    putStringClearToEOL(inscription, Coord_t{0, 0});

    // Same reasoning: `inscription` is itself a translated prompt/label at
    // this point, so the cursor must be placed by display column.
    if (getStringInput(inscription, Coord_t{0, displayWidth(inscription)}, msg_len)) {
        itemReplaceInscription(py.inventory[item_id], inscription);
    }
}

// Append an additional comment to an object description. -CJS-
void itemAppendToInscription(Inventory_t &item, uint8_t item_ident_type) {
    item.identification |= item_ident_type;
}

// Replace any existing comment in an object description with a new one. -CJS-
void itemReplaceInscription(Inventory_t &item, const char *inscription) {
    (void) strcpy(item.inscription, inscription);
}

void objectBlockedByMonster(int monster_id) {
    vtype_t description = {'\0'};
    vtype_t msg = {'\0'};

    Monster_t const &monster = monsters[monster_id];
    const char *name = _(creatures_list[monster.creature_id].name);

    if (monster.lit) {
        if (lang::currentLanguage() == "ja") {
            (void) snprintf(description, MORIA_MESSAGE_SIZE, "%s", name);
        } else {
            (void) snprintf(description, MORIA_MESSAGE_SIZE, _("The %s"), name);
        }
    } else {
        (void) strcpy(description, _("Something"));
    }

    (void) snprintf(msg, MORIA_MESSAGE_SIZE, _("%s is in your way!"), description);
    printMessage(msg);
}
