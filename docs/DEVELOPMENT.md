# Entwicklung / Development

## Prüfungen / Checks

Vom Repository-Hauptordner / from the repository root, with GCC, Python 3 and Node.js available:

```sh
python3 scripts/check_package.py
python3 Quellcode/tests/test_protocol.py
node Quellcode/tests/test_setup.cjs
```

Der Paketprüfer kontrolliert das Manifest, die Referenz-Firmware und die UF2-Blockadressen. Der Firmwaretest kompiliert den tatsächlichen C-Code mit simuliertem USB/Flash/Taster und ASan/UBSan. Der Seitentest führt das echte JavaScript mit einem Web-Serial-Testdouble aus. Der vorbereitete GitHub-Actions-Workflow führt diese drei Prüfungen aus; er baut und veröffentlicht keine Firmware.

The package checker verifies the manifest, reference firmware and UF2 block addresses. Firmware host tests compile actual C code with simulated USB/flash/button hardware and ASan/UBSan. Setup tests execute the actual JavaScript with a Web Serial test double. The prepared GitHub Actions workflow runs these three checks; it does not build or publish firmware.

## Firmware bauen / Build firmware

Die originale Anleitung bleibt in [Quellcode/BUILD.txt](../Quellcode/BUILD.txt). Original dependencies: Pico SDK 2.2.0 at `a1438dff1d38bd9c65dbd693f0e5db4b9ae91779`, TinyUSB at `86ad6e56c1700e85f1c5678607a762cfe3aa2f47`, Picotool 2.2.0 at `a7eb3988f0645239185fadb4e25d8279478c2dbb`. Original compiler: GNU Arm Embedded GCC 13.2.1; CMake 3.28.3, Ninja, Release.

```sh
cd Quellcode
git clone --branch 2.2.0 https://github.com/raspberrypi/pico-sdk.git
git -C pico-sdk checkout a1438dff1d38bd9c65dbd693f0e5db4b9ae91779
git -C pico-sdk submodule update --init lib/tinyusb
cmake -S firmware -B build -G Ninja -DPICO_BOARD=pico2 -DPICO_PLATFORM=rp2350-arm-s -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
```

Install the specified toolchain and Picotool first; provide `picotool_DIR` if necessary. Verify TinyUSB's checkout against the recorded revision. Build outputs retain the legacy `keepass_pico` name. Do not replace the bundled UF2 automatically. Byte-for-byte reproducible rebuilds are not promised.

`tests/verify_uf2.py build` additionally requires matching `keepass_pico.bin`, `.elf` and `.dis` files and `arm-none-eabi-nm`. Produce the disassembly with `arm-none-eabi-objdump -d build/keepass_pico.elf > build/keepass_pico.dis` before this extended verification. These matching build artifacts are not included in the ZIP. The lightweight package check does not replace the extended verification.

## Optionaler Browsertest / Optional browser test

`node Quellcode/tests/test_browser.cjs` requires Playwright resolvable by Node and Chromium (default `/usr/bin/chromium`, override with `CHROMIUM_PATH`). It still uses simulated Web Serial and is not a real USB test. Do not commit node_modules or screenshots containing actual passwords.

## Hardware vor Firmwarefreigabe / Hardware before firmware release

Mit künstlichem Passwort prüfen / use an artificial password:

- USB enumeration and German character output, including umlauts and dead keys.
- Short press outputs once; long press unlocks without output; expiry and one-write limit.
- Timing endpoints 3/50 ms, optional Enter and persistence after power cycling.
- Disconnect/suspend abort; no spontaneous output on reconnect.
- Delete, empty status and LED feedback.

Record board, OS/browser, firmware hash, test procedure, outcomes and limitations. Never claim a physical test from host-test results alone.
