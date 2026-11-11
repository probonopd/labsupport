/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.ctlsurface;

/** One control: a CC or note number (COUNT consecutive ones) on a channel (-1 any) and the event it is. */
final class Control {
    static final int CC = 0xB0, NOTE = 0x90;
    final int type, channel, number, count;
    final Event.Kind kind;
    final int index;
    final int flags;

    /**
     * One CC (number) carries COUNT buttons in its value: bit 0 is pressed, the bits above it are the button's offset
     * from index. A value beyond COUNT buttons is no match.
     */
    static final int VALUE_INDEX = 1;
    /** A KNOB that sends steps around 64 instead of a position: delta is value - 64, and 64 itself is swallowed. */
    static final int RELATIVE = 2;

    Control(int type, int channel, int number, int count, Event.Kind kind, int index) {
        this(type, channel, number, count, kind, index, 0);
    }

    Control(int type, int channel, int number, int count, Event.Kind kind, int index, int flags) {
        this.type = type;
        this.channel = channel;
        this.number = number;
        this.count = count;
        this.kind = kind;
        this.index = index;
        this.flags = flags;
    }
}
