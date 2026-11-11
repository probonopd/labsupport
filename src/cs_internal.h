/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef CS_INTERNAL_H
#define CS_INTERNAL_H

#include "labsupport.h"

/* One control: a CC or note number on a channel (-1 any) and the event it is. */
typedef struct cs_control {
    uint8_t type;      /* 0xB0 CC, 0x90 note (note off and note on with 0 count as release) */
    int8_t channel;
    uint8_t number;
    uint8_t count;     /* consecutive numbers mapping to consecutive indexes */
    uint8_t kind;      /* cs_event_kind */
    uint8_t index;
    uint8_t flags;     /* CS_CF_ */
} cs_control;

enum {
    /* One CC (NUMBER) carries COUNT buttons in its value: bit 0 is pressed, the
     * bits above it are the button's offset from INDEX (a device that sends all
     * its row of buttons on one CC). A value beyond COUNT buttons is no match. */
    CS_CF_VALUE_INDEX = 1,
    /* A KNOB that sends steps around 64 instead of a position: the event's delta
     * is value - 64, and 64 itself is swallowed. */
    CS_CF_RELATIVE = 2
};

#define CS_MAX_PORTS 4

/* How the main encoder encodes a turn. */
typedef enum {
    CS_ENC_OFFSET_BURST = 1, /* around 64, several messages per detent: steps by accumulated movement */
    CS_ENC_OFFSET            /* around 64, one message per step: value - 64 */
} cs_encoding;

typedef struct cs_protocol {
    const cs_control *controls;
    size_t ncontrols;
    cs_encoding encoding;
    /* per port (USB cable) of the device: the suffixes its name may have,
     * lower case without spaces and separated by '|', and its CS_ROLE_ flags */
    const char *port_suffixes[CS_MAX_PORTS];
    int port_roles[CS_MAX_PORTS];
    size_t (*connect)(const cs_model *m, uint8_t *buf, size_t cap);
    size_t (*disconnect)(const cs_model *m, uint8_t *buf, size_t cap);
    size_t (*text)(const cs_model *m, const char *l1, const char *l2, uint8_t *buf, size_t cap);
    size_t (*value)(const cs_model *m, cs_bar bar, int v, const char *l1, const char *l2, uint8_t *buf, size_t cap);
    size_t (*pad_light)(const cs_model *m, int pad, int r, int g, int b, uint8_t *buf, size_t cap);
} cs_protocol;

const cs_protocol *cs_protocol_of(cs_family f);

/* Building system exclusive messages into a caller buffer: once full, every
 * further write is dropped and cs_out_end returns 0. */
typedef struct cs_out {
    uint8_t *buf;
    size_t cap, len;
    int overflow;
} cs_out;

void cs_out_init(cs_out *o, uint8_t *buf, size_t cap);
void cs_out_byte(cs_out *o, int b);
void cs_out_bytes(cs_out *o, const uint8_t *b, size_t n);
/* F0 00 20 6B 7F 42: Arturia, broadcast device, the controller family. */
void cs_out_arturia(cs_out *o);
/* At most MAX characters of S, each outside 0x20..0x7E as '?', padded with
 * spaces to PAD (0 = no padding); the bytes of a character outside ASCII are
 * one character, so a name keeps as much as the device can show. */
void cs_out_ascii(cs_out *o, const char *s, int max, int pad);
size_t cs_out_end(cs_out *o);

#endif
