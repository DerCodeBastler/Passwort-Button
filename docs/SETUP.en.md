# Setup and operation

[Back to English README](../README.en.md)

## Install and connect

Use a Raspberry Pi Pico 2 and a USB cable that carries data. Hold BOOTSEL while connecting USB, release it, and copy `KeePass_Pico2_3ms.uf2` onto the `RP2350` drive. The device restarts as a USB keyboard plus a serial setup interface; no drive is expected during normal operation.

Open `Passwort_einrichten.html` locally in Chrome or Edge on Windows. Click “Pico verbinden” and select “KeePass-Taster Pico 2”. The connection button turns green after the protocol check. An existing password is never read back.

## Configure

Start with a harmless test string. Enter it in both password fields and choose the timing and Enter option. Hold BOOTSEL for about two seconds while connected until the LED double-blinks, then release. Click the save button within 60 seconds. A successful write closes the window; further changes require another long press.

| German control | Meaning |
| --- | --- |
| Neues Passwort | New password |
| Passwort wiederholen | Repeat password |
| Geschwindigkeit | Delay between completed HID reports |
| Nach dem Passwort Enter drücken | Append Enter |
| Passwort und Einstellungen speichern | Save password, timing and Enter |
| Tempo und Enter speichern | Update timing and Enter, keeping password |
| Gespeichertes Passwort löschen | Expand deletion controls |
| Ja, Passwort löschen | Confirm deletion after physical unlock |

The page accepts at most 128 supported characters: printable ASCII and ä ö ü Ä Ö Ü ß § € ° µ ² ³ ´. No arbitrary Unicode or emoji. Select **German (Germany)** as the Windows keyboard layout. The English documentation does not change the device's layout. Dead keys (^, ´, `) include a following space to output the accent itself.

## Daily use

Focus the intended input field and briefly press and release BOOTSEL. The device types once, optionally followed by Enter. It cannot tell whether the focused field is a password box, chat window or terminal. Test with a harmless value in an editor before using a real password.

The configured 3–50 ms delay applies to individual completed HID reports. A normal character requires press and release reports; 3 ms is not three milliseconds per character. Increase the value if characters are missed.

| LED / action | Meaning |
| --- | --- |
| Brief flash every two seconds | No password stored |
| Double blinking | Configuration unlocked |
| Lit during typing | Output active |
| Briefly lit after saving | Save feedback |
| Short click while unlocked | Close configuration without typing |

USB disconnection or suspend aborts output; the remaining text is not resumed. The firmware preserves supported legacy V1 records in memory, using 3 ms and extracting trailing Enter into its own option. Saving writes V2. Unknown or damaged records do not trigger password output.

## Delete and troubleshoot

Connect, expand deletion controls, physically unlock, choose delete and confirm. Timing remains; Enter is disabled. This is not a guarantee of forensic erasure. Flashing another UF2 does not necessarily erase the password sector.

If no device appears, check the data cable, board and flashed firmware. Close another application using its serial port. If saving says “Noch gesperrt”, repeat the long press and save before the window expires. If characters are incorrect, check the German keyboard layout; if characters are missing, increase the delay. A protocol mismatch requires the matching setup page and firmware. Do not overwrite a working installation just to change the visible project name.

See [SECURITY.md](../SECURITY.md) for host and physical-access limitations.
