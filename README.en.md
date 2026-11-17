# Passwort-Button

**Press a button instead of typing a password.** A Raspberry Pi Pico 2 stores one password locally and types it as a USB keyboard after a short physical BOOTSEL press. Use it with password managers, sign-in dialogs or other text fields; KeePass is just one example.

[Deutsch](README.md) · [English setup guide](docs/SETUP.en.md) · [Contributing](CONTRIBUTING.md) · [Security](SECURITY.md)

## Status

Initially **private** repository, `DerCodeBastler/Passwort-Button`. This package does not authorize public publication. Firmware **1.1.0** is byte-for-byte identical to the supplied ZIP. Only documentation, visible setup text and development support have changed. The setup interface remains German; English instructions are included.

## Features and requirements

| Feature | Scope |
| --- | --- |
| Hardware | Raspberry Pi Pico 2 / RP2350, USB data cable; no extra button |
| Controls | Built-in BOOTSEL button and LED feedback |
| Output | One stored password, optional Enter |
| Timing | Persistent 3–50 ms pause after each completed HID report |
| Characters | Up to 128 supported characters; German Windows keyboard layout |
| Configuration | Local HTML file, Chrome/Edge on Windows, Web Serial |
| Daily use | USB keyboard; configuration page can remain closed |
| Development | C source, host tests and prebuilt UF2 included |

This is not a FIDO2/passkey device. It does not identify applications: text goes into the **currently focused field**. Other boards and keyboard layouts are not listed as supported.

## Quick start

1. Extract the package. Unplug Pico 2, hold BOOTSEL, connect USB, then release the button.
2. Copy [`KeePass_Pico2_3ms.uf2`](KeePass_Pico2_3ms.uf2) to `RP2350`; the device restarts.
3. Open [`Passwort_einrichten.html`](Passwort_einrichten.html) locally in Chrome/Edge. Choose “Pico verbinden”. The device still identifies as “KeePass-Taster Pico 2”.
4. Enter a **harmless test password** twice; choose timing and optional Enter.
5. Hold BOOTSEL on the connected device for about **2 seconds**, until the LED double-blinks. Release it and save within 60 seconds. Each unlock allows one valid write operation.
6. Select the German Windows keyboard layout and focus an editor. Briefly press and release BOOTSEL. Only configure your real password locally after testing.

Speed and Enter can be changed without re-entering the password, using another physical unlock. A short press while configuration is unlocked closes the unlock window without typing. See the [full English guide](docs/SETUP.en.md).

## Security and privacy

Normal firmware operation exposes no USB command for reading the password or triggering output. **The host can capture the password during setup and typing.** Flash stores unencrypted keycodes. Physical access, debugging and replacement firmware are not prevented. Other host software can issue writes during the physical configuration window.

The setup page has no network requests or external resources. Firmware updates do not necessarily erase stored passwords. Never upload real passwords, flash dumps or personal logs. This hobby project is not a certified security key. Read [SECURITY.md](SECURITY.md).

## Development

See [build and test instructions](docs/DEVELOPMENT.md), [contribution guidelines](CONTRIBUTING.md), [review record](docs/REVIEW-2026-11-17.md), [changelog](CHANGELOG.md) and [checksums](SHA256.txt). The [historical report](Pruefbericht.txt) describes the original build and is not a new hardware test.

The legacy UF2 filename, USB product name and internal build identifiers are intentionally preserved. Firmware rebranding requires a separate rebuild and fresh software and hardware validation.

## Licensing

The original project contribution is available under the [MIT License](LICENSE), Copyright (c) 2026 DerCodeBastler. Commercial use, modifications and redistribution are permitted while retaining license and copyright notices. Changes need not be published. Third-party components retain their own licenses: [Pico SDK](Lizenzen/Pico_SDK.txt), [TinyUSB](Lizenzen/TinyUSB.txt).

## Contact

Use this repository's issues for questions, bugs and suggestions. While private, participants need repository access. GitHub has no general private direct-message inbox. The noreply address is not a contact address. For vulnerabilities see [SECURITY.md](SECURITY.md).
