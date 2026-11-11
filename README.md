# labsupport

labsupport lets software interact with Arturia KeyLab and MiniLab devices
through their displays, encoders, buttons and faders: an application shows its
own text and values on the device's display and is controlled by the device's
encoders, buttons, knobs, faders and pads. The library

* identifies a device and its ports by MIDI port name or USB vendor/product ID,
* tells which port carries the keys, which the controls and which takes the
  display and lights (port roles),
* decodes control messages (pads, knobs, faders, main encoder, buttons) into
  events,
* builds the system exclusive (SysEx) messages for display text, value bars,
  pad colors, and for taking over and releasing the display (connect and
  disconnect).

License: BSD-2-Clause, so that other applications can use it too.

## Supported devices

| Family | Models | USB IDs (vendor 0x1C75) | Mode the library drives | What the user must do |
|---|---|---|---|---|
| MiniLab 3 | MiniLab 3 | 0x020B, 0x220B | DAW mode | Enter DAW mode with Shift + Pad 3 (a host cannot switch it) |
| KeyLab | KeyLab 25, 49, 61, 88 (2014) | 0x0205, 0x0245, 0x0285, 0x02CD | the only mode; knobs set to absolute on connect | nothing (the 88 should be in memory 0 for the factory layout) |
| KeyLab Essential | KeyLab Essential 49, 61, 88 (2017) | 0x024A, 0x028A, 0x02CA (second hardware generation: 0x224A, 0x228A, 0x22CA) | Analog Lab map | keep the default Analog Lab map |
| KeyLab mkII | KeyLab mkII 49, 61, 88 | 0x024B, 0x028B, 0x02CB | Analog Lab mode | keep the default Analog Lab mode; press the Analog Lab button if it was left |
| KeyLab Essential mk3 | KeyLab Essential 49, 61, 88 mk3 | 0x024C, 0x028C, 0x02CC | DAW program, selected by connect | nothing (connect selects the program) |
| KeyLab mk3 | KeyLab 49, 61, 88 mk3 | 0x024E, 0x028E, 0x02CE | DAW program, selected by connect | if the unit ignores the selection, choose DAW with the Prog button |

Each `src/cs_<family>.c` explains in its header why its mode was chosen, what
its sources are and what is left out.

## Design

* C99 (`include/labsupport.h`, `src/`): no allocation, no I/O, no clock. The
  caller owns the MIDI ports and passes the time in, so the code runs in a
  desktop app, a plug-in, a command line tool or on a microcontroller.
* Java (`java/io/github/probonopd/labsupport/`): the same API as a twin, with no
  Android dependency (plain JVM classes).
* Both implementations are tested with the same vectors, `tests/vectors.txt`:
  device lookup, port roles, decoding, and the exact bytes of every message.

## Using it from C or C++

Add the repository as a git submodule, then either compile `src/*.c` with
`-Iinclude` into your build, or run `make` to get `liblabsupport.a`.

    git submodule add https://github.com/probonopd/labsupport third_party/labsupport

A complete sketch (the MIDI calls stand for your MIDI API):

    #include "labsupport.h"

    const cs_model *m = cs_model_by_name("KeyLab mkII 61");  /* or cs_model_by_usb() */
    if (!m) return;

    /* Choose ports by role from the port names the system reports. */
    for (each port p) {
        int roles = cs_port_roles(m, port_name(p));
        if (roles & CS_ROLE_CONTROLS) control_in = p;   /* decode this one */
        if (roles & CS_ROLE_DISPLAY)  display_out = p;  /* send SysEx here */
        if (roles & CS_ROLE_KEYS)     keys_in = p;      /* play this one */
    }

    uint8_t buf[CS_SYSEX_MAX];
    size_t n = cs_connect(m, buf, sizeof buf);
    if (n) send_sysex(display_out, buf, n);

    cs_decoder dec;
    cs_decoder_init(&dec, m);
    while (running) {
        /* for each channel message (status, d1, d2) from control_in */
        cs_event e = cs_decode(&dec, status, d1, d2, now_ms());
        switch (e.kind) {
        case CS_EV_NONE:      forward_to_synth(status, d1, d2); break;
        case CS_EV_KNOB:      /* e.index; e.value (absolute) or e.delta (relative knob) */
            n = cs_value(m, CS_BAR_KNOB, e.value, "Cutoff", "", buf, sizeof buf);
            if (n) send_sysex(display_out, buf, n);
            break;
        case CS_EV_PAD:       /* e.index, e.value = velocity, 0 = released */ break;
        case CS_EV_ENCODER:   /* e.delta steps */ break;
        case CS_EV_BUTTON:    if (e.index == CS_BTN_NEXT && e.value) next(); break;
        default: break;
        }
    }

    n = cs_pad_light(m, 0, 127, 0, 0, buf, sizeof buf);   /* pad 1 red */
    if (n) send_sysex(display_out, buf, n);

    n = cs_disconnect(m, buf, sizeof buf);                /* on exit: blanks the display */
    if (n) send_sysex(display_out, buf, n);

