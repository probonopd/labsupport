/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Arturia KeyLab 49/61/88 mk3 in its DAW program. Sources:
 * the Arturia KeyLab mk3 user manual, public DAW scripts and a hardware
 * report from an owner for the control numbers.
 *
 * Why the DAW program: its control numbers are the only ones documented, and
 * the pads and buttons of the other programs are not. The unit has two cables:
 * 0 "MIDI" carries the keys, wheels and pedals, 1 "DAW" the controls, the
 * display and the lights. The unit accepts display and lights on either
 * cable; the DAW one is where the scripts and the hardware report use them.
 * connect sets the connected flag (00 02 05 01) and then selects the DAW
 * program (00 02 07 01: program 1). That second message is not used by
 * the public scripts; they leave the choice of program to the user, so if
 * the unit ignores it, the user has to pick DAW with the Prog button.
 *
 * Disconnect clears the same flag (00 02 05 00): the unit accepts it, and
 * no message gives the display back, so the unit shows its own screen again
 * once the host stops drawing (what it draws right after is not traced).
 *
 * Soft buttons: the eight context buttons send CC 45-52 in order. Soft 1-4
 * (CC 45-48) are the top row and soft 5-8 (CC 49-52) the bottom row, left to
 * right (the unit's LED and display id order plus the
 * manual and one hardware experiment; the CC-to-button link itself is visible
 * only in the hardware experiment, so it is medium confidence).
 *
 * Left out: the touch sensors of the encoders and faders (the numbers of
 * fader 5 and 9 conflict between sources, and no table of
 * them is known), context button labels and icons (the display id order is inverted
 * against the CC order, so a label writer needs its own mapping), the Arturia
 * pad banks, and the lamps of the buttons. Text is white because the API has
 * no color for it. The encoders send steps around 64, so they are relative
 * knobs (the wire coding of the other relative modes is in the manual only). */

#include "cs_internal.h"

/* The controls are channel 1 and the 12 DAW pads channel 10 (index 9). */
static const cs_control controls[] = {
    {0x90, 9, 0, 12, CS_EV_PAD, 0, 0},
    {0xB0, 0, 116, 1, CS_EV_ENCODER, 0, 0},
    {0xB0, 0, 117, 1, CS_EV_CLICK, 0, 0},
    /* Encoders 1-9 are CC 91, 92, 94, 95, 96, 97, 102, 103, 104 */
    {0xB0, 0, 91, 2, CS_EV_KNOB, 0, CS_CF_RELATIVE},
    {0xB0, 0, 94, 4, CS_EV_KNOB, 2, CS_CF_RELATIVE},
    {0xB0, 0, 102, 3, CS_EV_KNOB, 6, CS_CF_RELATIVE},
    {0xB0, 0, 105, 9, CS_EV_FADER, 0, 0},
    {0xB0, 0, 20, 1, CS_EV_BUTTON, CS_BTN_STOP, 0},
    {0xB0, 0, 21, 1, CS_EV_BUTTON, CS_BTN_PLAY, 0},
    {0xB0, 0, 22, 1, CS_EV_BUTTON, CS_BTN_RECORD, 0},
    {0xB0, 0, 24, 1, CS_EV_BUTTON, CS_BTN_LOOP, 0},
    {0xB0, 0, 25, 1, CS_EV_BUTTON, CS_BTN_REWIND, 0},
    {0xB0, 0, 26, 1, CS_EV_BUTTON, CS_BTN_FORWARD, 0},
    {0xB0, 0, 40, 1, CS_EV_BUTTON, CS_BTN_BACK, 0},
    {0xB0, 0, 45, 8, CS_EV_BUTTON, CS_BTN_SOFT1, 0},
};

/* No fixed limit in the unit; 18 characters are the practical rule (scripts). */
#define LINE_MAX 18

static void head(cs_out *o, int cmd, int patch, int what)
{
    cs_out_arturia(o);
    cs_out_byte(o, cmd);
    cs_out_byte(o, patch);
    cs_out_byte(o, what);
}

static void connected(cs_out *o, int on)
{
    head(o, 0x00, 0x02, 0x05);
    cs_out_byte(o, on ? 0x01 : 0x00);
    cs_out_byte(o, 0xF7);
}

static size_t connect(const cs_model *m, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    connected(&o, 1);
    head(&o, 0x00, 0x02, 0x07);
    cs_out_byte(&o, 0x01);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

static size_t disconnect(const cs_model *m, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    connected(&o, 0);
    return cs_out_end(&o);
}

/* A text field is its index, the characters and a zero; a color field its index,
 * r, g, b and a zero; a number field its index, the value and a zero. */
static void field_text(cs_out *o, int index, const char *s)
{
    cs_out_byte(o, index);
    cs_out_ascii(o, s, LINE_MAX, 0);
    cs_out_byte(o, 0x00);
}

static void field_u7(cs_out *o, int index, int v)
{
    cs_out_byte(o, index);
    cs_out_byte(o, v);
    cs_out_byte(o, 0x00);
}

static void field_white(cs_out *o, int index)
{
    cs_out_byte(o, index);
    cs_out_byte(o, 0x7F);
    cs_out_byte(o, 0x7F);
    cs_out_byte(o, 0x7F);
    cs_out_byte(o, 0x00);
}

/* Screen 0x0C: two lines, each a text, a color mode (1: the color given) and a
 * color. */
static size_t text(const cs_model *m, const char *l1, const char *l2, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    head(&o, 0x00, 0x02, 0x04);
    cs_out_byte(&o, 0x0C);
    field_text(&o, 0, l1);
    field_u7(&o, 1, 1);
    field_white(&o, 2);
    field_text(&o, 3, l2);
    field_u7(&o, 4, 1);
    field_white(&o, 5);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

/* The knob popup 0x1C (an arc) or the fader popup 0x1D (a vertical track with a
 * thumb): the three pages have identical fields, the value text, the name, the
 * position, the color mode and the color . 0x1E draws a
 * bar without a thumb, so it is not a fader. The popup goes away by itself
 * after about a second. */
static size_t value(const cs_model *m, cs_bar bar, int v, const char *l1, const char *l2, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    head(&o, 0x00, 0x02, 0x04);
    cs_out_byte(&o, bar == CS_BAR_FADER ? 0x1D : 0x1C);
    field_text(&o, 0, l2);
    field_text(&o, 1, l1);
    field_u7(&o, 2, v);
    field_u7(&o, 3, 1);
    field_white(&o, 4);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

/* The 12 DAW pads are LEDs 0x52-0x5D. The message is 15 bytes long: the last
 * data byte is a state (0 to 2; what 1 and 2 do is not known), 0 here. */
static size_t pad_light(const cs_model *m, int pad, int r, int g, int b, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    head(&o, 0x04, 0x02, 0x03);
    cs_out_byte(&o, 0x52 + pad);
    cs_out_byte(&o, r);
    cs_out_byte(&o, g);
    cs_out_byte(&o, b);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

const cs_protocol cs_proto_keylab_mk3 = {
    controls, sizeof controls / sizeof controls[0], CS_ENC_OFFSET,
    /* MIDI, DAW */
    {"midi", "daw", NULL, NULL},
    {CS_ROLE_KEYS, CS_ROLE_CONTROLS | CS_ROLE_DISPLAY, 0, 0},
    connect, disconnect, text, value, pad_light,
};
