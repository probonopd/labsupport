/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.labsupport;

/** What a family sends and takes; the twin of cs_protocol. Writers return no bytes for what the family lacks. */
abstract class Protocol {
    static final byte[] NONE = new byte[0];

    enum Encoding {
        /** Around 64, several messages per detent: steps by accumulated movement */
        OFFSET_BURST,
        /** Around 64, one message per step: value - 64 */
        OFFSET
    }

    abstract Control[] controls();

    abstract Encoding encoding();

    /**
     * Per port (USB cable) of the device: the suffixes its name may have, lower case without spaces and separated by
     * '|'; and its Roles flags.
     */
    abstract String[] portSuffixes();

    abstract int[] portRoles();

    byte[] connect(Model m) {
        return NONE;
    }

    byte[] disconnect(Model m) {
        return NONE;
    }

    byte[] text(Model m, String l1, String l2) {
        return NONE;
    }

    /** Null when the family has no bars: then the text is shown. */
    byte[] value(Model m, boolean fader, int v, String l1, String l2) {
        return null;
    }

    byte[] padLight(Model m, int pad, int r, int g, int b) {
        return NONE;
    }

    static Protocol of(Family f) {
        switch (f) {
        case MINILAB3:
            return MiniLab3Protocol.INSTANCE;
        case KEYLAB:
            return KeyLabProtocol.INSTANCE;
        case KEYLAB_ESSENTIAL:
            return KeyLabEssentialProtocol.INSTANCE;
        case KEYLAB_MK2:
            return KeyLabMk2Protocol.INSTANCE;
        case KEYLAB_ESSENTIAL_MK3:
            return KeyLabEssentialMk3Protocol.INSTANCE;
        case KEYLAB_MK3:
            return KeyLabMk3Protocol.INSTANCE;
        default:
            throw new IllegalArgumentException("no protocol for " + f);
        }
    }
}
