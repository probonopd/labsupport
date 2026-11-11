/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.labsupport;

/**
 * Arturia MiniLab 3 in its DAW mode (Shift + Pad 3 on the device; a host cannot switch it). The display and lights
 * are Prot42 messages on the "MIDI" port (cable 0), measured on the device. The twin of cs_minilab3.c.
 */
final class MiniLab3Protocol extends Protocol {
    static final MiniLab3Protocol INSTANCE = new MiniLab3Protocol();
    /** The device takes about 30 characters a line. */
    private static final int LINE_MAX = 30;
    private static final Control[] CONTROLS = {
        new Control(Control.NOTE, 9, 36, 16, Event.Kind.PAD, 0),
        new Control(Control.CC, 0, 28, 1, Event.Kind.ENCODER, 0),
        new Control(Control.CC, 0, 118, 1, Event.Kind.CLICK, 0),
        new Control(Control.CC, 0, 86, 2, Event.Kind.KNOB, 0),
        new Control(Control.CC, 0, 89, 2, Event.Kind.KNOB, 2),
        new Control(Control.CC, 0, 110, 2, Event.Kind.KNOB, 4),
        new Control(Control.CC, 0, 116, 2, Event.Kind.KNOB, 6),
        new Control(Control.CC, 0, 14, 2, Event.Kind.FADER, 0),
        new Control(Control.CC, 0, 30, 2, Event.Kind.FADER, 2),
    };

    @Override
    Control[] controls() {
        return CONTROLS;
    }

    @Override
    Encoding encoding() {
        return Encoding.OFFSET_BURST;
    }

    /** MIDI, DIN THRU, MCU/HUI, ALV: the controls and the display on the first */
    @Override
    String[] portSuffixes() {
        return new String[] {"midi", "dinthru", "mcu/hui", "alv"};
    }

    @Override
    int[] portRoles() {
        return new int[] {Roles.KEYS | Roles.CONTROLS | Roles.DISPLAY, 0, 0, 0};
    }

    private static Out command(int cmd, int sub, int param) {
        return new Out().arturia().b(cmd, sub, param);
    }

    private static byte[] lines(Out o, String l1, String l2) {
        o.b(0x01).ascii(l1, LINE_MAX, 0).b(0x00, 0x02).ascii(l2, LINE_MAX, 0).b(0x00, 0xF7);
        return o.bytes();
    }

    @Override
    byte[] connect(Model m) {
        return command(0x02, 0x02, 0x40).b(0x6A, 0x21, 0xF7).bytes();
    }

    @Override
    byte[] disconnect(Model m) {
        return command(0x02, 0x02, 0x40).b(0x6A, 0x20, 0xF7).bytes();
    }

    @Override
    byte[] text(Model m, String l1, String l2) {
        return lines(command(0x04, 0x02, 0x60), l1, l2);
    }

    /**
     * The bar pages (modes 3 to 5) are drawn by the unit only when a control on the device moved within the last
     * 500 ms; otherwise the display stays on the program name. So a bar must be sent right after the control event it
     * reflects; a bar for a value that no control just changed (a menu value, say) may not show at all.
     */
    @Override
    byte[] value(Model m, boolean fader, int v, String l1, String l2) {
        return lines(command(0x04, 0x02, 0x60).b(0x1F, fader ? 0x04 : 0x03, 0x01, v, 0x00, 0x00), l1, l2);
    }

    /** Pads 1-8 are LEDs 0x34-0x3B, 9-16 are 0x44-0x4B; a color stays until the device is switched off. */
    @Override
    byte[] padLight(Model m, int pad, int r, int g, int b) {
        return command(0x02, 0x02, 0x16).b(pad < 8 ? 0x34 + pad : 0x44 + pad - 8, r, g, b, 0xF7).bytes();
    }
}
