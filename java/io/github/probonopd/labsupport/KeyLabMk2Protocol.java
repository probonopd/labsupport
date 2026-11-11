/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.labsupport;

/**
 * Arturia KeyLab mkII 49/61/88 in its Analog Lab mode, the mode the unit starts in; the twin of cs_keylab_mk2.c, where
 * the choice, its sources and what is left out are explained. Connect and disconnect send nothing: the unit has no
 * SysEx that switches the mode, and Analog Lab needs none.
 */
final class KeyLabMk2Protocol extends Protocol {
    static final KeyLabMk2Protocol INSTANCE = new KeyLabMk2Protocol();
    /** The LCD is 2 x 16 characters. */
    private static final int LINE_MAX = 16;
    private static final Control[] CONTROLS = {
        new Control(Control.NOTE, 9, 36, 16, Event.Kind.PAD, 0),
        // The jog wheel turns and clicks on one CC pair while the Category lamp is lit and on another while the Preset
        // lamp is lit; both are the encoder.
        new Control(Control.CC, 0, 112, 1, Event.Kind.ENCODER, 0),
        new Control(Control.CC, 0, 113, 1, Event.Kind.CLICK, 0),
        new Control(Control.CC, 0, 114, 1, Event.Kind.ENCODER, 0),
        new Control(Control.CC, 0, 115, 1, Event.Kind.CLICK, 0),
        new Control(Control.CC, 0, 116, 1, Event.Kind.BUTTON, Button.CATEGORY.code()),
        new Control(Control.CC, 0, 117, 1, Event.Kind.BUTTON, Button.PRESET.code()),
        new Control(Control.CC, 0, 28, 1, Event.Kind.BUTTON, Button.PREV.code()),
        new Control(Control.CC, 0, 29, 1, Event.Kind.BUTTON, Button.NEXT.code()),
        new Control(Control.CC, 0, 22, 1, Event.Kind.BUTTON, Button.PART1.code()),
        new Control(Control.CC, 0, 23, 1, Event.Kind.BUTTON, Button.PART2.code()),
        new Control(Control.CC, 0, 24, 1, Event.Kind.BUTTON, Button.LIVE.code()),
        // The nine select buttons share CC 30: value = ((n - 1) << 1) | pressed.
        new Control(Control.CC, 0, 30, 9, Event.Kind.BUTTON, Button.SELECT1.code(), Control.VALUE_INDEX),
        new Control(Control.CC, 0, 74, 1, Event.Kind.KNOB, 0),
        new Control(Control.CC, 0, 71, 1, Event.Kind.KNOB, 1),
        new Control(Control.CC, 0, 76, 2, Event.Kind.KNOB, 2),
        new Control(Control.CC, 0, 93, 1, Event.Kind.KNOB, 4),
        new Control(Control.CC, 0, 18, 2, Event.Kind.KNOB, 5),
        new Control(Control.CC, 0, 16, 2, Event.Kind.KNOB, 7),
        new Control(Control.CC, 0, 73, 1, Event.Kind.FADER, 0),
        new Control(Control.CC, 0, 75, 1, Event.Kind.FADER, 1),
        new Control(Control.CC, 0, 79, 1, Event.Kind.FADER, 2),
        new Control(Control.CC, 0, 72, 1, Event.Kind.FADER, 3),
        new Control(Control.CC, 0, 80, 4, Event.Kind.FADER, 4),
        new Control(Control.CC, 0, 85, 1, Event.Kind.FADER, 8),
        // Sent on the keys' cable but not for the keys: swallowed so that they are not played as controllers.
        new Control(Control.CC, 0, 26, 2, Event.Kind.SWALLOWED, 0),
        new Control(Control.CC, 0, 118, 1, Event.Kind.SWALLOWED, 0),
    };

    @Override
    Control[] controls() {
        return CONTROLS;
    }

    @Override
    Encoding encoding() {
        return Encoding.OFFSET;
    }

    /** MIDI, DAW (jack strings; "MIDI 1" and "MIDI 2" on older Linux kernels) */
    @Override
    String[] portSuffixes() {
        return new String[] {"midi|midi1", "daw|midi2", null, null};
    }

    @Override
    int[] portRoles() {
        return new int[] {Roles.KEYS | Roles.CONTROLS | Roles.DISPLAY, 0, 0, 0};
    }

    /** Both lines padded with spaces, so that an empty line does not depend on how the unit ends a message. */
    @Override
    byte[] text(Model m, String l1, String l2) {
        return new Out().arturia().b(0x04, 0x00, 0x60, 0x01).ascii(l1, LINE_MAX, LINE_MAX).b(0x00, 0x02)
            .ascii(l2, LINE_MAX, LINE_MAX).b(0x00, 0xF7).bytes();
    }

    /** Pads 1-16 are LEDs 0x70-0x7F; the driver has 5 bits a channel, so the 7-bit color is shifted down by 2. */
    @Override
    byte[] padLight(Model m, int pad, int r, int g, int b) {
        return new Out().arturia().b(0x02, 0x00, 0x16, 0x70 + pad, r >> 2, g >> 2, b >> 2, 0xF7).bytes();
    }
}
