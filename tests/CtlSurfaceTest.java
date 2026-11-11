/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.ctlsurface;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

/** Runs tests/vectors.txt against the Java library, as tests/test_ctlsurface.c does against the C one. */
public final class CtlSurfaceTest {
    private static int failures, checks;

    private static List<String> tokenize(String line) {
        List<String> tok = new ArrayList<>();
        int i = 0;
        while (i < line.length()) {
            while (i < line.length() && Character.isWhitespace(line.charAt(i))) {
                i++;
            }
            if (i >= line.length()) {
                break;
            }
            int end;
            if (line.charAt(i) == '"') {
                end = line.indexOf('"', i + 1);
                end = end < 0 ? line.length() : end;
                tok.add(line.substring(i + 1, end));
                i = end + 1;
            } else {
                end = i;
                while (end < line.length() && !Character.isWhitespace(line.charAt(end))) {
                    end++;
                }
                tok.add(line.substring(i, end));
                i = end;
            }
        }
        return tok;
    }

    private static void check(int lineno, String want, String got) {
        checks++;
        if (!want.equals(got)) {
            failures++;
            System.err.println("vectors.txt:" + lineno + ": " + want + ": got " + got);
        }
    }

    private static String roles(int r) {
        StringBuilder s = new StringBuilder();
        if ((r & Roles.KEYS) != 0) {
            s.append("+keys");
        }
        if ((r & Roles.CONTROLS) != 0) {
            s.append("+controls");
        }
        if ((r & Roles.DISPLAY) != 0) {
            s.append("+display");
        }
        return s.length() == 0 ? "none" : s.substring(1);
    }

    private static String hex(byte[] b) {
        if (b.length == 0) {
            return "none";
        }
        StringBuilder s = new StringBuilder();
        for (byte x : b) {
            s.append(s.length() == 0 ? "" : " ").append(String.format("%02X", x & 0xFF));
        }
        return s.toString();
    }

    public static void main(String[] args) throws Exception {
        List<String> lines = Files.readAllLines(Paths.get(args.length > 0 ? args[0] : "tests/vectors.txt"),
            StandardCharsets.UTF_8);
        Model m = null;
        Decoder dec = null;
        int lineno = 0;
        for (String line : lines) {
            lineno++;
            if (line.startsWith("#")) {
                continue;
            }
            List<String> t = tokenize(line);
            if (t.isEmpty()) {
                continue;
            }
            int arrow = t.indexOf("->");
            String want = arrow < 0 ? "" : String.join(" ", t.subList(arrow + 1, t.size()));
            switch (t.get(0)) {
            case "model":
                m = Models.byName(t.get(1));
                if (m == null) {
                    throw new AssertionError("vectors.txt:" + lineno + ": no model " + t.get(1));
                }
                dec = new Decoder(m);
                break;
            case "none": {
                Model x = Models.byName(t.get(1));
                check(lineno, "none", x == null ? "none" : x.name);
                break;
            }
            case "usb": {
                Model x = Models.byUsb(Integer.parseInt(t.get(1), 16), Integer.parseInt(t.get(2), 16));
                check(lineno, t.get(3), x == null ? "none" : x.name);
                break;
            }
            case "port":
                check(lineno, t.get(2), roles(Models.portRoles(m, t.get(1))));
                break;
            case "cable":
                check(lineno, t.get(2), roles(Models.portRolesAt(m, Integer.parseInt(t.get(1)))));
                break;
            case "decode":
                check(lineno, want, dec.decode(Integer.parseInt(t.get(2), 16), Integer.parseInt(t.get(3), 16),
                    Integer.parseInt(t.get(4), 16), Long.parseLong(t.get(1))).toString());
                break;
            case "connect":
                check(lineno, want, hex(Surface.connect(m)));
                break;
            case "disconnect":
                check(lineno, want, hex(Surface.disconnect(m)));
                break;
            case "text":
                check(lineno, want, hex(Surface.text(m, t.get(1), t.get(2))));
                break;
            case "value":
                check(lineno, want, hex(Surface.value(m, t.get(1).equals("fader"), Integer.parseInt(t.get(2)),
                    t.get(3), t.get(4))));
                break;
            case "pad":
                check(lineno, want, hex(Surface.padLight(m, Integer.parseInt(t.get(1)), Integer.parseInt(t.get(2)),
                    Integer.parseInt(t.get(3)), Integer.parseInt(t.get(4)))));
                break;
            default:
                check(lineno, "a known operation", t.get(0));
            }
        }
        System.out.println(checks + " checks, " + failures + " failures");
        System.exit(failures == 0 ? 0 : 1);
    }
}
