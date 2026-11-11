/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.labsupport;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;

/** Building system exclusive messages; the twin of cs_out. */
final class Out {
    private final ByteArrayOutputStream o = new ByteArrayOutputStream();

    Out b(int... bytes) {
        for (int x : bytes) {
            o.write(x);
        }
        return this;
    }

    /** F0 00 20 6B 7F 42: Arturia, broadcast device, the controller family. */
    Out arturia() {
        return b(0xF0, 0x00, 0x20, 0x6B, 0x7F, 0x42);
    }

    /**
     * At most MAX characters of S, each outside 0x20..0x7E as '?', padded with spaces to PAD (0 = none); counted per
     * UTF-8 character as the C library does.
     */
    Out ascii(String s, int max, int pad) {
        byte[] u = (s == null ? "" : s).getBytes(StandardCharsets.UTF_8);
        int shown = 0;
        for (int i = 0; i < u.length && shown < max; i++) {
            int c = u[i] & 0xFF;
            if ((c & 0xC0) == 0x80) {
                continue;
            }
            o.write(c >= 0x20 && c < 0x7F ? c : '?');
            shown++;
        }
        for (; shown < pad; shown++) {
            o.write(' ');
        }
        return this;
    }

    byte[] bytes() {
        return o.toByteArray();
    }
}
