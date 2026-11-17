"""Integrity checks for the reviewed package; not a hardware/security audit."""
from pathlib import Path
import hashlib
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
FIRMWARE_SHA256 = '7d0e74f256f98645176e88ffe62b8851f3941690c1f2d46540b4d84e0b79aab8'

def package_files():
    excluded = {'.git', 'node_modules', '__pycache__', 'build', 'pico-sdk', '.cache'}
    return sorted(p for p in ROOT.rglob('*') if p.is_file()
                  and not excluded.intersection(p.relative_to(ROOT).parts)
                  and p.name != 'SHA256.txt')

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def check():
    data = (ROOT / 'KeePass_Pico2_3ms.uf2').read_bytes()
    assert hashlib.sha256(data).hexdigest() == FIRMWARE_SHA256, 'Reference firmware changed'
    assert len(data) == 64512 and len(data) % 512 == 0
    blocks = []
    compat = 0
    for off in range(0, len(data), 512):
        m0, m1, flags, address, size, num, total, family = struct.unpack_from('<8I', data, off)
        assert (m0, m1) == (0x0A324655, 0x9E5D5157)
        assert struct.unpack_from('<I', data, off+508)[0] == 0x0AB16F30
        assert size == 256
        if family == 0xe48bff59:
            assert flags == 0x2000
            assert 0x10000000 <= address < address+size <= 0x103ff000, 'Password sector overlap'
            blocks.append((num, total, address))
        else:
            assert family == 0xe48bff57 and flags == 0xa000 and address == 0x10ffff00
            compat += 1
    assert len(blocks) == 125 and compat == 1
    assert [b[0] for b in blocks] == list(range(125))
    assert all(b[1] == 125 for b in blocks)
    assert [b[2] for b in blocks] == list(range(0x10000000, 0x10000000+125*256, 256))
    print('PASS: reference UF2 unchanged; 125 RP2350 blocks + compatibility block; no password sector')

    manifest = ROOT / 'SHA256.txt'
    if sys.argv[1:] == ['--write-manifest']:
        manifest.write_text(''.join(f'{digest(p)}  {p.relative_to(ROOT).as_posix()}\n' for p in package_files()))
        print('Manifest updated; rerun without arguments to verify')
        return
    assert len(sys.argv) == 1, 'Unsupported arguments'
    entries = {}
    for line in manifest.read_text().splitlines():
        sha, name = line.split('  ', 1)
        assert name not in entries, 'Duplicate manifest entry'
        assert len(sha) == 64 and all(c in '0123456789abcdef' for c in sha)
        entries[name] = sha
    files = {p.relative_to(ROOT).as_posix(): p for p in package_files()}
    assert entries.keys() == files.keys(), 'Manifest does not match package file set'
    for name, p in files.items():
        assert digest(p) == entries[name], f'Checksum mismatch: {name}'
    print(f'PASS: {len(entries)} package checksums')

if __name__ == '__main__':
    check()
