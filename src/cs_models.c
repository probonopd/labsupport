/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* The devices and their protocols. A product ID that is not verified is 0. */

#include "cs_internal.h"

#include <stddef.h>

extern const cs_protocol cs_proto_minilab3;
extern const cs_protocol cs_proto_keylab;
extern const cs_protocol cs_proto_keylab_essential;
extern const cs_protocol cs_proto_keylab_mk2;
extern const cs_protocol cs_proto_keylab_essential_mk3;
extern const cs_protocol cs_proto_keylab_mk3;

#define ARTURIA 0x1C75

const cs_model cs_models[] = {
    /* name, family, vendor, products, product string, pads, knobs, faders, lines, chars, rgb, bars */
    {"MiniLab 3", CS_FAMILY_MINILAB3, ARTURIA, {0x020B, 0x220B}, "Minilab3", 16, 8, 4, 2, 30, 1, 1},
    /* USB IDs carry a size code in the third digit: 0x24x = 49 keys,
     * 0x28x = 61, 0x2Cx = 88. The Essential mk3 product string is the short
     * "KL Essential" the unit reports. */
    /* First generation. The Essential has two hardware
     * generations with a PID each; the product string of the second is assumed
     * to equal the first's. Pads
     * of the Essential are RGB (0..31 a channel); the KeyLab has single color
     * pads and none on the 25. */
    {"KeyLab 25", CS_FAMILY_KEYLAB, ARTURIA, {0x0205, 0}, "KeyLab 25", 0, 10, 9, 2, 16, 0, 0},
    {"KeyLab 49", CS_FAMILY_KEYLAB, ARTURIA, {0x0245, 0}, "KeyLab 49", 16, 10, 9, 2, 16, 0, 0},
    {"KeyLab 61", CS_FAMILY_KEYLAB, ARTURIA, {0x0285, 0}, "KeyLab 61", 16, 10, 9, 2, 16, 0, 0},
    {"KeyLab 88", CS_FAMILY_KEYLAB, ARTURIA, {0x02CD, 0}, "KeyLab 88", 16, 10, 9, 2, 16, 0, 0},
    {"KeyLab Essential 49", CS_FAMILY_KEYLAB_ESSENTIAL, ARTURIA, {0x024A, 0x224A}, "Arturia KeyLab Essential 49", 8, 9, 9, 2, 16, 1, 0},
    {"KeyLab Essential 61", CS_FAMILY_KEYLAB_ESSENTIAL, ARTURIA, {0x028A, 0x228A}, "Arturia KeyLab Essential 61", 8, 9, 9, 2, 16, 1, 0},
    {"KeyLab Essential 88", CS_FAMILY_KEYLAB_ESSENTIAL, ARTURIA, {0x02CA, 0x22CA}, "Arturia KeyLab Essential 88", 8, 9, 9, 2, 16, 1, 0},
    {"KeyLab mkII 49", CS_FAMILY_KEYLAB_MK2, ARTURIA, {0x024B, 0}, "KeyLab mkII 49", 16, 9, 9, 2, 16, 1, 0},
    {"KeyLab mkII 61", CS_FAMILY_KEYLAB_MK2, ARTURIA, {0x028B, 0}, "KeyLab mkII 61", 16, 9, 9, 2, 16, 1, 0},
    {"KeyLab mkII 88", CS_FAMILY_KEYLAB_MK2, ARTURIA, {0x02CB, 0}, "KeyLab mkII 88", 16, 9, 9, 2, 16, 1, 0},
    {"KeyLab Essential 49 mk3", CS_FAMILY_KEYLAB_ESSENTIAL_MK3, ARTURIA, {0x024C, 0}, "KL Essential 49 mk3", 16, 9, 9, 2, 18, 1, 1},
    {"KeyLab Essential 61 mk3", CS_FAMILY_KEYLAB_ESSENTIAL_MK3, ARTURIA, {0x028C, 0}, "KL Essential 61 mk3", 16, 9, 9, 2, 18, 1, 1},
    {"KeyLab Essential 88 mk3", CS_FAMILY_KEYLAB_ESSENTIAL_MK3, ARTURIA, {0x02CC, 0}, "KL Essential 88 mk3", 16, 9, 9, 2, 18, 1, 1},
    {"KeyLab 49 mk3", CS_FAMILY_KEYLAB_MK3, ARTURIA, {0x024E, 0}, "KeyLab 49 mk3", 12, 9, 9, 2, 18, 1, 1},
    {"KeyLab 61 mk3", CS_FAMILY_KEYLAB_MK3, ARTURIA, {0x028E, 0}, "KeyLab 61 mk3", 12, 9, 9, 2, 18, 1, 1},
    {"KeyLab 88 mk3", CS_FAMILY_KEYLAB_MK3, ARTURIA, {0x02CE, 0}, "KeyLab 88 mk3", 12, 9, 9, 2, 18, 1, 1},
    {NULL, 0, 0, {0, 0}, NULL, 0, 0, 0, 0, 0, 0, 0},
};

const cs_protocol *cs_protocol_of(cs_family f)
{
    switch (f) {
    case CS_FAMILY_MINILAB3:
        return &cs_proto_minilab3;
    case CS_FAMILY_KEYLAB:
        return &cs_proto_keylab;
    case CS_FAMILY_KEYLAB_ESSENTIAL:
        return &cs_proto_keylab_essential;
    case CS_FAMILY_KEYLAB_MK2:
        return &cs_proto_keylab_mk2;
    case CS_FAMILY_KEYLAB_ESSENTIAL_MK3:
        return &cs_proto_keylab_essential_mk3;
    case CS_FAMILY_KEYLAB_MK3:
        return &cs_proto_keylab_mk3;
    default:
        return NULL;
    }
}
