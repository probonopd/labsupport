/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.ctlsurface;

/** Buttons by what they are for; Event.index of a BUTTON is code(), as cs_button in C. */
public enum Button {
    PREV, NEXT, CATEGORY, PRESET, PART1, PART2, LIVE,
    SOFT1, SOFT2, SOFT3, SOFT4, SOFT5, SOFT6, SOFT7, SOFT8,
    SELECT1, SELECT2, SELECT3, SELECT4, SELECT5, SELECT6, SELECT7, SELECT8, SELECT9,
    PLAY, STOP, RECORD, LOOP, REWIND, FORWARD,
    BACK, OK;

    public int code() {
        return ordinal() + 1;
    }

    public static Button of(int code) {
        Button[] all = values();
        return code >= 1 && code <= all.length ? all[code - 1] : null;
    }
}
