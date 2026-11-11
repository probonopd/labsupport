/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.ctlsurface;

/** A device and what it has; the twin of cs_model in ctlsurface.h. */
public final class Model {
    public final String name;
    public final Family family;
    public final int usbVendor;
    /** 0 = unknown or unused */
    final int[] usbProducts;
    /** The USB product string as the host shows it */
    public final String product;
    public final int pads, knobs, faders;
    /** Text lines the host can write, and the characters of a line that are shown */
    public final int lines, lineChars;
    /** Pads take a color, not only on/off */
    public final boolean rgbPads;
    /** The display can show a value bar */
    public final boolean bars;

    Model(String name, Family family, int usbVendor, int[] usbProducts, String product, int pads, int knobs, int faders,
          int lines, int lineChars, boolean rgbPads, boolean bars) {
        this.name = name;
        this.family = family;
        this.usbVendor = usbVendor;
        this.usbProducts = usbProducts;
        this.product = product;
        this.pads = pads;
        this.knobs = knobs;
        this.faders = faders;
        this.lines = lines;
        this.lineChars = lineChars;
        this.rgbPads = rgbPads;
        this.bars = bars;
    }

    @Override
    public String toString() {
        return name;
    }
}
