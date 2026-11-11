/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.ctlsurface;

/** A control moved; the twin of cs_event. */
public final class Event {
    public enum Kind {
        /** Not a control: play it (keys, wheels, pedals) */
        NONE,
        /** A control message that makes no event of its own */
        SWALLOWED,
        /** index 0.., value velocity, 0 = released */
        PAD,
        /** index 0.., value 0..127; delta is 0 for an absolute knob, the signed step (value - 64, never 0) for a relative one */
        KNOB,
        /** index 0.., value 0..127 */
        FADER,
        /** The main encoder: delta steps, negative counter-clockwise */
        ENCODER,
        /** The main encoder pressed: value 1 down, 0 up */
        CLICK,
        /** index a Button ordinal, value 1 down, 0 up */
        BUTTON
    }

    public final Kind kind;
    public final int index, value, delta;

    public Event(Kind kind, int index, int value, int delta) {
        this.kind = kind;
        this.index = index;
        this.value = value;
        this.delta = delta;
    }

    @Override
    public String toString() {
        return kind.name().toLowerCase(java.util.Locale.ROOT) + " " + index + " " + value + " " + delta;
    }
}
