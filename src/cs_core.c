/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "cs_internal.h"

#include <string.h>

/* Names are compared lower case and without spaces: ALSA, Android and Windows
 * spell one device "Minilab3 MIDI", "MiniLab 3" and "Minilab3". */
static size_t normalize(const char *s, char *out, size_t cap)
{
    size_t n = 0;
    for (; s != NULL && *s != '\0' && n + 1 < cap; s++) {
        char c = *s;
        if (c == ' ') {
            continue;
        }
        out[n++] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
    }
    out[n] = '\0';
    return n;
}

/* Where the product string ends in NAME, or -1. The longest product wins, so
 * that "KeyLab 61 mk3" is not taken for "KeyLab 61". */
static const cs_model *find(const char *name, size_t *product_end)
{
    char n[128], p[128];
    const cs_model *best = NULL;
    size_t best_len = 0;
    normalize(name, n, sizeof n);
    for (const cs_model *m = cs_models; m->name != NULL; m++) {
        size_t len = normalize(m->product, p, sizeof p);
        const char *at = len > 0 ? strstr(n, p) : NULL;
        if (at != NULL && len > best_len) {
            best = m;
            best_len = len;
            *product_end = (size_t)(at - n) + len;
        }
    }
    return best;
}

const cs_model *cs_model_by_name(const char *name)
{
    size_t end;
    return find(name, &end);
}

const cs_model *cs_model_by_usb(uint16_t vendor, uint16_t product)
{
    for (const cs_model *m = cs_models; m->name != NULL; m++) {
        if (m->usb_vendor == vendor && product != 0
            && (m->usb_products[0] == product || m->usb_products[1] == product)) {
            return m;
        }
    }
    return NULL;
}

/* True when WORD is one of the '|' separated words of LIST. */
static int listed(const char *list, const char *word)
{
    size_t wl = strlen(word);
    while (list != NULL && *list != '\0') {
        const char *bar = strchr(list, '|');
        size_t l = bar != NULL ? (size_t)(bar - list) : strlen(list);
        if (l == wl && strncmp(list, word, l) == 0) {
            return 1;
        }
        if (bar == NULL) {
            break;
        }
        list = bar + 1;
    }
    return 0;
}

int cs_port_roles(const cs_model *m, const char *port_name)
{
    char n[128];
    size_t end;
    if (m == NULL || find(port_name, &end) != m) {
        return 0;
    }
    normalize(port_name, n, sizeof n);
    const char *suffix = n + end;
    const cs_protocol *p = cs_protocol_of(m->family);
    if (*suffix == '\0') {
        return p->port_roles[0];
    }
    for (int i = 0; i < CS_MAX_PORTS; i++) {
        if (listed(p->port_suffixes[i], suffix)) {
            return p->port_roles[i];
        }
    }
    return 0;
}

int cs_port_roles_at(const cs_model *m, int index)
{
    if (m == NULL || index < 0 || index >= CS_MAX_PORTS) {
        return 0;
    }
    return cs_protocol_of(m->family)->port_roles[index];
}

/* ------------------------------------------------------------- input ---- */

/* Messages of the encoder closer than this belong to one turn (MiniLab 3:
 * measured 2 to 5 per detent). */
#define ENC_BURST_MS 70
/* Within one turn, this much movement (the sum of the offsets from 64, which
 * grow with the speed) is one more step; 5 keeps a slow detent at one step. */
#define ENC_TICKS_PER_STEP 5

void cs_decoder_init(cs_decoder *d, const cs_model *m)
{
    memset(d, 0, sizeof *d);
    d->model = m;
    d->enc_ms = -1000000000LL;
}

static cs_event event(cs_event_kind kind, int index, int value, int delta)
{
    cs_event e;
    e.kind = kind;
    e.index = index;
    e.value = value;
    e.delta = delta;
    return e;
}

static cs_event encoder(cs_decoder *d, cs_encoding enc, int v, int64_t now)
{
    if (v == 64) {
        return event(CS_EV_SWALLOWED, 0, v, 0);
    }
    if (enc == CS_ENC_OFFSET) {
        return event(CS_EV_ENCODER, 0, v, v - 64);
    }
    /* The first message of a turn is a step, then one more per
     * ENC_TICKS_PER_STEP of movement; a turn ends when it pauses or turns back. */
    int dir = v > 64 ? 1 : -1;
    int same_turn = dir == d->enc_dir && now - d->enc_ms < ENC_BURST_MS;
    d->enc_ms = now;
    if (!same_turn) {
        d->enc_dir = dir;
        d->enc_ticks = 0;
        return event(CS_EV_ENCODER, 0, v, dir);
    }
    d->enc_ticks += v > 64 ? v - 64 : 64 - v;
    int steps = d->enc_ticks / ENC_TICKS_PER_STEP;
    if (steps == 0) {
        return event(CS_EV_SWALLOWED, 0, v, 0);
    }
    d->enc_ticks %= ENC_TICKS_PER_STEP;
    return event(CS_EV_ENCODER, 0, v, dir * steps);
}

