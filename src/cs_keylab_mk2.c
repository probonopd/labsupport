/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Arturia KeyLab mkII 49/61/88 in its Analog Lab mode, the mode the unit starts
 * in. Sources: the Arturia KeyLab mkII user manual and public DAW scripts.
 *
 * Why Analog Lab and not DAW mode: the unit accepts host text in both, but
 * no SysEx changes the mode: command 05 is ignored, and command 06 only
 * reloads a stored memory (only the front panel switches). Analog Lab needs no switch, and its controls are all
 * on cable 0 ("MIDI"), the one that also carries the keys. So connect and
 * disconnect send nothing; if the user leaves Analog Lab by hand, the display
 * and the controls stop working until the Analog Lab button is pressed again.
 * There is no message that gives the display back either: host text stays
 * until the unit next draws its own.
 *
 * The jog wheel sends two messages per scan event (64, then 64 + n), which the
 * offset decoder reads as one step per second message. How many scan events
 * make a detent is not known.
 *
 * Pad lights: the message is accepted in every mode, but the unit repaints a
 * pad when it is pressed or released, when a matching note arrives and on a
 * memory or mode change, so a color lasts until then. The LED driver counts
 * only the low 5 bits of a channel (32 wraps to 0), hence the scaling 0..127
 * to 0..31.
 *
 * Left out because the sources do not settle them: a value bar (the mkII has no
 * documented one, so cs_value shows text), the lamps of the Category and Preset
 * buttons and of the mode buttons, and the two further banks of encoders and
 * faders (CC 35-43 and 67-70, 87-90, 92): the unit selects them with Part
 * 1, Part 2 and Live, but the channel and state they use on the wire are not
 * recorded. */

#include "cs_internal.h"

/* Pads are channel 10 (index 9) in every mode; the controls are channel 1. */
static const cs_control controls[] = {
    {0x90, 9, 36, 16, CS_EV_PAD, 0, 0},
    /* The jog wheel turns and clicks on one CC pair while the Category lamp is
     * lit and on another while the Preset lamp is lit; both are the encoder. */
    {0xB0, 0, 112, 1, CS_EV_ENCODER, 0, 0},
    {0xB0, 0, 113, 1, CS_EV_CLICK, 0, 0},
    {0xB0, 0, 114, 1, CS_EV_ENCODER, 0, 0},
    {0xB0, 0, 115, 1, CS_EV_CLICK, 0, 0},
    {0xB0, 0, 116, 1, CS_EV_BUTTON, CS_BTN_CATEGORY, 0},
    {0xB0, 0, 117, 1, CS_EV_BUTTON, CS_BTN_PRESET, 0},
    /* Left and right arrow (CC 28 left, 29 right; the naming is a medium-confidence
     * reading of the observed button events) */
    {0xB0, 0, 28, 1, CS_EV_BUTTON, CS_BTN_PREV, 0},
    {0xB0, 0, 29, 1, CS_EV_BUTTON, CS_BTN_NEXT, 0},
    /* The three Control buttons: Part 1, Part 2 and Live in the manual (3.2 p.26),
     * Next, Prev and Bank in a DAW. */
    {0xB0, 0, 22, 1, CS_EV_BUTTON, CS_BTN_PART1, 0},
    {0xB0, 0, 23, 1, CS_EV_BUTTON, CS_BTN_PART2, 0},
    {0xB0, 0, 24, 1, CS_EV_BUTTON, CS_BTN_LIVE, 0},
    /* The nine select buttons share CC 30: value = ((n - 1) << 1) | pressed. The
     * order of n over the buttons (n = 1 Piano .. 9 Multi, manual 3.3.1) is a
     * medium-to-high confidence reading. */
    {0xB0, 0, 30, 9, CS_EV_BUTTON, CS_BTN_SELECT1, CS_CF_VALUE_INDEX},
    /* Encoders 1-9, absolute in the factory memory */
    {0xB0, 0, 74, 1, CS_EV_KNOB, 0, 0},
    {0xB0, 0, 71, 1, CS_EV_KNOB, 1, 0},
    {0xB0, 0, 76, 2, CS_EV_KNOB, 2, 0},
    {0xB0, 0, 93, 1, CS_EV_KNOB, 4, 0},
    {0xB0, 0, 18, 2, CS_EV_KNOB, 5, 0},
    {0xB0, 0, 16, 2, CS_EV_KNOB, 7, 0},
    /* Faders 1-9 */
    {0xB0, 0, 73, 1, CS_EV_FADER, 0, 0},
    {0xB0, 0, 75, 1, CS_EV_FADER, 1, 0},
    {0xB0, 0, 79, 1, CS_EV_FADER, 2, 0},
    {0xB0, 0, 72, 1, CS_EV_FADER, 3, 0},
    {0xB0, 0, 80, 4, CS_EV_FADER, 4, 0},
    {0xB0, 0, 85, 1, CS_EV_FADER, 8, 0},
    /* Sent on the keys' cable but not for the keys: two buttons (CC 26 and 27,
     * sent only while an internal flag is clear; the physical buttons are not
     * identified) and the announcement that Analog Lab mode became active.
     * Swallowed so that they are not played as controllers. */
    {0xB0, 0, 26, 2, CS_EV_SWALLOWED, 0, 0},
    {0xB0, 0, 118, 1, CS_EV_SWALLOWED, 0, 0},
};

/* The LCD is 2 x 16 characters. */
#define LINE_MAX 16

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
    /* Padded with spaces: the unit pads a short line itself, but then a line
     * that is sent empty depends on how the unit ends the message. */
    cs_out_ascii(&o, l1, LINE_MAX, LINE_MAX);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x02);
    cs_out_ascii(&o, l2, LINE_MAX, LINE_MAX);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

/* Pads 1-16 are LEDs 0x70-0x7F. The driver has 5 bits per channel and takes
 * the value unshifted, so the 7-bit color is shifted down by 2, as the unit
 * does itself with the value of a mono LED. */
static size_t pad_light(const cs_model *m, int pad, int r, int g, int b, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    cs_out_arturia(&o);
    cs_out_byte(&o, 0x02);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x16);
    cs_out_byte(&o, 0x70 + pad);
    cs_out_byte(&o, r >> 2);
    cs_out_byte(&o, g >> 2);
    cs_out_byte(&o, b >> 2);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

const cs_protocol cs_proto_keylab_mk2 = {
    controls, sizeof controls / sizeof controls[0], CS_ENC_OFFSET,
    /* MIDI, DAW (jack strings; "MIDI 1" and "MIDI 2" on older Linux kernels) */
    {"midi|midi1", "daw|midi2", NULL, NULL},
    {CS_ROLE_KEYS | CS_ROLE_CONTROLS | CS_ROLE_DISPLAY, 0, 0, 0},
    NULL, NULL, text, NULL, pad_light,
};
