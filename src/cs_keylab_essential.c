/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Arturia KeyLab Essential 49/61/88 (first generation, 2017) in its Analog
 * Lab map, the map the unit starts in. Sources:
 * the Arturia KeyLab Essential user manual and a public MiniDexed forum
 * measurement (one owner, MIDI monitor) for the turn CC numbers.
 *
 * Why the Analog Lab map: the unit draws host text in every map (it does
 * not check the map), and in this map everything is on cable 0 ("MIDI"), so
 * the host needs neither the DAW cable (which an emulated unit does
 * not offer) nor a map recall, which would change
 * the map the user is in. Host text stays until the next control action or a
 * recall redraws the display; no message gives the display back, so connect
 * and disconnect send nothing.
 *
 * Controls on channel 1: the jog is offset binary around 64 (65 one step
 * clockwise, 63 counter-clockwise, larger when turned fast), CC 114 while
 * "Preset" is lit and CC 112 while "Category" is lit; a 64 is no step. Its
 * click is CC 115 or 113. Category CC 116, Preset CC 117, Backward CC 28,
 * Forward CC 29, Part 1 / Part 2 / Live CC 22 / 23 / 24. Knobs 1-9 CC 74, 71,
 * 76, 77, 93, 18, 19, 16, 17 and faders 1-9 CC 73, 75, 79, 72, 80, 81, 82, 83,
 * 85 (the first set; Part and Live select the second).
 *
 * Pads 1-8 are notes 36-43 on channel 10 (the manual's pad table), apart from
 * the keys on channel 1. Buttons and the click are momentary: 127 on press, 0
 * on release.
 *
 * Left out: the Windows port names
 * ("MIDIIN2 (Arturia KeyLab Essenti" is cut before the size), the button lamps
 * and a value bar.
 *
 * Identity (documentation only): the reply carries the member bytes 05 52 (49
 * keys), 05 54 (61) and 05 58 (88) after the family 02 00. */

#include "cs_internal.h"

static const cs_control controls[] = {
    {0xB0, 0, 112, 1, CS_EV_ENCODER, 0, 0},
    {0xB0, 0, 113, 1, CS_EV_CLICK, 0, 0},
    {0xB0, 0, 114, 1, CS_EV_ENCODER, 0, 0},
    {0xB0, 0, 115, 1, CS_EV_CLICK, 0, 0},
    {0xB0, 0, 116, 1, CS_EV_BUTTON, CS_BTN_CATEGORY, 0},
    {0xB0, 0, 117, 1, CS_EV_BUTTON, CS_BTN_PRESET, 0},
    {0xB0, 0, 28, 1, CS_EV_BUTTON, CS_BTN_PREV, 0},
    {0xB0, 0, 29, 1, CS_EV_BUTTON, CS_BTN_NEXT, 0},
    {0xB0, 0, 22, 1, CS_EV_BUTTON, CS_BTN_PART1, 0},
    {0xB0, 0, 23, 1, CS_EV_BUTTON, CS_BTN_PART2, 0},
    {0xB0, 0, 24, 1, CS_EV_BUTTON, CS_BTN_LIVE, 0},
    {0xB0, 0, 74, 1, CS_EV_KNOB, 0, 0},
    {0xB0, 0, 71, 1, CS_EV_KNOB, 1, 0},
    {0xB0, 0, 76, 2, CS_EV_KNOB, 2, 0},
    {0xB0, 0, 93, 1, CS_EV_KNOB, 4, 0},
    {0xB0, 0, 18, 2, CS_EV_KNOB, 5, 0},
    {0xB0, 0, 16, 2, CS_EV_KNOB, 7, 0},
    {0xB0, 0, 73, 1, CS_EV_FADER, 0, 0},
    {0xB0, 0, 75, 1, CS_EV_FADER, 1, 0},
    {0xB0, 0, 79, 1, CS_EV_FADER, 2, 0},
    {0xB0, 0, 72, 1, CS_EV_FADER, 3, 0},
    {0xB0, 0, 80, 4, CS_EV_FADER, 4, 0},
    {0xB0, 0, 85, 1, CS_EV_FADER, 8, 0},
    {0x90, 9, 36, 8, CS_EV_PAD, 0, 0},
};

/* The LCD is 2 x 16 characters. */
#define LINE_MAX 16

/* Padded: whether the unit pads text from SysEx itself was not determined. */
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

/* Pads 1-8 are the LEDs 0x70-0x77 with 5 bits a channel, so the 7-bit color is
 * shifted down by 2. */
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

const cs_protocol cs_proto_keylab_essential = {
    controls, sizeof controls / sizeof controls[0], CS_ENC_OFFSET,
    /* MIDI, DAW. "mid" is how a 31-character port name ends on Linux; the
     * macOS names end in "In" or "Out". The DAW cable carries only the DAW
     * map's messages, which this map does not use. */
    {"midi|midi1|mid|midiin|midiout", "daw|midi2|dawin|dawout", NULL, NULL},
    {CS_ROLE_KEYS | CS_ROLE_CONTROLS | CS_ROLE_DISPLAY, 0, 0, 0},
    NULL, NULL, text, NULL, pad_light,
};