cs_event cs_decode(cs_decoder *d, int status, int d1, int d2, int64_t now_ms)
{
    const cs_protocol *p = cs_protocol_of(d->model->family);
    int type = status & 0xF0, ch = status & 0x0F;
    int released = type == 0x80 || (type == 0x90 && d2 == 0);
    if (type == 0x80) {
        type = 0x90;
    }
    for (size_t i = 0; i < p->ncontrols; i++) {
        const cs_control *c = &p->controls[i];
        int by_value = (c->flags & CS_CF_VALUE_INDEX) != 0;
        int span = by_value ? 1 : c->count;
        if (c->type != type || (c->channel >= 0 && c->channel != ch)
            || d1 < c->number || d1 >= c->number + span || (by_value && (d2 >> 1) >= c->count)) {
            continue;
        }
        int index = c->index + (by_value ? d2 >> 1 : d1 - c->number);
        if (by_value) {
            return event((cs_event_kind)c->kind, index, d2 & 1, 0);
        }
        switch ((cs_event_kind)c->kind) {
        case CS_EV_ENCODER:
            return encoder(d, p->encoding, d2, now_ms);
        case CS_EV_PAD:
            return event(CS_EV_PAD, index, released ? 0 : d2, 0);
        case CS_EV_CLICK:
        case CS_EV_BUTTON:
            return event((cs_event_kind)c->kind, index, (type == 0x90 ? !released : d2 != 0) ? 1 : 0, 0);
        case CS_EV_SWALLOWED:
            return event(CS_EV_SWALLOWED, 0, d2, 0);
        case CS_EV_KNOB:
            if (c->flags & CS_CF_RELATIVE) {
                return d2 == 64 ? event(CS_EV_SWALLOWED, index, d2, 0) : event(CS_EV_KNOB, index, d2, d2 - 64);
            }
            return event(CS_EV_KNOB, index, d2, 0);
        default:
            return event((cs_event_kind)c->kind, index, d2, 0);
        }
    }
    return event(CS_EV_NONE, 0, 0, 0);
}

/* ------------------------------------------------------------ output ---- */

void cs_out_init(cs_out *o, uint8_t *buf, size_t cap)
{
    o->buf = buf;
    o->cap = cap;
    o->len = 0;
    o->overflow = 0;
}

void cs_out_byte(cs_out *o, int b)
{
    if (o->len < o->cap) {
        o->buf[o->len++] = (uint8_t)b;
    } else {
        o->overflow = 1;
    }
}

void cs_out_bytes(cs_out *o, const uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        cs_out_byte(o, b[i]);
    }
}

void cs_out_arturia(cs_out *o)
{
    static const uint8_t header[] = {0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42};
    cs_out_bytes(o, header, sizeof header);
}

void cs_out_ascii(cs_out *o, const char *s, int max, int pad)
{
    int shown = 0;
    for (const unsigned char *c = (const unsigned char *)(s != NULL ? s : ""); *c != '\0' && shown < max; c++) {
        if ((*c & 0xC0) == 0x80) {
            continue; /* a continuation byte of the character just written */
        }
        cs_out_byte(o, *c >= 0x20 && *c < 0x7F ? *c : '?');
        shown++;
    }
    for (; shown < pad; shown++) {
        cs_out_byte(o, ' ');
    }
}

size_t cs_out_end(cs_out *o)
{
    return o->overflow ? 0 : o->len;
}

static int clamp7(int v)
{
    return v < 0 ? 0 : v > 127 ? 127 : v;
}

size_t cs_connect(const cs_model *m, uint8_t *buf, size_t cap)
{
    const cs_protocol *p = cs_protocol_of(m->family);
    return p->connect != NULL ? p->connect(m, buf, cap) : 0;
}

/* Most devices have no message that gives the display back, so the
 * application's last text would stay on it after the application is gone: the
 * display is blanked first, then the device's own release (if it has one) lets
 * it draw its own screen again. */
size_t cs_disconnect(const cs_model *m, uint8_t *buf, size_t cap)
{
    const cs_protocol *p = cs_protocol_of(m->family);
    size_t n = p->text != NULL ? p->text(m, "", "", buf, cap) : 0;
    if (p->text != NULL && n == 0) {
        return 0;
    }
    if (p->disconnect != NULL) {
        size_t r = p->disconnect(m, buf + n, cap - n);
        if (r == 0) {
            return 0;
        }
        n += r;
    }
    return n;
}

size_t cs_text(const cs_model *m, const char *line1, const char *line2, uint8_t *buf, size_t cap)
{
    const cs_protocol *p = cs_protocol_of(m->family);
    return p->text != NULL ? p->text(m, line1, line2, buf, cap) : 0;
}

size_t cs_value(const cs_model *m, cs_bar bar, int value, const char *line1, const char *line2,
                uint8_t *buf, size_t cap)
{
    const cs_protocol *p = cs_protocol_of(m->family);
    if (p->value == NULL) {
        return cs_text(m, line1, line2, buf, cap);
    }
    return p->value(m, bar, clamp7(value), line1, line2, buf, cap);
}

size_t cs_pad_light(const cs_model *m, int pad, int r, int g, int b, uint8_t *buf, size_t cap)
{
    const cs_protocol *p = cs_protocol_of(m->family);
    if (p->pad_light == NULL || pad < 0 || pad >= m->pads) {
        return 0;
    }
    return p->pad_light(m, pad, clamp7(r), clamp7(g), clamp7(b), buf, cap);
}
