/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Arturia KeyLab 25/49/61/88 (first generation, 2014). Source:
 * the Arturia KeyLab user manual, forum tables and public DAW scripts, and
 * observation of the unit (unit versions 1.19, 1.33 and, on the 88,
 * 1.0.17 and 1.1.0.4).
 *
 * Why no mode: this generation has no DAW mode, and the unit has neither a
 * mode flag nor an Analog Lab detection; the display runs in any state
 * and the lights take any host write. So everything is on the one cable there
 * is, in the unit's factory state, and connect has just one thing to make
 * sure of: that the ten encoders send positions, not steps. The factory
 * memory 0 does, memories 1 to 10 do not, and the 88 comes back in the memory
 * it was last in, so connect writes the global knob mode "absolute"
 * (02 00 40 02 01). Global item 2 is KnobMode: the forum table and Bitwig name
 * it, and the unit agrees by value (the factory memories 1 to
 * 10 store 0x7F, "relative", in that item's slot; memory 0 holds 01,
 * "absolute"; item names by the manual's order, medium confidence). The write changes the working memory only, so
 * knobs are absolute by default in memory 0 only, and it is not undone: the
 * previous mode is not known. Disconnect sends nothing: the unit redraws its
 * own display on the next control change.
 *
 * From the unit's factory defaults: the Param and
 * Value encoders (CC 112 and 114, relative, binary offset around 64; clicks
 * CC 113 and 115), the ten encoders (control ids 1 to 10 hold CC 74, 71, 76,
 * 77, 18, 19, 16, 17, 93, 91; with the hardware id table that is 74, 71, 76, 77,
 * 93, 18, 19, 16, 17, 91 by position), the nine faders (CC 73, 75, 79, 72, 80,
 * 81, 82, 83, 85) and the pads (notes 36 to 51, channel 10). The factory
 * channel is 1 for the controls, but they are matched on any channel because
 * a user memory may change it. A turn is one message per detent; the step
 * (1 to 3) grows with the speed and the "Knob Accl" setting.
 *
 * Left out: the switches (factory CC 22 to 31, press and release values not
 * traced), Sound and Multi (CC 118, 119), Bank 1 and 2 (CC 47, 46), the Volume
 * encoder (CC 7) and Edit, which have no role here, the transport buttons (MMC
 * system exclusive by default, not channel messages), a value bar (none), and
 * the lamps of the buttons (LED ids 0x12 to 0x1B, 0x1E, 0x1F and 0x58 to 0x5D
 * are accepted, but the pairing with physical lamps is by id only). There are
 * no arrow, Back or OK buttons on this generation: the Param and Value encoder
 * clicks are the only presses (Category and Preset in Analog Lab). */

#include "cs_internal.h"

static const cs_control controls[] = {
    /* The unit's own 16 pads; pad notes of other channels are keys */
    {0x90, 9, 36, 16, CS_EV_PAD, 0, 0},
    /* Param encoder and Value encoder, turn then click */
    {0xB0, -1, 112, 1, CS_EV_ENCODER, 0, 0},
    {0xB0, -1, 113, 1, CS_EV_CLICK, 0, 0},
    {0xB0, -1, 114, 1, CS_EV_ENCODER, 0, 0},
    {0xB0, -1, 115, 1, CS_EV_CLICK, 0, 0},
    /* Encoders 1 to 10 by position */
    {0xB0, -1, 74, 1, CS_EV_KNOB, 0, 0},
    {0xB0, -1, 71, 1, CS_EV_KNOB, 1, 0},
    {0xB0, -1, 76, 2, CS_EV_KNOB, 2, 0},
    {0xB0, -1, 93, 1, CS_EV_KNOB, 4, 0},
    {0xB0, -1, 18, 2, CS_EV_KNOB, 5, 0},
    {0xB0, -1, 16, 2, CS_EV_KNOB, 7, 0},
    {0xB0, -1, 91, 1, CS_EV_KNOB, 9, 0},
    /* Faders 1 to 9 */
    {0xB0, -1, 73, 1, CS_EV_FADER, 0, 0},
    {0xB0, -1, 75, 1, CS_EV_FADER, 1, 0},
    {0xB0, -1, 79, 1, CS_EV_FADER, 2, 0},
    {0xB0, -1, 72, 1, CS_EV_FADER, 3, 0},
    {0xB0, -1, 80, 4, CS_EV_FADER, 4, 0},
    {0xB0, -1, 85, 1, CS_EV_FADER, 8, 0},
};

/* The LCD is 2 x 16 characters; a 17th would spill into the next line. */
#define LINE_MAX 16

/* A short line is padded by the unit, but sending the padding keeps an empty
 * line independent of how the unit treats an empty text. */
static size_t text(const cs_model *m, const char *l1, const char *l2, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    cs_out_arturia(&o);
    cs_out_byte(&o, 0x04);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x60);
    cs_out_byte(&o, 0x01);
    cs_out_ascii(&o, l1, LINE_MAX, LINE_MAX);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x02);
    cs_out_ascii(&o, l2, LINE_MAX, LINE_MAX);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

static size_t connect(const cs_model *m, uint8_t *buf, size_t cap)
{
    static const uint8_t knobs_absolute[] = {0x02, 0x00, 0x40, 0x02, 0x01, 0xF7};
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    cs_out_arturia(&o);
    cs_out_bytes(&o, knobs_absolute, sizeof knobs_absolute);
    return cs_out_end(&o);
}

/* Pads 1-16 are LED ids 0x70-0x7F of the one LED command; any non-zero value
 * is on (bit 0 matters for the switch LEDs only), and there is no color. */
static size_t pad_light(const cs_model *m, int pad, int r, int g, int b, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    cs_out_arturia(&o);
    cs_out_byte(&o, 0x02);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x10);
    cs_out_byte(&o, 0x70 + pad);
    cs_out_byte(&o, (r | g | b) != 0 ? 0x7F : 0x00);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

const cs_protocol cs_proto_keylab = {
    controls, sizeof controls / sizeof controls[0], CS_ENC_OFFSET,
    /* The one cable; ALSA names it "KeyLab 61 MIDI 1" (a single port gets a
     * number), newer kernels "KeyLab 61 MIDI" */
    {"midi|midi1", NULL, NULL, NULL},
    {CS_ROLE_KEYS | CS_ROLE_CONTROLS | CS_ROLE_DISPLAY, 0, 0, 0},
    connect, NULL, text, NULL, pad_light,
};
