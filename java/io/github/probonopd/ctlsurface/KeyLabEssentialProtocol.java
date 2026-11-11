/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.ctlsurface;

/**
 * Arturia KeyLab Essential 49/61/88 (first generation, 2017) in its Analog Lab map, the map it starts in; the twin of
 * cs_keylab_essential.c, where the choice, its sources and what is left out are explained. Everything is on cable 0.
 */
final class KeyLabEssentialProtocol extends Protocol {
    static final KeyLabEssentialProtocol INSTANCE = new KeyLabEssentialProtocol();
    /** The LCD is 2 x 16 characters. */
    private static final int LINE_MAX = 16;
    // Channel 1. The jog is CC 114 while "Preset" is lit and CC 112 while "Category" is lit, offset binary around 64;
    // its click CC 115 or 113. Pads 1-8 are notes 36-43 on channel 10, apart from the keys on channel 1.
    private static final Control[] CONTROLS = {
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
        new Control(Control.NOTE, 9, 36, 8, Event.Kind.PAD, 0),
    };

    @Override
    Control[] controls() {
        return CONTROLS;
    }

    @Override
    Encoding encoding() {
        return Encoding.OFFSET;
    }

    /**
     * MIDI, DAW. "mid" is how a 31-character port name ends on Linux; the macOS names end in "In" or "Out". The DAW
     * cable carries only the DAW map's messages, which this map does not use.
     */
    @Override
    String[] portSuffixes() {
        return new String[] {"midi|midi1|mid|midiin|midiout", "daw|midi2|dawin|dawout", null, null};
    }

    @Override
    int[] portRoles() {
        return new int[] {Roles.KEYS | Roles.CONTROLS | Roles.DISPLAY, 0, 0, 0};
    }

    /** Padded: whether the unit pads text from SysEx itself was not determined. */
    @Override
    byte[] text(Model m, String l1, String l2) {
        return new Out().arturia().b(0x04, 0x00, 0x60, 0x01).ascii(l1, LINE_MAX, LINE_MAX).b(0x00, 0x02)
            .ascii(l2, LINE_MAX, LINE_MAX).b(0x00, 0xF7).bytes();
    }

    /** Pads 1-8 are the LEDs 0x70-0x77 with 5 bits a channel, so the 7-bit color is shifted down by 2. */
    @Override
    byte[] padLight(Model m, int pad, int r, int g, int b) {
        return new Out().arturia().b(0x02, 0x00, 0x16, 0x70 + pad, r >> 2, g >> 2, b >> 2, 0xF7).bytes();
    }
}
