#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TRZ - build-time compression of the zero-terminated UI strings (STR_xxx).

Used by radio/src/CMakeLists.txt when TRZ=YES (two-pass link):

  pass 1   firmware_pass1.elf is linked normally. --gc-sections already removed
           every string this configuration does not use, and the remaining
           STR_ objects contain the final bytes (after translate.py).
  trz.py   reads firmware_pass1.elf, selects the strings that are only ever
           drawn through the LCD text primitives, BPE-compresses them with the
           byte codes 0x80..0xFF that these strings do not use, stores them in
           one blob with suffix sharing, and writes:
             trz_data.c     TRZ_BLOB, TRZ_DICT and link-time aliases
                            "STR_X = TRZ_BLOB + offset" (no object is recompiled)
             trz_rename.h   renames the original STR_X definitions in
                            translations.cpp so the linker drops them
             trz_report.txt what was compressed / excluded and why
  final    the same objects + translations.cpp (TRZ_FINAL) + trz_data.c.

The decoder is trzResolve() in radio/src/gui/128x64/lcd.cpp; getTextWidth() and
lcdDrawSizedText() call it, so every lcdDrawText* path handles both kinds of
strings.

Rules that keep it safe:
  * a string is eligible only if every use in the sources compiled for 128x64 is
    an argument of a known draw function (WHITELIST); pointer arithmetic,
    indexing, assignment to variables or any other call excludes it;
  * strings compared by pointer (popup results, `x == STR_Y`) never share a
    start address with another such string;
  * decoded length <= TRZ_MAXLEN (stack buffer in the decoder, from translations.h).

