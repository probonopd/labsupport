/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * labsupport: MIDI keyboard controllers with a display, used as the panel of an
 * application. A model table says which device a port belongs to and what it
 * has; a decoder turns its channel messages into control events; encoders write
 * the system exclusive messages for its display and its lights.
 *
 * Plain C99, no allocation, no I/O, no clock: the caller owns the ports and
 * passes the time in, so the same code runs in a desktop app, a plug-in, a
 * command line tool and on a microcontroller. Each src/cs_<family>.c says
 * what is known about its device and why its mode was chosen.
 */

#ifndef LABSUPPORT_H
#define LABSUPPORT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The protocol generation of a device. Models of one family differ only in
 * size (keys, pads, faders), never in their messages. */
typedef enum {
    CS_FAMILY_MINILAB3 = 1,
    CS_FAMILY_KEYLAB,              /* KeyLab 25/49/61/88, 2014 */
    CS_FAMILY_KEYLAB_ESSENTIAL,    /* KeyLab Essential 49/61/88, 2017 */
    CS_FAMILY_KEYLAB_MK2,          /* KeyLab 49/61/88 mkII, 2019 */
    CS_FAMILY_KEYLAB_ESSENTIAL_MK3, /* KeyLab Essential 49/61/88 mk3, 2023 */
    CS_FAMILY_KEYLAB_MK3           /* KeyLab 49/61/88 mk3, 2024 */
} cs_family;

/* What a port of a device carries for the application (bit flags). */
enum {
    CS_ROLE_KEYS = 1,      /* the keys, wheels and pedals: play them */
    CS_ROLE_CONTROLS = 2,  /* the controls: decode them with cs_decode */
    CS_ROLE_DISPLAY = 4    /* takes the messages for the display and the lights */
};

typedef struct cs_model {
    const char *name;        /* "KeyLab mkII 61" */
    cs_family family;
    uint16_t usb_vendor;
    uint16_t usb_products[2]; /* 0 = unknown or unused */
    const char *product;     /* USB product string as the host shows it */
    int pads, knobs, faders;
    int lines;               /* text lines the host can write */
    int line_chars;          /* characters of a line that are shown */
    int rgb_pads;            /* pads take a color, not only on/off */
    int bars;                /* the display can show a value bar */
} cs_model;

/* The models, ending with an entry whose name is NULL. */
extern const cs_model cs_models[];

/* The model whose USB product or MIDI port name this is (a port name is the
 * product string plus a suffix such as " MIDI", " DAW" or " MIDI 1"), or NULL.
 * Case and spaces are ignored, as hosts and kernels spell names differently. */
const cs_model *cs_model_by_name(const char *name);

/* The model with this USB vendor and product ID, or NULL. */
const cs_model *cs_model_by_usb(uint16_t vendor, uint16_t product);

/* The roles of the port of model M with this name (the device's name plus a
 * suffix such as " MIDI" or " DAW"); 0 for a port the application does not
 * use or a name of another device. A name without a suffix is the first port. */
int cs_port_roles(const cs_model *m, const char *port_name);

/* The roles of port INDEX (0.., the USB cable number), for hosts that do not
 * know the port names. */
int cs_port_roles_at(const cs_model *m, int index);

/* ------------------------------------------------------------- input ---- */

typedef enum {
    CS_EV_NONE = 0,     /* not a control: play it (keys, wheels, pedals) */
    CS_EV_SWALLOWED,    /* a control message that makes no event of its own */
    CS_EV_PAD,          /* index 0.., value velocity, 0 = released */
    CS_EV_KNOB,         /* index 0.., value 0..127; delta is 0 for an absolute knob, the
                           signed step (value - 64, never 0) for a relative one */
    CS_EV_FADER,        /* index 0.., value 0..127 */
    CS_EV_ENCODER,      /* the main encoder: delta steps, negative counter-clockwise */
    CS_EV_CLICK,        /* the main encoder pressed: value 1 down, 0 up */
    CS_EV_BUTTON        /* index a cs_button, value 1 down, 0 up */
} cs_event_kind;

/* Buttons by what they are for, so that an application need not know which
 * device has which. */
typedef enum {
    CS_BTN_PREV = 1, CS_BTN_NEXT,       /* the arrow buttons beside the display */
    CS_BTN_CATEGORY, CS_BTN_PRESET,     /* browse by category / by preset */
    CS_BTN_PART1, CS_BTN_PART2, CS_BTN_LIVE,
    CS_BTN_SOFT1, CS_BTN_SOFT2, CS_BTN_SOFT3, CS_BTN_SOFT4, /* below the display */
    CS_BTN_SOFT5, CS_BTN_SOFT6, CS_BTN_SOFT7, CS_BTN_SOFT8,
    CS_BTN_SELECT1, CS_BTN_SELECT2, CS_BTN_SELECT3, CS_BTN_SELECT4, CS_BTN_SELECT5,
    CS_BTN_SELECT6, CS_BTN_SELECT7, CS_BTN_SELECT8, CS_BTN_SELECT9,
    CS_BTN_PLAY, CS_BTN_STOP, CS_BTN_RECORD, CS_BTN_LOOP, CS_BTN_REWIND, CS_BTN_FORWARD,
    CS_BTN_BACK, CS_BTN_OK,
    CS_BTN_COUNT
} cs_button;

typedef struct cs_event {
    cs_event_kind kind;
    int index;
    int value;
    int delta;
} cs_event;

typedef struct cs_decoder {
    const cs_model *model;
    int enc_dir;
    int64_t enc_ms;
    int enc_ticks;
} cs_decoder;

void cs_decoder_init(cs_decoder *d, const cs_model *m);

/* The control a channel message from the control port stands for. */
cs_event cs_decode(cs_decoder *d, int status, int d1, int d2, int64_t now_ms);

/* ------------------------------------------------------------ output ---- */

/* Every writer below puts one or more complete system exclusive messages into
 * BUF and returns their length, or 0 when CAP is too small or the device has
 * no such thing. CS_SYSEX_MAX is always enough. */
#define CS_SYSEX_MAX 512

/* Take the display and the lights for the application; send again whenever
 * the device may have been switched to another mode by hand. */
size_t cs_connect(const cs_model *m, uint8_t *buf, size_t cap);

/* Blank the display and give the display and the lights back to the device;
 * send it when the application ends, so that none of its text stays behind. */
size_t cs_disconnect(const cs_model *m, uint8_t *buf, size_t cap);

/* Two lines of text; characters outside 7-bit ASCII become '?'. */
size_t cs_text(const cs_model *m, const char *line1, const char *line2, uint8_t *buf, size_t cap);

typedef enum { CS_BAR_KNOB = 1, CS_BAR_FADER } cs_bar;

/* Two lines and a bar for VALUE 0..127; text only where there are no bars. */
size_t cs_value(const cs_model *m, cs_bar bar, int value, const char *line1, const char *line2,
                uint8_t *buf, size_t cap);

/* The light of pad PAD (0..) in 0..127 per channel; on/off devices light any
 * nonzero color. */
size_t cs_pad_light(const cs_model *m, int pad, int r, int g, int b, uint8_t *buf, size_t cap);

#ifdef __cplusplus
}
#endif

#endif
