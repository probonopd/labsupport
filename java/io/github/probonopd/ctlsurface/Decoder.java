/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.ctlsurface;

/** Turns the channel messages of a device's control port into events; the twin of cs_decode. Not thread safe. */
public final class Decoder {
    /** Messages of the encoder closer than this belong to one turn (MiniLab 3: measured 2 to 5 per detent). */
    static final long ENC_BURST_MS = 70;
    /** Within one turn, this much movement (the offsets from 64, which grow with the speed) is one more step. */
    static final int ENC_TICKS_PER_STEP = 5;

    private final Protocol protocol;
    private int encDir;
    private long encMs = -1_000_000_000L;
    private int encTicks;

    public Decoder(Model m) {
        protocol = Protocol.of(m.family);
    }

    private Event encoder(int v, long now) {
        if (v == 64) {
            return new Event(Event.Kind.SWALLOWED, 0, v, 0);
        }
        if (protocol.encoding() == Protocol.Encoding.OFFSET) {
            return new Event(Event.Kind.ENCODER, 0, v, v - 64);
        }
        // The first message of a turn is a step, then one more per ENC_TICKS_PER_STEP of movement; a turn ends when
        // it pauses or turns back.
        int dir = v > 64 ? 1 : -1;
        boolean sameTurn = dir == encDir && now - encMs < ENC_BURST_MS;
        encMs = now;
        if (!sameTurn) {
            encDir = dir;
            encTicks = 0;
            return new Event(Event.Kind.ENCODER, 0, v, dir);
        }
        encTicks += Math.abs(v - 64);
        int steps = encTicks / ENC_TICKS_PER_STEP;
        if (steps == 0) {
            return new Event(Event.Kind.SWALLOWED, 0, v, 0);
        }
        encTicks %= ENC_TICKS_PER_STEP;
        return new Event(Event.Kind.ENCODER, 0, v, dir * steps);
    }

    /** The control a channel message stands for; kind NONE for everything that should play. */
    public Event decode(int status, int d1, int d2, long nowMs) {
        int type = status & 0xF0, ch = status & 0x0F;
        boolean released = type == 0x80 || (type == 0x90 && d2 == 0);
        if (type == 0x80) {
            type = 0x90;
        }
        for (Control c : protocol.controls()) {
            boolean byValue = (c.flags & Control.VALUE_INDEX) != 0;
            int span = byValue ? 1 : c.count;
            if (c.type != type || (c.channel >= 0 && c.channel != ch) || d1 < c.number || d1 >= c.number + span
                || (byValue && (d2 >> 1) >= c.count)) {
                continue;
            }
            int index = c.index + (byValue ? d2 >> 1 : d1 - c.number);
            if (byValue) {
                return new Event(c.kind, index, d2 & 1, 0);
            }
            switch (c.kind) {
            case ENCODER:
                return encoder(d2, nowMs);
            case PAD:
                return new Event(Event.Kind.PAD, index, released ? 0 : d2, 0);
            case CLICK:
            case BUTTON:
                return new Event(c.kind, index, (type == 0x90 ? !released : d2 != 0) ? 1 : 0, 0);
            case SWALLOWED:
                return new Event(Event.Kind.SWALLOWED, 0, d2, 0);
            case KNOB:
                if ((c.flags & Control.RELATIVE) != 0) {
                    return d2 == 64 ? new Event(Event.Kind.SWALLOWED, index, d2, 0) : new Event(Event.Kind.KNOB, index, d2, d2 - 64);
                }
                return new Event(Event.Kind.KNOB, index, d2, 0);
            default:
                return new Event(c.kind, index, d2, 0);
            }
        }
        return new Event(Event.Kind.NONE, 0, 0, 0);
    }
}
