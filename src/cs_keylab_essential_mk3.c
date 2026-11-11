/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Arturia KeyLab Essential 49/61/88 mk3 in its DAW program. Sources:
 * the Arturia KeyLab Essential mk3 user manual, public DAW scripts and hardware
 * reports from owners.
 *
 * Why the DAW program: the unit shows the host's display and lights only
 * while the DAW program is active and "DAW connected" (state 0x5A) is set; in
 * a User program neither works, and the control numbers of the Arturia program
 * have no proven relative encoding for the main encoder. The DAW program has
 * fixed, hardware-confirmed numbers. connect therefore sets the state and
 * selects the DAW program (item 2 of the program record, value 1; the unit
 * echoes it). Everything is on cable 0 ("MIDI"); the unit drops Prot42 on
 * the DINTHRU and MCU/HUI cables and the ALV cable is Analog Lab's, so only
 * the first is used.
 *
 * Disconnect clears the state (02 0F 40 5A 00, a valid write): the host's
 * display and lights are dropped again and the unit shows its own screen
 * when the host stops drawing. There is no "give the display back" message
 * and the program is left as it is.
 *
 * The main encoder (CC 116, click CC 117) is binary offset around 64 and, in
 * the DAW program, one message per step (the Arturia program sends two, a
 * neutral 64 first). A step is
 * +-1 per quadrature state change; the size of a step at speed is not known.
 *
 * Left out: the Bank and Part buttons (swallowed, no app role), the footer and
 * header areas of the display, the icons and the lamps of the buttons. Any
 * program message repaints the four context buttons dark, so send connect
 * only when it is needed. */

#include "cs_internal.h"

/* Pads are channel 11 (index 10) in the DAW program; the controls are channel 1.
 * Pad 1-8 are bank A, 9-16 bank B; within a bank the top row (pads 1-4) has
 * the higher notes. */
static const cs_control controls[] = {
    {0x90, 10, 40, 4, CS_EV_PAD, 0, 0},
    {0x90, 10, 36, 4, CS_EV_PAD, 4, 0},
    {0x90, 10, 48, 4, CS_EV_PAD, 8, 0},
    {0x90, 10, 44, 4, CS_EV_PAD, 12, 0},
    {0xB0, 0, 116, 1, CS_EV_ENCODER, 0, 0},
    {0xB0, 0, 117, 1, CS_EV_CLICK, 0, 0},
    /* The four buttons under the display */
    {0xB0, 0, 44, 4, CS_EV_BUTTON, CS_BTN_SOFT1, 0},
    {0xB0, 0, 20, 1, CS_EV_BUTTON, CS_BTN_STOP, 0},
    {0xB0, 0, 21, 1, CS_EV_BUTTON, CS_BTN_PLAY, 0},
    {0xB0, 0, 22, 1, CS_EV_BUTTON, CS_BTN_RECORD, 0},
    {0xB0, 0, 24, 1, CS_EV_BUTTON, CS_BTN_LOOP, 0},
    {0xB0, 0, 25, 1, CS_EV_BUTTON, CS_BTN_REWIND, 0},
    {0xB0, 0, 26, 1, CS_EV_BUTTON, CS_BTN_FORWARD, 0},
    {0xB0, 0, 96, 9, CS_EV_KNOB, 0, 0},
    {0xB0, 0, 105, 9, CS_EV_FADER, 0, 0},
    /* Buttons without a role here, on the keys' cable: not to be played */
    {0xB0, 0, 23, 1, CS_EV_SWALLOWED, 0, 0},   /* Tap */
    {0xB0, 0, 27, 1, CS_EV_SWALLOWED, 0, 0},   /* Metronome */
    {0xB0, 0, 40, 4, CS_EV_SWALLOWED, 0, 0},   /* Save, Quantize, Undo, Redo */
    {0xB0, 0, 118, 2, CS_EV_SWALLOWED, 0, 0},  /* Bank, Part */
};

/* 18 characters a line are shown (hardware report); the unit has no limit of
 * its own beyond the 100 bytes of a message. */
#define LINE_MAX 18
/* The knob and fader widgets clip their two texts to 12. */
#define WIDGET_MAX 12

static void head(cs_out *o, int cmd, int what)
{
    cs_out_arturia(o);
    cs_out_byte(o, cmd);
    cs_out_byte(o, 0x01); /* patch 1: the DAW program */
    cs_out_byte(o, what);
}

static void state(cs_out *o, int on)
{
    cs_out_arturia(o);
    cs_out_byte(o, 0x02);
    cs_out_byte(o, 0x0F);
    cs_out_byte(o, 0x40);
    cs_out_byte(o, 0x5A); /* DAW connected */
    cs_out_byte(o, on ? 0x01 : 0x00);
    cs_out_byte(o, 0xF7);
}

static size_t connect(const cs_model *m, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    state(&o, 1);
    /* Active program (item 2) = 1, the DAW program */
    cs_out_arturia(&o);
    cs_out_byte(&o, 0x21);
    cs_out_byte(&o, 0x11);
    cs_out_byte(&o, 0x40);
    cs_out_byte(&o, 0x02);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x01);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

static size_t disconnect(const cs_model *m, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    state(&o, 0);
    return cs_out_end(&o);
}

/* Screen 0x12: two lines; field 1 is line 1, field 2 line 2, then a flag byte
 * (0: the screen stays). */
static size_t text(const cs_model *m, const char *l1, const char *l2, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    head(&o, 0x04, 0x60);
    cs_out_byte(&o, 0x12);
    cs_out_byte(&o, 0x01);
    cs_out_ascii(&o, l1, LINE_MAX, 0);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x02);
    cs_out_ascii(&o, l2, LINE_MAX, 0);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

/* Screen 0x14 (knob) or 0x15 (fader): the two texts and the position, shown as
 * a transient screen (flag 1), as the Bitwig extension sends it. */
static size_t value(const cs_model *m, cs_bar bar, int v, const char *l1, const char *l2, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    head(&o, 0x04, 0x60);
    cs_out_byte(&o, bar == CS_BAR_FADER ? 0x15 : 0x14);
    cs_out_byte(&o, 0x01);
    cs_out_ascii(&o, l1, WIDGET_MAX, 0);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x02);
    cs_out_ascii(&o, l2, WIDGET_MAX, 0);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x03);
    cs_out_byte(&o, v);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x01);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

/* The pad lights are LEDs 0x1C-0x23 for pads 1-8 and 0x24-0x2B for pads 9-16.
 * Those of the inactive bank do not light, and the unit takes 7 bits a channel
 * (it doubles them). */
static size_t pad_light(const cs_model *m, int pad, int r, int g, int b, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    head(&o, 0x04, 0x16);
    cs_out_byte(&o, 0x1C + pad);
    cs_out_byte(&o, r);
    cs_out_byte(&o, g);
    cs_out_byte(&o, b);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

const cs_protocol cs_proto_keylab_essential_mk3 = {
    controls, sizeof controls / sizeof controls[0], CS_ENC_OFFSET,
    /* MIDI, DINTHRU, MCU/HUI, ALV */
    {"midi", "dinthru", "mcu/hui", "alv"},
    {CS_ROLE_KEYS | CS_ROLE_CONTROLS | CS_ROLE_DISPLAY, 0, 0, 0},
    connect, disconnect, text, value, pad_light,
};
