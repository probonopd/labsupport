/*
 * Copyright (c) 2026 Simon Peter
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Runs tests/vectors.txt against the C library. */

#include "ctlsurface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 64

static int failures, checks;

/* Splits LINE into words; a word in double quotes may hold spaces. */
static int tokenize(char *line, char **tok)
{
    int n = 0;
    char *p = line;
    while (*p != '\0' && n < MAX_TOKENS) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        if (*p == '"') {
            tok[n++] = ++p;
            while (*p != '\0' && *p != '"') {
                p++;
            }
        } else {
            tok[n++] = p;
            while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
                p++;
            }
        }
        if (*p != '\0') {
            *p++ = '\0';
        }
    }
    return n;
}

static void fail(int lineno, const char *what, const char *got)
{
    failures++;
    fprintf(stderr, "vectors.txt:%d: %s: got %s\n", lineno, what, got);
}

static void hex(const uint8_t *b, size_t n, char *out)
{
    out[0] = '\0';
    if (n == 0) {
        strcpy(out, "none");
        return;
    }
    for (size_t i = 0; i < n; i++) {
        sprintf(out + strlen(out), i == 0 ? "%02X" : " %02X", b[i]);
    }
}

/* The expected bytes are the tokens after "->". */
static void expect_bytes(int lineno, char **tok, int n, int arrow, const uint8_t *b, size_t len)
{
    char got[CS_SYSEX_MAX * 3 + 8], want[CS_SYSEX_MAX * 3 + 8];
    want[0] = '\0';
    for (int i = arrow + 1; i < n; i++) {
        strcat(want, tok[i]);
        if (i + 1 < n) {
            strcat(want, " ");
        }
    }
    hex(b, len, got);
    checks++;
    if (strcmp(got, want) != 0) {
        fail(lineno, want, got);
    }
}

static const char *kind_name(cs_event_kind k)
{
    static const char *names[] = {"none", "swallowed", "pad", "knob", "fader", "encoder", "click", "button"};
    return names[k];
}

/* "keys+controls+display", or "none" */
static void roles_name(int r, char *out)
{
    out[0] = '\0';
    if (r & CS_ROLE_KEYS) {
        strcat(out, "+keys");
    }
    if (r & CS_ROLE_CONTROLS) {
        strcat(out, "+controls");
    }
    if (r & CS_ROLE_DISPLAY) {
        strcat(out, "+display");
    }
    memmove(out, out[0] != '\0' ? out + 1 : "none", strlen(out[0] != '\0' ? out + 1 : "none") + 1);
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "tests/vectors.txt";
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror(path);
        return 2;
    }
    char line[4096];
    char *tok[MAX_TOKENS];
    const cs_model *m = NULL;
    cs_decoder dec;
    uint8_t buf[CS_SYSEX_MAX];
    int lineno = 0;
    while (fgets(line, sizeof line, f) != NULL) {
        lineno++;
        if (line[0] == '#') {
            continue;
        }
        int n = tokenize(line, tok);
        if (n == 0) {
            continue;
        }
        int arrow = -1;
        for (int i = 0; i < n; i++) {
            if (strcmp(tok[i], "->") == 0) {
                arrow = i;
            }
        }
        const char *op = tok[0];
        if (strcmp(op, "model") == 0) {
            m = cs_model_by_name(tok[1]);
            checks++;
            if (m == NULL) {
                fail(lineno, tok[1], "no model");
                fclose(f);
                return 1;
            }
            cs_decoder_init(&dec, m);
        } else if (strcmp(op, "none") == 0) {
            const cs_model *x = cs_model_by_name(tok[1]);
            checks++;
            if (x != NULL) {
                fail(lineno, "none", x->name);
            }
        } else if (strcmp(op, "usb") == 0) {
            const cs_model *x = cs_model_by_usb((uint16_t)strtol(tok[1], NULL, 16), (uint16_t)strtol(tok[2], NULL, 16));
            const char *got = x != NULL ? x->name : "none";
            checks++;
            if (strcmp(got, tok[3]) != 0) {
                fail(lineno, tok[3], got);
            }
        } else if (strcmp(op, "port") == 0 || strcmp(op, "cable") == 0) {
            char got[64];
            roles_name(op[0] == 'p' ? cs_port_roles(m, tok[1]) : cs_port_roles_at(m, atoi(tok[1])), got);
            checks++;
            if (strcmp(got, tok[2]) != 0) {
                fail(lineno, tok[2], got);
            }
        } else if (strcmp(op, "decode") == 0) {
            cs_event e = cs_decode(&dec, (int)strtol(tok[2], NULL, 16), (int)strtol(tok[3], NULL, 16),
                                   (int)strtol(tok[4], NULL, 16), strtoll(tok[1], NULL, 10));
            char got[64], want[64];
            snprintf(got, sizeof got, "%s %d %d %d", kind_name(e.kind), e.index, e.value, e.delta);
            snprintf(want, sizeof want, "%s %s %s %s", tok[arrow + 1], tok[arrow + 2], tok[arrow + 3], tok[arrow + 4]);
            checks++;
            if (strcmp(got, want) != 0) {
                fail(lineno, want, got);
            }
        } else if (strcmp(op, "connect") == 0) {
            expect_bytes(lineno, tok, n, arrow, buf, cs_connect(m, buf, sizeof buf));
        } else if (strcmp(op, "disconnect") == 0) {
            expect_bytes(lineno, tok, n, arrow, buf, cs_disconnect(m, buf, sizeof buf));
        } else if (strcmp(op, "text") == 0) {
            expect_bytes(lineno, tok, n, arrow, buf, cs_text(m, tok[1], tok[2], buf, sizeof buf));
        } else if (strcmp(op, "value") == 0) {
            cs_bar bar = strcmp(tok[1], "fader") == 0 ? CS_BAR_FADER : CS_BAR_KNOB;
            expect_bytes(lineno, tok, n, arrow, buf, cs_value(m, bar, atoi(tok[2]), tok[3], tok[4], buf, sizeof buf));
        } else if (strcmp(op, "pad") == 0) {
            expect_bytes(lineno, tok, n, arrow, buf,
                         cs_pad_light(m, atoi(tok[1]), atoi(tok[2]), atoi(tok[3]), atoi(tok[4]), buf, sizeof buf));
        } else {
            fail(lineno, "a known operation", op);
        }
    }
    fclose(f);

    /* A buffer too small for a message gives 0, never a cut message. */
    if (m != NULL) {
        checks++;
        if (cs_text(cs_model_by_name("Minilab3"), "a", "b", buf, 5) != 0) {
            fail(0, "0 for a small buffer", "bytes");
        }
    }
    printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
