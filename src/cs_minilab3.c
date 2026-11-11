/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Arturia MiniLab 3 in its DAW mode (Shift + Pad 3 on the device; a host cannot
 * switch it). The display and lights are Prot42 messages on the "MIDI" port
 * (cable 0), measured on the device. */

#include "cs_internal.h"

static const cs_control controls[] = {
    {0x90, 9, 36, 16, CS_EV_PAD, 0, 0},      /* pads 1-8 bank A, 9-16 bank B */
    {0xB0, 0, 28, 1, CS_EV_ENCODER, 0, 0},
    {0xB0, 0, 118, 1, CS_EV_CLICK, 0, 0},
    {0xB0, 0, 86, 2, CS_EV_KNOB, 0, 0},
    {0xB0, 0, 89, 2, CS_EV_KNOB, 2, 0},
    {0xB0, 0, 110, 2, CS_EV_KNOB, 4, 0},
    {0xB0, 0, 116, 2, CS_EV_KNOB, 6, 0},
    {0xB0, 0, 14, 2, CS_EV_FADER, 0, 0},
    {0xB0, 0, 30, 2, CS_EV_FADER, 2, 0},
};

/* The device takes about 30 characters a line. */
#define LINE_MAX 30

static void command(cs_out *o, int cmd, int sub, int param)
{
    cs_out_arturia(o);
    cs_out_byte(o, cmd);
    cs_out_byte(o, sub);
    cs_out_byte(o, param);
}

static void lines(cs_out *o, const char *l1, const char *l2)
{
    cs_out_byte(o, 0x01);
    cs_out_ascii(o, l1, LINE_MAX, 0);
    cs_out_byte(o, 0x00);
    cs_out_byte(o, 0x02);
    cs_out_ascii(o, l2, LINE_MAX, 0);
    cs_out_byte(o, 0x00);
    cs_out_byte(o, 0xF7);
}

static size_t host(int on, uint8_t *buf, size_t cap)
{
    cs_out o;
    cs_out_init(&o, buf, cap);
    command(&o, 0x02, 0x02, 0x40);
    cs_out_byte(&o, 0x6A);
    cs_out_byte(&o, on ? 0x21 : 0x20);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

static size_t connect(const cs_model *m, uint8_t *buf, size_t cap)
{
    (void)m;
    return host(1, buf, cap);
}

static size_t disconnect(const cs_model *m, uint8_t *buf, size_t cap)
{
    (void)m;
    return host(0, buf, cap);
}

static size_t text(const cs_model *m, const char *l1, const char *l2, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    command(&o, 0x04, 0x02, 0x60);
    lines(&o, l1, l2);
    return cs_out_end(&o);
}

/* The bar pages (modes 3 to 5) are drawn by the unit only when a control
 * on the device moved within the last 500 ms; otherwise the display stays on
 * the program name. So a bar must be sent
 * right after the control event it reflects; a bar for a value that no
 * control just changed (a menu value, say) may not show at all. */
static size_t value(const cs_model *m, cs_bar bar, int v, const char *l1, const char *l2, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    command(&o, 0x04, 0x02, 0x60);
    cs_out_byte(&o, 0x1F);
    cs_out_byte(&o, bar == CS_BAR_FADER ? 0x04 : 0x03);
    cs_out_byte(&o, 0x01);
    cs_out_byte(&o, v);
    cs_out_byte(&o, 0x00);
    cs_out_byte(&o, 0x00);
    lines(&o, l1, l2);
    return cs_out_end(&o);
}

/* Pads 1-8 are LEDs 0x34-0x3B, 9-16 are 0x44-0x4B; a color stays until the
 * device is switched off. */
static size_t pad_light(const cs_model *m, int pad, int r, int g, int b, uint8_t *buf, size_t cap)
{
    cs_out o;
    (void)m;
    cs_out_init(&o, buf, cap);
    command(&o, 0x02, 0x02, 0x16);
    cs_out_byte(&o, pad < 8 ? 0x34 + pad : 0x44 + pad - 8);
    cs_out_byte(&o, r);
    cs_out_byte(&o, g);
    cs_out_byte(&o, b);
    cs_out_byte(&o, 0xF7);
    return cs_out_end(&o);
}

const cs_protocol cs_proto_minilab3 = {
    controls, sizeof controls / sizeof controls[0], CS_ENC_OFFSET_BURST,
    /* MIDI, DIN THRU, MCU/HUI, ALV: the controls and the display on the first */
    {"midi", "dinthru", "mcu/hui", "alv"},
    {CS_ROLE_KEYS | CS_ROLE_CONTROLS | CS_ROLE_DISPLAY, 0, 0, 0},
    connect, disconnect, text, value, pad_light,
};
