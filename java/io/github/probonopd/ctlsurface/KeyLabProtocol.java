/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.ctlsurface;

/**
 * Arturia KeyLab 25/49/61/88 (first generation, 2014): no mode, one cable; the twin of cs_keylab.c, where the choice,
 * its sources and what is left out are explained. Connect only makes the ten encoders send positions.
 */
final class KeyLabProtocol extends Protocol {
    static final KeyLabProtocol INSTANCE = new KeyLabProtocol();
    /** The LCD is 2 x 16 characters; a 17th would spill into the next line. */
    private static final int LINE_MAX = 16;
    private static final Control[] CONTROLS = {
        new Control(Control.NOTE, 9, 36, 16, Event.Kind.PAD, 0),
        new Control(Control.CC, -1, 112, 1, Event.Kind.ENCODER, 0),
        new Control(Control.CC, -1, 113, 1, Event.Kind.CLICK, 0),
        new Control(Control.CC, -1, 114, 1, Event.Kind.ENCODER, 0),
        new Control(Control.CC, -1, 115, 1, Event.Kind.CLICK, 0),
        new Control(Control.CC, -1, 74, 1, Event.Kind.KNOB, 0),
        new Control(Control.CC, -1, 71, 1, Event.Kind.KNOB, 1),
        new Control(Control.CC, -1, 76, 2, Event.Kind.KNOB, 2),
        new Control(Control.CC, -1, 93, 1, Event.Kind.KNOB, 4),
        new Control(Control.CC, -1, 18, 2, Event.Kind.KNOB, 5),
        new Control(Control.CC, -1, 16, 2, Event.Kind.KNOB, 7),
        new Control(Control.CC, -1, 91, 1, Event.Kind.KNOB, 9),
        new Control(Control.CC, -1, 73, 1, Event.Kind.FADER, 0),
        new Control(Control.CC, -1, 75, 1, Event.Kind.FADER, 1),
        new Control(Control.CC, -1, 79, 1, Event.Kind.FADER, 2),
        new Control(Control.CC, -1, 72, 1, Event.Kind.FADER, 3),
        new Control(Control.CC, -1, 80, 4, Event.Kind.FADER, 4),
        new Control(Control.CC, -1, 85, 1, Event.Kind.FADER, 8),
    };

    @Override
    Control[] controls() {
        return CONTROLS;
    }

    @Override
    Encoding encoding() {
        return Encoding.OFFSET;
    }

    /** The one cable; ALSA names it "KeyLab 61 MIDI 1", newer kernels "KeyLab 61 MIDI". */
    @Override
    String[] portSuffixes() {
        return new String[] {"midi|midi1", null, null, null};
    }

    @Override
    int[] portRoles() {
        return new int[] {Roles.KEYS | Roles.CONTROLS | Roles.DISPLAY, 0, 0, 0};
    }

    /** The global knob mode "absolute": memories 1 to 10 of the unit send steps, and the 88 returns to its last memory. */
    @Override
    byte[] connect(Model m) {
        return new Out().arturia().b(0x02, 0x00, 0x40, 0x02, 0x01, 0xF7).bytes();
    }

    /** Both lines padded, so that an empty line does not depend on how the unit treats an empty text. */
    @Override
    byte[] text(Model m, String l1, String l2) {
        return new Out().arturia().b(0x04, 0x00, 0x60, 0x01).ascii(l1, LINE_MAX, LINE_MAX).b(0x00, 0x02)
            .ascii(l2, LINE_MAX, LINE_MAX).b(0x00, 0xF7).bytes();
    }

    /** Pads 1-16 are LED ids 0x70-0x7F; any non-zero value is on and there is no color. */
    @Override
    byte[] padLight(Model m, int pad, int r, int g, int b) {
        return new Out().arturia().b(0x02, 0x00, 0x10, 0x70 + pad, (r | g | b) != 0 ? 0x7F : 0x00, 0xF7).bytes();
    }
}
