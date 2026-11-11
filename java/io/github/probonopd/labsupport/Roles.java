/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.labsupport;

/** What a port of a device carries for the application (bit flags); the twin of the CS_ROLE_ flags. */
public final class Roles {
    /** The keys, wheels and pedals: play them */
    public static final int KEYS = 1;
    /** The controls: decode them with Decoder */
    public static final int CONTROLS = 2;
    /** Takes the messages for the display and the lights */
    public static final int DISPLAY = 4;

    private Roles() {
    }
}