usage: trz.py --elf firmware_pass1.elf --src radio/src --out <dir> [--mincnt 3]
"""

import argparse
import collections
import os
import re
import struct
import sys

WHITELIST = {
    'lcdDrawText', 'lcdDrawTextAlignedLeft', 'lcdDrawTextIndented', 'lcdDrawSizedText',
    'POPUP_MENU_ADD_ITEM', 'POPUP_MENU_TITLE', 'POPUP_MENU_START', 'POPUP_WARNING',
    'POPUP_CONFIRMATION', 'editChoice', 'editCheckBox', 'drawStringWithIndex',
    'ON_OFF_MENU_ITEM', 'SIMPLE_MENU', 'MENU', 'TITLE', 'SUBMENU', 'SIMPLE_SUBMENU',
    'ALERT', 'RAISE_ALERT', 'showMessageBox', 'editDelay', 'drawProgressBar',
    'drawAlertBox', 'editSingleName', 'title', 'slider_5pos',
}
# directories / files that are not compiled for the 128x64 firmware
EXCLUDED_PATHS = re.compile(r'gui/212x64|gui/480x272|colorlcd|horus|taranis|sky9x|tests/|'
                            r'companion|/lua/|targets/simu|bootloader|translations')
TRANSLATION_SOURCES = re.compile(r'(^|/)translations(\.h|\.cpp|/[a-z_]+\.h\.txt)$')
STR_TOKEN = re.compile(r'\bS?STR_[A-Z0-9_]+\b')


# ----------------------------------------------------------------------------- ELF
def read_elf_strings(path):
    """{symbol: bytes} for all STR_* data objects of a 32-bit little-endian ELF"""
    data = open(path, 'rb').read()
    if data[:4] != b'\x7fELF' or data[4] != 1 or data[5] != 1:
        sys.exit('trz.py: %s is not a 32-bit little-endian ELF' % path)
    shoff, = struct.unpack_from('<I', data, 0x20)
    shentsize, shnum = struct.unpack_from('<HH', data, 0x2E)
    sections = [struct.unpack_from('<10I', data, shoff + i * shentsize) for i in range(shnum)]
    result = {}
    for sh in sections:
        if sh[1] != 2:          # SHT_SYMTAB
            continue
        strtab = sections[sh[6]]
        for off in range(sh[4], sh[4] + sh[5], 16):
            st_name, st_value, st_size, st_info, _, st_shndx = struct.unpack_from('<IIIBBH', data, off)
            if st_size == 0 or (st_info & 0xF) != 1 or st_shndx == 0 or st_shndx >= 0xFF00:
                continue        # not a defined data object
            end = data.index(b'\0', strtab[4] + st_name)
            name = data[strtab[4] + st_name:end].decode()
            if not re.match(r'^STR_[A-Z0-9_]+$', name):
                continue
            sec = sections[st_shndx]
            if sec[1] == 8:     # SHT_NOBITS
                continue
            pos = sec[4] + st_value - sec[3]
            result[name] = data[pos:pos + st_size]
    return result


# ----------------------------------------------------------------------------- sources
def scan_uses(src):
    uses = collections.defaultdict(list)
    arith = set()
    for dirpath, _, files in os.walk(src):
        for f in files:
            if not f.endswith(('.cpp', '.h', '.h.txt')):
                continue
            path = os.path.join(dirpath, f)
            rel = os.path.relpath(path, src).replace(os.sep, '/')
            text = open(path, encoding='utf-8', errors='replace').read()
            if TRANSLATION_SOURCES.search(rel):
                # translation files only matter for arithmetic like
                # "#define STR_US (STR_TMR1LATMAXUS+13)"
                for m in re.finditer(r'\b(STR_[A-Z0-9_]+)\s*(\+|-(?!>)|\[(?!\s*\]))', text):
                    arith.add(m.group(1))
                continue
            if EXCLUDED_PATHS.search(rel) or f.endswith('.h.txt'):
                continue
            for line in text.split('\n'):
                for m in STR_TOKEN.finditer(line):
                    uses[m.group()].append(line)
    return uses, arith


def classify_use(name, line):
    """returns 'ok', 'compare' or a reason string for exclusion"""
    if re.search(r'(==|!=)\s*%s\b|\b%s\s*(==|!=)' % (name, name), line):
        return 'compare'
    if re.search(r'\bwarningText\s*=\s*%s\s*;' % name, line):
        return 'ok'
    for _ in range(3):  # unwrap "(cond ? A : B)" so A/B count as direct arguments
        line = re.sub(r'(?<![\w\s])\s*\(([^()]*\?[^()]*)\)', r' \1', line)
    line = re.sub(r',\s*\(([^()]*\?[^()]*)\)', r', \1', line)
    if re.search(r'\b%s\s*(\+|\[|-)' % name, line) or re.search(r'(\+|-)\s*%s\b' % name, line):
        return 'pointer arithmetic'
    call = None
    for m in re.finditer(r'(\w+)\s*\(([^()]*)', line):
        if re.search(r'\b%s\b' % name, m.group(2)):
            call = m.group(1)
    if call in WHITELIST:
        return 'ok'
    return 'used outside a draw call: ' + line.strip()[:70]


# ----------------------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--elf', required=True)
    ap.add_argument('--src', required=True, help='radio/src directory')
    ap.add_argument('--out', required=True)
    ap.add_argument('--mincnt', type=int, default=3, help='minimum pair count for a BPE token')
    args = ap.parse_args()

    hdr = open(os.path.join(args.src, 'translations.h'), encoding='utf-8').read()
    maxlen = int(re.search(r'#define\s+TRZ_MAXLEN\s+(\d+)', hdr).group(1))
    cpp = open(os.path.join(args.src, 'translations.cpp'), encoding='utf-8').read()
    defined = set(re.findall(r'^\s*const char (STR_[A-Z0-9_]+)\[\]\s*=', cpp, re.M))

    live = read_elf_strings(args.elf)
    uses, arith = scan_uses(args.src)

    elig, compared, excluded = {}, set(), {}
    for name in sorted(live):
        raw = live[name]
        reason = None
        if name not in defined:
            reason = 'not a plain string in translations.cpp'
        elif not raw.endswith(b'\0') or b'\0' in raw[:-1] or len(raw) < 2:
            reason = 'not a single zero-terminated string'
        elif len(raw) - 1 > maxlen:
            reason = 'longer than TRZ_MAXLEN'
        elif name in arith:
            reason = 'pointer arithmetic in translations'
        elif name not in uses:
            reason = 'no use found in the 128x64 sources'
        else:
            for line in uses[name]:
                r = classify_use(name, line)
                if r == 'compare':
                    compared.add(name)
                elif r != 'ok':
                    reason = r
                    break
        if reason:
            excluded[name] = reason
        else:
            elig[name] = raw[:-1]
    popup = {n for n in elig if n in compared or any('POPUP_MENU' in l for l in uses[n])}

    # BPE with the byte codes 0x80..0xFF not used by any eligible string
    used = set(b''.join(elig.values()))
    free_codes = [c for c in range(0x80, 0x100) if c not in used]
    uniq = sorted(set(elig.values()))
    corpus = [bytearray(s) for s in uniq]
    dic = {}
    for code in free_codes:
        cnt = collections.Counter()
        for s in corpus:
            for i in range(len(s) - 1):
                cnt[(s[i], s[i + 1])] += 1
        if not cnt:
            break
        (a, b), n = cnt.most_common(1)[0]
        if n < args.mincnt:
            break
        dic[code] = (a, b)
        for s in corpus:
            i = 0
            while i < len(s) - 1:
                if s[i] == a and s[i + 1] == b:
                    s[i:i + 2] = bytes([code])
                i += 1
    enc = dict(zip(uniq, map(bytes, corpus)))

    def dec(b):
        out = bytearray()
        for c in b:
            out += dec(bytes(dic[c])) if c in dic else bytes([c])
        return bytes(out)
    for u in uniq:
        assert dec(enc[u]) == u, u

    # blob with suffix sharing; pointer-compared strings keep a unique start
    blob, offsets, popup_starts = bytearray(), {}, set()
    for name, raw in sorted(elig.items(), key=lambda kv: (-len(enc[kv[1]]), kv[0])):
        e = enc[raw] + b'\0'
        i = blob.find(e)
        while i >= 0 and name in popup and i in popup_starts:
            i = blob.find(e, i + 1)
        if i < 0:
            i = len(blob)
            blob += e
        offsets[name] = i
        if name in popup:
            popup_starts.add(i)
    for name, raw in elig.items():  # final self-check on the real layout
        o = offsets[name]
        assert dec(blob[o:blob.index(b'\0', o)]) == raw, name

    dict_bytes = bytearray()
    for c in range(0x80, 0x100):
        dict_bytes += bytes(dic.get(c, (0, 0)))

    # ------------------------------------------------------------------ outputs
    os.makedirs(args.out, exist_ok=True)

    def c_string(b):
        return '"' + ''.join(chr(c) if 32 <= c < 127 and chr(c) not in '"\\?' else '\\%03o' % c
                             for c in b) + '"'

    def write_if_changed(path, text):
        if os.path.exists(path) and open(path).read() == text:
            return
        open(path, 'w').write(text)

    lines = ['/* generated by radio/util/trz.py - do not edit */', '',
             '#define TRZ_BLOB_SIZE %d' % max(len(blob), 1), '',
             'const char TRZ_BLOB[TRZ_BLOB_SIZE] =']
    body = bytes(blob[:-1]) if blob else b''
    for i in range(0, max(len(body), 1), 48):
        lines.append('  ' + c_string(body[i:i + 48]))
    lines[-1] += ';'
    lines += ['', 'const unsigned char TRZ_DICT[256] = {']
    for i in range(0, 256, 16):
        lines.append('  ' + ','.join(str(x) for x in dict_bytes[i:i + 16]) + ',')
    lines += ['};', '', '/* link-time aliases: references to STR_X resolve into the blob */', '__asm__(',
              '  "\\t.global TRZ_BLOB_END\\n\\t.set TRZ_BLOB_END, TRZ_BLOB+%d\\n"' % len(blob)]
    for name in sorted(offsets):
        lines.append('  "\\t.global %s\\n\\t.set %s, TRZ_BLOB+%d\\n"' % (name, name, offsets[name]))
    lines += [');', '']
    write_if_changed(os.path.join(args.out, 'trz_data.c'), '\n'.join(lines))

    ren = ['/* generated by radio/util/trz.py - do not edit */',
           '/* the original definitions are renamed; the linker drops them */']
    ren += ['#define %s %s_trz_unused' % (n, n) for n in sorted(offsets)]
    write_if_changed(os.path.join(args.out, 'trz_rename.h'), '\n'.join(ren) + '\n')

    raw_size = sum(len(v) + 1 for v in elig.values())
    rep = ['TRZ report for %s' % args.elf,
           'strings in pass-1 image: %d, compressed: %d, excluded: %d' % (len(live), len(elig), len(excluded)),
           'raw %d B -> blob %d B + dict 256 B (%d tokens, mincnt %d), saving before decoder %d B'
           % (raw_size, len(blob), len(dic), args.mincnt, raw_size - len(blob) - 256),
           'pointer-compared (unique start): %s' % ' '.join(sorted(popup)), '', '[compressed]']
    rep += ['  %-32s @%-5d %r' % (n, offsets[n], elig[n]) for n in sorted(elig)]
    rep += ['', '[excluded]'] + ['  %-32s %s' % (n, r) for n, r in sorted(excluded.items())]
    write_if_changed(os.path.join(args.out, 'trz_report.txt'), '\n'.join(rep) + '\n')
    print('TRZ: %d strings, %d B -> %d B blob + 256 B dict' % (len(elig), raw_size, len(blob)))


if __name__ == '__main__':
    main()
