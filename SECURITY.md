# Sicherheit / Security

## Deutsch

Geltungsbereich: mitgelieferte Firmware 1.1.0 und Einrichtungsseite. Es gibt keine zugesagte Supportfrist und keinen unabhängigen Sicherheitsaudit.

- Normalbetrieb: keine USB-Befehle zum Lesen des gespeicherten Passworts oder Auslösen der Ausgabe. Die Ausgabe benötigt einen kurzen physischen BOOTSEL-Klick.
- Änderungen: etwa zwei Sekunden BOOTSEL öffnen ein Fenster von maximal 60 Sekunden für eine gültige Schreibaktion. Andere Software auf dem PC kann dieses Fenster ebenfalls nutzen. Es authentifiziert keine bestimmte Anwendung.
- Grenzen: PC oder Browser können Eingabe und Tastaturausgabe mitlesen. Die Tastencodes liegen unverschlüsselt im Flash. Physischer Zugriff, SWD, Bootloader und ausgetauschte Firmware werden nicht abgewehrt. Secure Boot/OTP-Sperren werden nicht eingerichtet.
- Datenpflege: Firmwareupdates sind keine sichere Löschung. Keine Passwörter, Flash-Dumps, persönliche Seriennummern oder ungefilterten Logs in Commits, Issues oder Anhänge aufnehmen. Testwerte müssen künstlich sein.

Sicherheitslücken bitte über GitHubs **„Report a vulnerability“** melden, sofern der Inhaber diese Funktion vor der öffentlichen Freigabe aktiviert hat. Falls sie nicht verfügbar ist, zunächst nur um einen privaten Meldeweg bitten, ohne technische Angriffsschritte oder Geheimnisse öffentlich zu posten. Der Meldeweg ist in diesem Paket vorbereitet, nicht serverseitig aktiviert. Keine Reaktionszeit zugesichert.

## English

Scope: bundled firmware 1.1.0 and setup page. No guaranteed support term or independent security audit.

- Normal operation offers no USB command to read stored passwords or trigger typing. Output requires a short physical BOOTSEL press.
- Holding BOOTSEL for about two seconds opens a window of at most 60 seconds for one valid write. Other host software can use that window; it does not authenticate the configuration application.
- The host/browser can capture setup input and keyboard output. Flash stores unencrypted keycodes. Physical access, SWD, bootloader access and replacement firmware are not prevented. Secure Boot and OTP locking are not enabled.
- Firmware updates do not guarantee erasure. Never commit passwords, flash dumps, personal serial numbers or unfiltered logs. Use artificial test values.

Use GitHub's **“Report a vulnerability”** if the owner has enabled it before public release. Otherwise request a private reporting channel without publishing exploit details or secrets. This package documents the process but does not enable the server-side feature. No response-time commitment is made.
