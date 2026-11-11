/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.ctlsurface;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

/** The devices, and finding one by name or USB IDs; the twin of cs_models.c and the lookups of cs_core.c. */
public final class Models {
    private static final int ARTURIA = 0x1C75;

    /** A product ID that is not verified is 0. */
    public static final List<Model> ALL = Collections.unmodifiableList(Arrays.asList(
        new Model("MiniLab 3", Family.MINILAB3, ARTURIA, new int[] {0x020B, 0x220B}, "Minilab3", 16, 8, 4, 2, 30, true, true),
        // First generation; see cs_models.c for where the IDs and strings come from.
        new Model("KeyLab 25", Family.KEYLAB, ARTURIA, new int[] {0x0205, 0}, "KeyLab 25", 0, 10, 9, 2, 16, false, false),
        new Model("KeyLab 49", Family.KEYLAB, ARTURIA, new int[] {0x0245, 0}, "KeyLab 49", 16, 10, 9, 2, 16, false, false),
        new Model("KeyLab 61", Family.KEYLAB, ARTURIA, new int[] {0x0285, 0}, "KeyLab 61", 16, 10, 9, 2, 16, false, false),
        new Model("KeyLab 88", Family.KEYLAB, ARTURIA, new int[] {0x02CD, 0}, "KeyLab 88", 16, 10, 9, 2, 16, false, false),
        new Model("KeyLab Essential 49", Family.KEYLAB_ESSENTIAL, ARTURIA, new int[] {0x024A, 0x224A}, "Arturia KeyLab Essential 49", 8, 9, 9, 2, 16, true, false),
        new Model("KeyLab Essential 61", Family.KEYLAB_ESSENTIAL, ARTURIA, new int[] {0x028A, 0x228A}, "Arturia KeyLab Essential 61", 8, 9, 9, 2, 16, true, false),
        new Model("KeyLab Essential 88", Family.KEYLAB_ESSENTIAL, ARTURIA, new int[] {0x02CA, 0x22CA}, "Arturia KeyLab Essential 88", 8, 9, 9, 2, 16, true, false),
        new Model("KeyLab mkII 49", Family.KEYLAB_MK2, ARTURIA, new int[] {0x024B, 0}, "KeyLab mkII 49", 16, 9, 9, 2, 16, true, false),
        new Model("KeyLab mkII 61", Family.KEYLAB_MK2, ARTURIA, new int[] {0x028B, 0}, "KeyLab mkII 61", 16, 9, 9, 2, 16, true, false),
        new Model("KeyLab mkII 88", Family.KEYLAB_MK2, ARTURIA, new int[] {0x02CB, 0}, "KeyLab mkII 88", 16, 9, 9, 2, 16, true, false),
        new Model("KeyLab Essential 49 mk3", Family.KEYLAB_ESSENTIAL_MK3, ARTURIA, new int[] {0x024C, 0}, "KL Essential 49 mk3", 16, 9, 9, 2, 18, true, true),
        new Model("KeyLab Essential 61 mk3", Family.KEYLAB_ESSENTIAL_MK3, ARTURIA, new int[] {0x028C, 0}, "KL Essential 61 mk3", 16, 9, 9, 2, 18, true, true),
        new Model("KeyLab Essential 88 mk3", Family.KEYLAB_ESSENTIAL_MK3, ARTURIA, new int[] {0x02CC, 0}, "KL Essential 88 mk3", 16, 9, 9, 2, 18, true, true),
        new Model("KeyLab 49 mk3", Family.KEYLAB_MK3, ARTURIA, new int[] {0x024E, 0}, "KeyLab 49 mk3", 12, 9, 9, 2, 18, true, true),
        new Model("KeyLab 61 mk3", Family.KEYLAB_MK3, ARTURIA, new int[] {0x028E, 0}, "KeyLab 61 mk3", 12, 9, 9, 2, 18, true, true),
        new Model("KeyLab 88 mk3", Family.KEYLAB_MK3, ARTURIA, new int[] {0x02CE, 0}, "KeyLab 88 mk3", 12, 9, 9, 2, 18, true, true)));

    private Models() {
    }

    /** Names are compared lower case and without spaces: hosts spell one device "Minilab3 MIDI" and "MiniLab 3". */
    static String normalize(String s) {
        return s == null ? "" : s.replace(" ", "").toLowerCase(Locale.ROOT);
    }

    /**
     * The model whose USB product or MIDI port name this is (the product string plus a suffix such as " MIDI" or
     * " DAW"), or null. The longest product wins, so that "KeyLab 61 mk3" is not taken for "KeyLab 61".
     */
    public static Model byName(String name) {
        return find(normalize(name), null);
    }

    private static Model find(String n, int[] productEnd) {
        Model best = null;
        int bestLen = 0;
        for (Model m : ALL) {
            String p = normalize(m.product);
            int at = p.isEmpty() ? -1 : n.indexOf(p);
            if (at >= 0 && p.length() > bestLen) {
                best = m;
                bestLen = p.length();
                if (productEnd != null) {
                    productEnd[0] = at + p.length();
                }
            }
        }
        return best;
    }

    /** The model with this USB vendor and product ID, or null. */
    public static Model byUsb(int vendor, int product) {
        for (Model m : ALL) {
            if (m.usbVendor == vendor && product != 0 && (m.usbProducts[0] == product || m.usbProducts[1] == product)) {
                return m;
            }
        }
        return null;
    }

    private static boolean listed(String list, String word) {
        return list != null && Arrays.asList(list.split("\\|", -1)).contains(word);
    }

    /**
     * The Roles of the port of model M with this name (the device's name plus a suffix such as " MIDI" or " DAW"); 0
     * for a port the application does not use or a name of another device. A name without a suffix is the first port.
     */
    public static int portRoles(Model m, String portName) {
        String n = normalize(portName);
        int[] end = new int[1];
        if (m == null || find(n, end) != m) {
            return 0;
        }
        String suffix = n.substring(end[0]);
        Protocol p = Protocol.of(m.family);
        if (suffix.isEmpty()) {
            return p.portRoles()[0];
        }
        String[] suffixes = p.portSuffixes();
        for (int i = 0; i < suffixes.length; i++) {
            if (listed(suffixes[i], suffix)) {
                return p.portRoles()[i];
            }
        }
        return 0;
    }

    /** The Roles of port INDEX (0.., the USB cable number), for hosts that do not know the port names. */
    public static int portRolesAt(Model m, int index) {
        if (m == null) {
            return 0;
        }
        int[] roles = Protocol.of(m.family).portRoles();
        return index >= 0 && index < roles.length ? roles[index] : 0;
    }
}
