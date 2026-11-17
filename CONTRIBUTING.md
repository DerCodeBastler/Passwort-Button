# Mitarbeit / Contributing

## Deutsch

Fehlerberichte, Dokumentationsverbesserungen und nachvollziehbare Pull Requests sind willkommen. Das Repository soll zunächst privat bleiben. Die MIT-Lizenz gilt für den eigenen Projektanteil; die öffentliche Freigabe steht noch aus. Siehe [Lizenzhinweise](docs/LICENSING.md).

1. Problem und erwartetes Verhalten in einem Issue beschreiben. Nur künstliche Testdaten verwenden.
2. Einen eigenen Branch erstellen, die Änderung klein halten und Dokumentation in Deutsch und Englisch aktualisieren.
3. [Lokale Prüfungen](docs/DEVELOPMENT.md) ausführen. Bei Codeänderungen relevante Fehlerfälle ergänzen; reine Textkorrekturen benötigen keine künstlichen Verhaltenstests.
4. Im Pull Request Änderungen, Testbefunde und verbleibende Grenzen angeben. Hardwaretest und Simulation ausdrücklich unterscheiden.
5. Firmware, USB-Deskriptoren, Build-Einstellungen oder UF2 nicht beiläufig ändern. Eine neue Firmware benötigt Review, Softwaretests, UF2-Prüfung und einen dokumentierten Test am Pico 2, bevor sie das bewährte Binärartefakt ersetzt.

Der Maintainer prüft Beiträge vor dem Merge. Kein automatischer Merge und keine automatische Veröffentlichung. Zugangsdaten oder persönliche Kontaktdaten gehören nicht ins Projekt. Prüfsummen nach akzeptierten Änderungen mit `python3 scripts/check_package.py --write-manifest` erneuern; anschließend ohne Option prüfen. Für eine neue Firmware muss zusätzlich die freigegebene Referenz im Prüfsystem bewusst aktualisiert werden.

## English

Bug reports, documentation improvements and focused pull requests are welcome. The repository is intended to stay private initially. The original project contribution is MIT-licensed; public release is pending. See [license notes](docs/LICENSING.md).

1. Explain the problem and expected behavior in an issue using artificial test data.
2. Create a branch, keep changes focused, and update German and English documentation.
3. Run the [local checks](docs/DEVELOPMENT.md). Add relevant failure cases for code changes; prose edits do not need artificial behavioral tests.
4. Describe changes, evidence and limitations in the pull request. Clearly distinguish simulated tests from real hardware tests.
5. Do not casually change firmware, USB descriptors, build settings or the UF2. Replacement firmware needs review, software tests, UF2 checks and a documented Pico 2 hardware test before replacing the known binary.

The maintainer reviews contributions before merging. There is no automatic merge or publishing. Never include credentials or personal contact information. Refresh checksums after accepted changes with `python3 scripts/check_package.py --write-manifest`, then run without options. New firmware also requires an intentional update of the approved reference in the checker.
