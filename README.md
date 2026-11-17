# Passwort-Button

**Ein Tastendruck statt Passwort eintippen.** Ein Raspberry Pi Pico 2 speichert ein Passwort lokal und gibt es nach einem kurzen Druck auf BOOTSEL als USB-Tastatur aus. Für Passwortmanager, Anmeldemasken und andere Texteingabefelder – KeePass ist nur ein Anwendungsbeispiel.

[English documentation](README.en.md) · [Einrichtung](Anleitung.txt) · [Mitarbeiten](CONTRIBUTING.md) · [Sicherheit](SECURITY.md)

## Projektstand

**Öffentliches Open-Source-Projekt** `DerCodeBastler/Passwort-Button`, veröffentlicht am 17.11.2026. Firmware **1.1.0** bleibt bytegleich zum gelieferten ZIP; nur Dokumentation, sichtbare Einrichtungstexte und Entwicklungsunterstützung wurden überarbeitet. Die Einrichtungsoberfläche ist deutsch; eine englische Anleitung liegt bei.

## Funktionen und Voraussetzungen

| Merkmal | Umfang |
| --- | --- |
| Hardware | Raspberry Pi Pico 2 / RP2350, USB-Datenkabel; kein zusätzlicher Taster |
| Bedienung | Eingebauter BOOTSEL-Taster, LED-Rückmeldung |
| Ausgabe | Ein gespeichertes Passwort, optional Enter |
| Tempo | 3–50 ms Pause nach jeder abgeschlossenen HID-Meldung, dauerhaft gespeichert |
| Zeichen | Bis 128 unterstützte Zeichen; deutsches Windows-Tastaturlayout |
| Einrichtung | Lokale HTML-Datei in Chrome/Edge am Windows-PC, Web Serial |
| Betrieb | Nach Einrichtung als USB-Tastatur, Browser muss nicht offen bleiben |
| Entwicklung | C-Firmware, Hosttests und vorkompilierte UF2 enthalten |

Keine FIDO2-/Passkey-Funktion, keine Anwendungserkennung: Der Text landet im **aktuell fokussierten Eingabefeld**. Andere Boards und Tastaturlayouts sind nicht als unterstützt ausgewiesen.

## Schnellstart

1. Paket vollständig entpacken. Pico 2 abziehen, BOOTSEL halten, USB verbinden und Taster loslassen.
2. [`KeePass_Pico2_3ms.uf2`](KeePass_Pico2_3ms.uf2) auf das Laufwerk `RP2350` kopieren. Das Gerät startet neu.
3. [`Passwort_einrichten.html`](Passwort_einrichten.html) lokal in Chrome/Edge öffnen, „Pico verbinden“ wählen. Das Gerät heißt weiterhin „KeePass-Taster Pico 2“.
4. Zunächst ein **unverfängliches Testpasswort** zweimal eingeben. Tempo und optionales Enter wählen.
5. Am verbundenen Pico BOOTSEL etwa **2 Sekunden halten**, bis die LED doppelt blinkt. Loslassen und innerhalb von 60 Sekunden speichern. Eine Freigabe erlaubt eine gültige Schreibaktion.
6. Im Editor testen: deutsches Windows-Layout wählen, Eingabefeld fokussieren, BOOTSEL kurz drücken und loslassen. Erst danach ein echtes Passwort lokal einrichten.

Tempo und Enter können ohne erneute Passworteingabe geändert werden. Auch dafür ist die lange Tastengeste nötig. Ein kurzer Klick bei offener Einrichtungsfreigabe schließt sie, ohne zu tippen. Details zu LED, Löschen und Fehlersuche: [Anleitung](Anleitung.txt) und [FAQ](docs/FAQ.md).

## Sicherheit und Datenschutz

Im normalen Firmwarebetrieb gibt es keinen USB-Befehl zum Zurücklesen des Passworts oder Starten der Ausgabe. **Der angeschlossene PC kann das Passwort bei Einrichtung und Ausgabe mitlesen.** Das Gerät speichert unverschlüsselte Tastencodes; physischer Zugriff, Debugging und fremde Firmware werden nicht verhindert. Während der Einrichtungsfreigabe kann auch andere PC-Software Schreibbefehle senden.

Keine Cloud und keine Netzwerkaufrufe in der Einrichtungsseite. Ein Firmwareupdate löscht das gespeicherte Passwort nicht notwendigerweise. Keine realen Passwörter, Flash-Abbilder oder persönlichen Logs hochladen. Dies ist ein Hobbyprojekt, kein zertifizierter Sicherheitsschlüssel. Siehe [Sicherheitsmodell](SECURITY.md).

## Entwicklung und Nachweise

- [Build und Tests – Deutsch / English](docs/DEVELOPMENT.md)
- [Prüfung dieser Aufbereitung](docs/REVIEW-2026-11-17.md)
- [Historischer Firmware-Prüfbericht](Pruefbericht.txt) vom 10.11.2026; kein neuer Hardwaretest
- [SHA-256 aller Paketdateien](SHA256.txt)
- [Änderungen](CHANGELOG.md) und [Beitragsregeln](CONTRIBUTING.md)

Der alte UF2-Dateiname, USB-Produktname, Build-Zielname und interne Kennungen bleiben absichtlich erhalten. Ein neues Branding innerhalb der Firmware benötigt einen separaten Neubau und erneute Software- und Hardwareprüfung.

## Lizenz

Der eigene Projektanteil steht unter der [MIT-Lizenz](LICENSE), Copyright (c) 2026 DerCodeBastler. Nutzung, Änderung und Weitergabe sind auch kommerziell erlaubt; Lizenz- und Copyright-Hinweise müssen erhalten bleiben. Änderungen müssen nicht veröffentlicht werden. Drittanbieter behalten ihre eigenen Lizenzen: [Pico SDK](Lizenzen/Pico_SDK.txt), [TinyUSB](Lizenzen/TinyUSB.txt).

## Kontakt

Fragen, Fehler und Vorschläge bitte über die Issues dieses Repositories. GitHub bietet keinen allgemeinen privaten Direktnachrichten-Posteingang. Die Noreply-Adresse ist keine Kontaktadresse. Sicherheitslücken: [SECURITY.md](SECURITY.md).