Every writer returns the number of bytes put into the buffer, or 0 when the
buffer is too small or the device has no such feature. `CS_SYSEX_MAX` is always
enough. A result may hold several SysEx messages back to back.

## Using it from Java or Android

Add the sources under `java/` to your build (they have no dependencies).

    Model m = Models.byName("KeyLab mkII 61");   // or Models.byUsb(0x1C75, 0x028B)
    int roles = Models.portRoles(m, portName);   // Roles.KEYS | CONTROLS | DISPLAY bits
    Decoder dec = new Decoder(m);

    // for each channel message from the controls port:
    Event e = dec.decode(status, d1, d2, System.currentTimeMillis());
    if (e.kind == Event.Kind.BUTTON && e.index == Button.NEXT.code() && e.value == 1) { next(); }

    byte[] connect = Surface.connect(m);
    byte[] text = Surface.text(m, "Line 1", "Line 2");
    byte[] bar = Surface.value(m, false, e.value, "Cutoff", "");
    byte[] pad = Surface.padLight(m, 0, 127, 0, 0);
    byte[] bye = Surface.disconnect(m);

On Android, enumerate `android.media.midi` devices, match each one's product
name with `Models.byName` (or vendor and product IDs with `Models.byUsb`), open
the output port (device to app) whose `Models.portRoles` has CONTROLS and feed
its messages to `Decoder.decode`, and open the input port (app to device) whose
roles contain DISPLAY and write the byte arrays from `Surface` to it. When the
MIDI API does not give port names, use `Models.portRolesAt(m, index)` with the
port number. An empty array means the device has no such feature.

## Event model

`cs_decode` / `Decoder.decode` takes one channel message plus a millisecond
timestamp and returns an event of a `kind`:

| Kind | index | value | delta |
|---|---|---|---|
| NONE | - | - | - (not a control: play it) |
| SWALLOWED | - | raw data | a control message that makes no event of its own |
| PAD | 0.. | velocity, 0 = released | 0 |
| KNOB | 0.. | 0..127 | 0 for an absolute knob; the signed step (never 0) for a relative one |
| FADER | 0.. | 0..127 | 0 |
| ENCODER | 0 | raw | steps, negative counter-clockwise |
| CLICK | 0 | 1 down, 0 up | 0 |
| BUTTON | `cs_button` / `Button.code()` | 1 down, 0 up | 0 |

Relative knobs and encoders send steps around 64 (binary offset); the decoder
reports the signed step in `delta`. The timestamp lets the decoder scale fast
turns of the main encoder. Buttons are identified by purpose (PREV, NEXT,
CATEGORY, PRESET, PART1..3, SOFT1..8, SELECT1..9, transport, BACK, OK), so an
application need not know which device has which.

## Port roles

A device exposes one or two MIDI ports; `cs_port_roles` (name) and
`cs_port_roles_at` (USB cable number) return bit flags:

* KEYS: keys, wheels and pedals; play them.
* CONTROLS: pads, knobs, faders, buttons; decode with `cs_decode`.
* DISPLAY: accepts the SysEx for display and lights.

On most devices one port ("MIDI") has all three roles. On the KeyLab mk3 the
"MIDI" port has KEYS, and the "DAW" port has CONTROLS and DISPLAY. A name
without a suffix is the first port; the roles of a port the application does
not use, or of another device's port, are 0. Matching ignores case and spaces.

## Tests

    make test

needs a C compiler and a JDK. It builds the library with `-Wall -Wextra -Werror`,
runs `tests/vectors.txt` through the C runner and the Java runner, and reports
failures. `make clean` removes the build output.

## Adding a device family

Add `src/cs_<family>.c` (controls table, port roles, writers) and its twin
`<Family>Protocol.java`, register the family in `include/labsupport.h`,
`src/cs_models.c`, `Models.java` and the `Makefile` source list, and add its
vectors to `tests/vectors.txt`. Both runners must pass.

## Limitations and open points

* The device must already be in the mode listed in the table where the library
  cannot switch it; if the user changes mode by hand, display and controls may
  stop working until the right mode is selected again (send connect again).
* Most devices have no message that gives the display back; disconnect blanks
  the text and the unit shows its own screen once the host stops drawing.
* Pad lights last until the unit repaints the pad (on press, release, memory or
  mode change) on some families.
* MiniLab 3 value bars show only right after the control event they reflect.
* Some control details are medium confidence or unknown: the size of a main
  encoder detent on some devices, touch sensors, button lamps, display footer
  and icons, and the extra banks of knobs and faders on the mkII are not
  covered.
* The KeyLab (2014) connect writes the global knob mode to absolute in the
  working memory and does not restore the previous mode.

## License

BSD-2-Clause, see `LICENSE`.
