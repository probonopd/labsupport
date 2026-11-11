/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.labsupport;

/**
 * The system exclusive messages for a device's display and lights; the twin of the writers in labsupport.h. Every
 * method returns one or more complete messages, or no bytes when the device has no such thing; they go to the port
 * whose role is CONTROL, or MAIN when the device has no CONTROL port.
 */
public final class Surface {
    private Surface() {
    }

    private static int clamp7(int v) {
        return Math.max(0, Math.min(127, v));
    }

    /** Take the display and the lights; send again whenever the device may have been switched by hand. */
    public static byte[] connect(Model m) {
        return Protocol.of(m.family).connect(m);
    }

    /**
     * Blank the display and give the display and the lights back to the device; send it when the application ends, so
     * that none of its text stays behind (most devices have no message that gives the display back).
     */
    public static byte[] disconnect(Model m) {
        Protocol p = Protocol.of(m.family);
        byte[] blank = p.text(m, "", "");
        byte[] release = p.disconnect(m);
        byte[] all = java.util.Arrays.copyOf(blank, blank.length + release.length);
        System.arraycopy(release, 0, all, blank.length, release.length);
        return all;
    }

    /** Two lines of text; characters outside 7-bit ASCII become '?'. */
    public static byte[] text(Model m, String line1, String line2) {
        return Protocol.of(m.family).text(m, line1, line2);
    }

    /** Two lines and a bar for VALUE 0..127; text only where there are no bars. */
    public static byte[] value(Model m, boolean fader, int value, String line1, String line2) {
        byte[] b = Protocol.of(m.family).value(m, fader, clamp7(value), line1, line2);
        return b != null ? b : text(m, line1, line2);
    }

    /** The light of pad PAD (0..) in 0..127 per channel; on/off devices light any nonzero color. */
    public static byte[] padLight(Model m, int pad, int r, int g, int b) {
        if (pad < 0 || pad >= m.pads) {
            return Protocol.NONE;
        }
        return Protocol.of(m.family).padLight(m, pad, clamp7(r), clamp7(g), clamp7(b));
    }
}
