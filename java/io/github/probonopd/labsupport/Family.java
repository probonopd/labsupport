/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package io.github.probonopd.labsupport;

/** The protocol generation of a device; models of one family differ only in size. */
public enum Family {
    MINILAB3,
    /** KeyLab 25/49/61/88, 2014 */
    KEYLAB,
    /** KeyLab Essential 49/61/88, 2017 */
    KEYLAB_ESSENTIAL,
    /** KeyLab 49/61/88 mkII, 2019 */
    KEYLAB_MK2,
    /** KeyLab Essential 49/61/88 mk3, 2023 */
    KEYLAB_ESSENTIAL_MK3,
    /** KeyLab 49/61/88 mk3, 2024 */
    KEYLAB_MK3
}
