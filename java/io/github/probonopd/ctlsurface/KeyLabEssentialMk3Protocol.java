/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.ctlsurface;

/**
 * Arturia KeyLab Essential 49/61/88 mk3 in its DAW program; the twin of cs_keylab_essential_mk3.c, where the choice,
 * its sources and what is left out are explained.
 */
final class KeyLabEssentialMk3Protocol extends Protocol {
    static final KeyLabEssentialMk3Protocol INSTANCE = new KeyLabEssentialMk3Protocol();
    /** 18 characters a line are shown (hardware report). */
    private static final int LINE_MAX = 18;
    /** The knob and fader widgets clip their two texts to 12. */
    private static final int WIDGET_MAX = 12;
    private static final Control[] CONTROLS = {
        // Pads are channel 11 (index 10) in the DAW program; the top row of a bank has the higher notes.
        new Control(Control.NOTE, 10, 40, 4, Event.Kind.PAD, 0),
        new Control(Control.NOTE, 10, 36, 4, Event.Kind.PAD, 4),
        new Control(Control.NOTE, 10, 48, 4, Event.Kind.PAD, 8),
        new Control(Control.NOTE, 10, 44, 4, Event.Kind.PAD, 12),
        new Control(Control.CC, 0, 116, 1, Event.Kind.ENCODER, 0),
        new Control(Control.CC, 0, 117, 1, Event.Kind.CLICK, 0),
        new Control(Control.CC, 0, 44, 4, Event.Kind.BUTTON, Button.SOFT1.code()),
        new Control(Control.CC, 0, 20, 1, Event.Kind.BUTTON, Button.STOP.code()),
        new Control(Control.CC, 0, 21, 1, Event.Kind.BUTTON, Button.PLAY.code()),
        new Control(Control.CC, 0, 22, 1, Event.Kind.BUTTON, Button.RECORD.code()),
        new Control(Control.CC, 0, 24, 1, Event.Kind.BUTTON, Button.LOOP.code()),
        new Control(Control.CC, 0, 25, 1, Event.Kind.BUTTON, Button.REWIND.code()),
        new Control(Control.CC, 0, 26, 1, Event.Kind.BUTTON, Button.FORWARD.code()),
        new Control(Control.CC, 0, 96, 9, Event.Kind.KNOB, 0),
        new Control(Control.CC, 0, 105, 9, Event.Kind.FADER, 0),
        // Buttons without a role here, on the keys' cable: Tap, Metronome, Save/Quantize/Undo/Redo, Bank/Part.
        new Control(Control.CC, 0, 23, 1, Event.Kind.SWALLOWED, 0),
        new Control(Control.CC, 0, 27, 1, Event.Kind.SWALLOWED, 0),
        new Control(Control.CC, 0, 40, 4, Event.Kind.SWALLOWED, 0),
        new Control(Control.CC, 0, 118, 2, Event.Kind.SWALLOWED, 0),
    };

    @Override
    Control[] controls() {
        return CONTROLS;
    }

    @Override
    Encoding encoding() {
        return Encoding.OFFSET;
    }

    /** MIDI, DINTHRU, MCU/HUI, ALV */
    @Override
    String[] portSuffixes() {
        return new String[] {"midi", "dinthru", "mcu/hui", "alv"};
    }

    @Override
    int[] portRoles() {
        return new int[] {Roles.KEYS | Roles.CONTROLS | Roles.DISPLAY, 0, 0, 0};
    }

    private static Out head(int cmd, int what) {
        return new Out().arturia().b(cmd, 0x01, what); // patch 1: the DAW program
    }

    private static Out state(boolean on) {
        return new Out().arturia().b(0x02, 0x0F, 0x40, 0x5A, on ? 0x01 : 0x00, 0xF7); // DAW connected
    }

    /** Sets the state and selects the DAW program (item 2 of the program record = 1; the unit echoes it). */
    @Override
    byte[] connect(Model m) {
        return concat(state(true).bytes(), new Out().arturia().b(0x21, 0x11, 0x40, 0x02, 0x00, 0x01, 0xF7).bytes());
    }

    @Override
    byte[] disconnect(Model m) {
        return state(false).bytes();
    }

    /** Screen 0x12: two lines, then a flag byte (0: the screen stays). */
    @Override
    byte[] text(Model m, String l1, String l2) {
        return head(0x04, 0x60).b(0x12, 0x01).ascii(l1, LINE_MAX, 0).b(0x00, 0x02).ascii(l2, LINE_MAX, 0)
            .b(0x00, 0x00, 0xF7).bytes();
    }

    /** Screen 0x14 (knob) or 0x15 (fader): the two texts and the position, as a transient screen (flag 1). */
    @Override
    byte[] value(Model m, boolean fader, int v, String l1, String l2) {
        return head(0x04, 0x60).b(fader ? 0x15 : 0x14, 0x01).ascii(l1, WIDGET_MAX, 0).b(0x00, 0x02)
            .ascii(l2, WIDGET_MAX, 0).b(0x00, 0x03, v, 0x00, 0x01, 0xF7).bytes();
    }

    /** LEDs 0x1C-0x23 for pads 1-8 and 0x24-0x2B for pads 9-16; the unit takes 7 bits a channel. */
    @Override
    byte[] padLight(Model m, int pad, int r, int g, int b) {
        return head(0x04, 0x16).b(0x1C + pad, r, g, b, 0xF7).bytes();
    }

    private static byte[] concat(byte[] a, byte[] b) {
        byte[] c = new byte[a.length + b.length];
        System.arraycopy(a, 0, c, 0, a.length);
        System.arraycopy(b, 0, c, a.length, b.length);
        return c;
    }
}
