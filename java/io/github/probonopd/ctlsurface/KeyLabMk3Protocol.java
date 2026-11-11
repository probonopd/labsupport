/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.ctlsurface;

/**
 * Arturia KeyLab 49/61/88 mk3 in its DAW program; the twin of cs_keylab_mk3.c, where the choice, its sources and what
 * is left out are explained. Cable 0 "MIDI" carries the keys, cable 1 "DAW" the controls, the display and the lights.
 */
final class KeyLabMk3Protocol extends Protocol {
    static final KeyLabMk3Protocol INSTANCE = new KeyLabMk3Protocol();
    /** No fixed limit in the unit; 18 characters are the practical rule. */
    private static final int LINE_MAX = 18;
    private static final Control[] CONTROLS = {
        new Control(Control.NOTE, 9, 0, 12, Event.Kind.PAD, 0),
        new Control(Control.CC, 0, 116, 1, Event.Kind.ENCODER, 0),
        new Control(Control.CC, 0, 117, 1, Event.Kind.CLICK, 0),
        // Encoders 1-9 are CC 91, 92, 94, 95, 96, 97, 102, 103, 104
        new Control(Control.CC, 0, 91, 2, Event.Kind.KNOB, 0, Control.RELATIVE),
        new Control(Control.CC, 0, 94, 4, Event.Kind.KNOB, 2, Control.RELATIVE),
        new Control(Control.CC, 0, 102, 3, Event.Kind.KNOB, 6, Control.RELATIVE),
        new Control(Control.CC, 0, 105, 9, Event.Kind.FADER, 0),
        new Control(Control.CC, 0, 20, 1, Event.Kind.BUTTON, Button.STOP.code()),
        new Control(Control.CC, 0, 21, 1, Event.Kind.BUTTON, Button.PLAY.code()),
        new Control(Control.CC, 0, 22, 1, Event.Kind.BUTTON, Button.RECORD.code()),
        new Control(Control.CC, 0, 24, 1, Event.Kind.BUTTON, Button.LOOP.code()),
        new Control(Control.CC, 0, 25, 1, Event.Kind.BUTTON, Button.REWIND.code()),
        new Control(Control.CC, 0, 26, 1, Event.Kind.BUTTON, Button.FORWARD.code()),
        new Control(Control.CC, 0, 40, 1, Event.Kind.BUTTON, Button.BACK.code()),
        new Control(Control.CC, 0, 45, 8, Event.Kind.BUTTON, Button.SOFT1.code()),
    };

    @Override
    Control[] controls() {
        return CONTROLS;
    }

    @Override
    Encoding encoding() {
        return Encoding.OFFSET;
    }

    /** MIDI, DAW */
    @Override
    String[] portSuffixes() {
        return new String[] {"midi", "daw", null, null};
    }

    @Override
    int[] portRoles() {
        return new int[] {Roles.KEYS, Roles.CONTROLS | Roles.DISPLAY, 0, 0};
    }

    private static Out head(int cmd, int patch, int what) {
        return new Out().arturia().b(cmd, patch, what);
    }

    /**
     * Sets the connected flag (00 02 05 01) and selects the DAW program (00 02 07 01); the second is not used by the public
     * scripts.
     */
    @Override
    byte[] connect(Model m) {
        byte[] a = head(0x00, 0x02, 0x05).b(0x01, 0xF7).bytes();
        byte[] b = head(0x00, 0x02, 0x07).b(0x01, 0xF7).bytes();
        byte[] c = new byte[a.length + b.length];
        System.arraycopy(a, 0, c, 0, a.length);
        System.arraycopy(b, 0, c, a.length, b.length);
        return c;
    }

    @Override
    byte[] disconnect(Model m) {
        return head(0x00, 0x02, 0x05).b(0x00, 0xF7).bytes();
    }

    /** A text field is its index, the characters and a zero. */
    private static Out fieldText(Out o, int index, String s) {
        return o.b(index).ascii(s, LINE_MAX, 0).b(0x00);
    }

    private static Out fieldU7(Out o, int index, int v) {
        return o.b(index, v, 0x00);
    }

    /** White: the API has no color for text. */
    private static Out fieldWhite(Out o, int index) {
        return o.b(index, 0x7F, 0x7F, 0x7F, 0x00);
    }

    /** Screen 0x0C: two lines, each a text, a color mode (1: the color given) and a color. */
    @Override
    byte[] text(Model m, String l1, String l2) {
        Out o = head(0x00, 0x02, 0x04).b(0x0C);
        fieldWhite(fieldU7(fieldText(o, 0, l1), 1, 1), 2);
        fieldWhite(fieldU7(fieldText(o, 3, l2), 4, 1), 5);
        return o.b(0xF7).bytes();
    }

    /**
     * The knob popup 0x1C or the fader popup 0x1D (a thumb on a track; 0x1E is a bar without a thumb): value text, name,
     * position, color mode and color.
     */
    @Override
    byte[] value(Model m, boolean fader, int v, String l1, String l2) {
        Out o = head(0x00, 0x02, 0x04).b(fader ? 0x1D : 0x1C);
        fieldText(o, 0, l2);
        fieldText(o, 1, l1);
        fieldU7(o, 2, v);
        fieldU7(o, 3, 1);
        fieldWhite(o, 4);
        return o.b(0xF7).bytes();
    }

    /** The 12 DAW pads are LEDs 0x52-0x5D; the last data byte is a state, 0 here. */
    @Override
    byte[] padLight(Model m, int pad, int r, int g, int b) {
        return head(0x04, 0x02, 0x03).b(0x52 + pad, r, g, b, 0x00, 0xF7).bytes();
    }
}
