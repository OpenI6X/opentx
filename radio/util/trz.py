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
    a text argument: a parameter declared trz_text/trz_item (opentx_types.h), a
    macro that passes it on to one, or an assignment to a variable declared that
    way. Pointer arithmetic, indexing, other variables or any other call exclude
    it. The marks are read from the declarations (see "marks" below), and the
    marked functions are checked to only pass such parameters on to other marks;
  * strings compared by pointer (`x == STR_Y`, trz_item: popup items/results)
    never share a start address with another such string;
  * decoded length <= TRZ_MAXLEN (stack buffer in the decoder, from translations.h).

usage: trz.py --elf firmware_pass1.elf --src radio/src --out <dir> [--mincnt 3]
"""

import argparse
import collections
import os
import re
import struct
import sys

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


def classify_use(name, line, marks, macro_param=False):
    """'text', 'item', 'compare' or a reason string for exclusion. 'text'/'item':
    every use of name in line is a marked argument or an assignment to a marked
    variable ('item' if any of them is a trz_item)."""
    if line.lstrip().startswith(('//', '/*', '* ')):
        return 'text'  # commented-out code is not a use
    if re.search(r'(==|!=)\s*%s\b|\b%s\s*(==|!=)' % (name, name), line):
        return 'compare'
    kinds = set()

    def assignment(m):  # "var = NAME;" / "var[i] = NAME;" with var a marked variable
        kind = marks.vars.get(m.group(1))
        if not kind:
            return m.group()
        kinds.add(kind)
        return ';'
    line = re.sub(r'\b([A-Za-z_]\w*)\s*(?:\[[^\]=]*\])?\s*=\s*%s\s*;' % name, assignment, line)
    for _ in range(3):  # unwrap "(cond ? A : B)" so A/B count as direct arguments
        line = re.sub(r'(?<![\w\s])\s*\(([^()]*\?[^()]*)\)', r' \1', line)
    line = re.sub(r',\s*\(([^()]*\?[^()]*)\)', r', \1', line)
    if re.search(r'\b%s\s*(\+|\[|-)' % name, line) or re.search(r'(\+|-)\s*%s\b' % name, line):
        return 'pointer arithmetic'
    if macro_param and re.search(r'\b%s\s*(\(|##)|##\s*%s\b|#\s*%s\b' % (name, name, name), line):
        return 'not a value: ' + line.strip()[:70]  # macro parameter used as a name
    for call, argn in enclosing_calls(name, line):
        kind = 'text' if call and DEBUG_CALL.match(call) else marks.kind(call, argn)
        if not kind:
            return 'not a text argument: ' + line.strip()[:70]
        kinds.add(kind)
    return 'item' if 'item' in kinds else 'text'


def enclosing_calls(name, line):
    """(innermost named call, 0-based argument index) for every use of name in line.
    Uses inside comments and string/char literals do not count; unnamed (grouping)
    parentheses are transparent. A use outside any call gives (None, None)."""
    res, stack, i, n = [], [], 0, len(line)
    while i < n:
        c = line[i]
        if c in '"\'':
            j = i + 1
            while j < n and line[j] != c:
                j += 2 if line[j] == '\\' else 1
            i = j + 1
            continue
        if line.startswith('//', i):
            break
        m = re.match(r'[A-Za-z_]\w*', line[i:])
        if m:
            tok = m.group()
            k = i + len(tok)
            while k < n and line[k] in ' \t':
                k += 1
            if k < n and line[k] == '(':
                stack.append([tok, 0])
                i = k + 1
                continue
            if tok == name:
                named = [e for e in stack if e[0] is not None]
                res.append((named[-1][0], named[-1][1]) if named else (None, None))
            i += len(tok)
            continue
        if c == '(':
            stack.append([None, 0])
        elif c == ')':
            if stack:
                stack.pop()
        elif c == ',':
            named = [e for e in stack if e[0] is not None]
            if stack and stack[-1][0] is None and named:
                pass  # comma inside a grouping paren (rare); keep the call's index
            elif stack:
                stack[-1][1] += 1
        i += 1
    return res



# ----------------------------------------------------------------------------- marks
# Which arguments may receive a compressed string is declared in the sources, not
# here: text parameters (and variables) of the 128x64 GUI are declared with the
# `const char *` aliases from opentx_types.h
#   trz_text      only ever drawn (lcdDrawSizedText()/getTextWidth() decode)
#   trz_item      also drawn only, but the pointer is kept and compared later
#                 (popup menu items): such strings never share a start address
#   TRZ_VA_ITEMS  in front of "...": every variadic argument is a trz_item
# Function-like macros inherit the marks of what they expand to
# (#define TITLE(str) title(str)), so they need no annotation.
# The definitions of the marked functions are checked: a marked parameter may only
# be tested against NULL, passed on to another mark or decoded (trzResolve); any
# other use (strlen, arithmetic, an unmarked call) stops the build. So making a new
# helper accept compressed strings = declaring its text parameter trz_text.
MARK_TYPES = {'trz_text': 'text', 'trz_item': 'item'}
MARK_TOKEN = re.compile(r'\b(trz_text|trz_item|TRZ_VA_ITEMS)\b')
BUILTIN_MARKS = {'TRZ_TEXT': {0: 'text'}}  # translations.h: explicit "only displayed" marker
NOT_FUNCTIONS = {'if', 'while', 'for', 'switch', 'return', 'sizeof', 'defined'}


class Marks:
    def __init__(self):
        self.calls = {k: dict(v) for k, v in BUILTIN_MARKS.items()}  # name: {pos: kind}
        self.va = {}       # name: (first variadic position, kind)
        self.vars = {}     # variable name: kind
        self.where = {}    # name: 'file:line' of the first mark (report)
        self.warnings = []
        self.checked = []  # 'file:line name(param)' of the checked definitions

    def kind(self, call, argn):
        if call is None or argn is None:
            return None
        k = self.calls.get(call, {}).get(argn)
        if k is None and call in self.va and argn >= self.va[call][0]:
            k = self.va[call][1]
        return k

    def describe(self):
        out = []
        for name in sorted(set(self.calls) | set(self.va)):
            pos = ['%d:%s' % (p, k) for p, k in sorted(self.calls.get(name, {}).items())]
            if name in self.va:
                pos.append('%d..:%s' % self.va[name])
            out.append('  %-28s %-22s %s' % (name, ' '.join(pos), self.where.get(name, '')))
        for name in sorted(self.vars):
            out.append('  %-28s %-22s %s' % (name, 'var:' + self.vars[name], self.where.get(name, '')))
        return out


CODE_NOISE = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)


def strip_code(text):
    """comments -> spaces, string/char literal contents dropped, newlines kept"""
    def repl(m):
        t = m.group()
        if t[0] in '"\'':
            return t[0] * 2
        return ' ' + '\n' * t.count('\n') if t.startswith('/*') else ''
    return CODE_NOISE.sub(repl, text)


def split_args(s):
    """split at top-level commas"""
    args, depth, cur = [], 0, ''
    for c in s:
        if c in '([{':
            depth += 1
        elif c in ')]}':
            depth -= 1
        if c == ',' and depth == 0:
            args.append(cur)
            cur = ''
        else:
            cur += c
    args.append(cur)
    return [a.strip() for a in args]


def iter_sources(src):
    for dirpath, _, files in os.walk(src):
        for f in sorted(files):
            if not f.endswith(('.cpp', '.h')):
                continue
            rel = os.path.relpath(os.path.join(dirpath, f), src).replace(os.sep, '/')
            if EXCLUDED_PATHS.search(rel) or TRANSLATION_SOURCES.search(rel):
                continue
            yield rel, strip_code(open(os.path.join(src, rel), encoding='utf-8', errors='replace').read())


def split_preprocessor(code):
    """(code with preprocessor lines blanked, [(line, directive)])"""
    lines, directives, cont = code.split('\n'), [], None
    for i, line in enumerate(lines):
        if cont is not None or line.lstrip().startswith('#'):
            if cont is None:
                directives.append([i + 1, line])
            else:
                directives[-1][1] += ' ' + line
            cont = True if line.rstrip().endswith('\\') else None
            if cont:
                directives[-1][1] = directives[-1][1].rstrip()[:-1]
            lines[i] = ''
    return '\n'.join(lines), directives


def find_open_paren(code, pos):
    """index of the '(' that encloses pos, or None at statement level"""
    depth = 0
    for i in range(pos - 1, -1, -1):
        c = code[i]
        if c == ')':
            depth += 1
        elif c == '(':
            if depth == 0:
                return i
            depth -= 1
        elif c in ';{}' and depth == 0:
            return None
    return None


def matching(code, i, open_c, close_c):
    depth = 0
    for j in range(i, len(code)):
        if code[j] == open_c:
            depth += 1
        elif code[j] == close_c:
            depth -= 1
            if depth == 0:
                return j
    return None


def null_tests(p, line):
    """replaces tests of p against NULL (`if (p)`, `!p`, `p ? a : b`, `p && ...`)
    with 0: they work with compressed strings too"""
    p = r'\b%s\b' % p
    line = re.sub(r'\b(if|while)\s*\(\s*!?\s*%s\s*\)' % p, r'\1 (0)', line)
    line = re.sub(r'\(\s*!?\s*%s\s*\)\s*\?' % p, '0 ?', line)
    line = re.sub(r'!\s*%s' % p, '0', line)
    line = re.sub(r'%s\s*(\?|&&|\|\|)' % p, r'0 \1', line)
    line = re.sub(r'(&&|\|\|)\s*%s' % p, r'\1 0', line)
    return line


DEBUG_CALL = re.compile(r'(TRACE|DEBUG)\w*$')  # debug output: shows raw bytes, harmless


def scan_marks(src):
    marks = Marks()
    macros = collections.defaultdict(list)   # name: [(params, body, where)]
    files = []
    for rel, code in iter_sources(src):
        code, directives = split_preprocessor(code)
        files.append((rel, code))
        for line, d in directives:
            m = re.match(r'\s*#\s*define\s+([A-Za-z_]\w*)\(([^)]*)\)(.*)', d)
            if m:
                macros[m.group(1)].append((split_args(m.group(2)), m.group(3), '%s:%d' % (rel, line)))
        done = set()
        for m in MARK_TOKEN.finditer(code):
            where = '%s:%d' % (rel, code.count('\n', 0, m.start()) + 1)
            op = find_open_paren(code, m.start())
            if op is None:  # variable: "extern trz_text warningText;"
                v = re.match(r'\s*(?:const\s+)?([A-Za-z_]\w*)', code[m.end():])
                if m.group(1) in MARK_TYPES and v and not re.search(r'\btypedef\b[^;]*$', code[:m.start()]):
                    marks.vars[v.group(1)] = MARK_TYPES[m.group(1)]
                    marks.where.setdefault(v.group(1), where)
                continue
            if op in done:
                continue
            done.add(op)
            name = re.search(r'([A-Za-z_]\w*)\s*$', code[:op])
            cp = matching(code, op, '(', ')')
            if not name or name.group(1) in NOT_FUNCTIONS or cp is None:
                continue
            name = name.group(1)
            params = split_args(code[op + 1:cp])
            for pos, p in enumerate(params):
                t = MARK_TOKEN.search(p)
                if not t:
                    continue
                if t.group(1) == 'TRZ_VA_ITEMS':
                    marks.va[name] = (pos, 'item')
                else:
                    marks.calls.setdefault(name, {})[pos] = MARK_TYPES[t.group(1)]
            marks.where.setdefault(name, where)

    # macros: a parameter is marked when every use in the expansion is a marked
    # argument (in every definition that uses it); repeat until nothing changes
    changed = True
    while changed:
        changed = False
        marked = re.compile(r'\b(%s)\s*\(|\b(%s)\b' % ('|'.join(marks.calls), '|'.join(marks.vars) or '$^'))
        for name, defs in macros.items():
            if not any(marked.search(body) for _, body, _ in defs):
                continue  # does not expand to anything marked
            votes = collections.defaultdict(set)
            for params, body, where in defs:
                for pos, p in enumerate(params):
                    if re.match(r'[A-Za-z_]\w*$', p) and re.search(r'\b%s\b' % p, body):
                        votes[pos].add(classify_use(p, body, marks, macro_param=True))
            for pos, v in votes.items():
                kind = 'item' if v == {'item'} or v == {'item', 'text'} else 'text' if v == {'text'} else None
                if kind and marks.calls.get(name, {}).get(pos) != kind:
                    marks.calls.setdefault(name, {})[pos] = kind
                    marks.where.setdefault(name, defs[0][2] + ' (macro)')
                    changed = True

    # check the definitions of the marked functions: a marked parameter may only
    # be passed on to marked arguments/variables, tested, or decoded with trzResolve()
    names = re.compile(r'\b(%s)\s*\(' % '|'.join(sorted(marks.calls, key=len, reverse=True)))
    for rel, code in files:
        for m in names.finditer(code):
            name, op = m.group(1), m.end() - 1
            cp = matching(code, op, '(', ')')
            body = cp is not None and re.match(r'\s*(?:const\s*)?\{', code[cp + 1:])
            if not body:
                continue
            start = cp + 1 + body.end()
            end = matching(code, start - 1, '{', '}') or len(code)
            text = code[start:end]
            params = split_args(code[op + 1:cp])
            for pos in marks.calls[name]:
                pname = re.search(r'([A-Za-z_]\w*)\s*(=.*)?$', params[pos]) if pos < len(params) else None
                if not pname:
                    continue
                if not re.search(r'\bchar\b|\btrz_', params[pos]):
                    continue  # another function with the same name (no string at pos)
                pname = pname.group(1)
                marks.checked.append('%s:%d %s(%s)' % (rel, code.count('\n', 0, op) + 1, name, pname))
                if re.search(r'\btrzResolve\s*\(\s*%s\b' % pname, text):
                    marks.checked[-1] += ' decoder'
                    continue  # the decoder itself
                for i, line in enumerate(text.split('\n')):
                    if not re.search(r'\b%s\b' % pname, line):
                        continue
                    test = null_tests(pname, line)
                    r = classify_use(pname, test, marks)
                    if r not in ('text', 'item', 'compare'):
                        marks.warnings.append('%s:%d %s(): parameter %s %s' % (
                            rel, code.count('\n', 0, start) + i + 1, name, pname, r))
    return marks


# ----------------------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--elf', required=True)
    ap.add_argument('--src', required=True, help='radio/src directory')
    ap.add_argument('--out', required=True)
    ap.add_argument('--mincnt', type=int, default=3, help='minimum pair count for a BPE token')
    ap.add_argument('--ignore-marks', action='store_true', help='do not fail on wrong marks (debugging)')
    args = ap.parse_args()

    hdr = open(os.path.join(args.src, 'translations.h'), encoding='utf-8').read()
    maxlen = int(re.search(r'#define\s+TRZ_MAXLEN\s+(\d+)', hdr).group(1))
    cpp = open(os.path.join(args.src, 'translations.cpp'), encoding='utf-8').read()
    defined = set(re.findall(r'^\s*const char (STR_[A-Z0-9_]+)\[\]\s*=', cpp, re.M))

    live = read_elf_strings(args.elf)
    uses, arith = scan_uses(args.src)
    marks = scan_marks(args.src)
    if marks.warnings and not args.ignore_marks:
        # a function that takes a trz_text/trz_item but does not only draw it
        # would show garbage: fix the function or drop the mark
        sys.exit('trz.py: error: wrong trz_text/trz_item marks:\n  ' + '\n  '.join(marks.warnings))

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
                r = classify_use(name, line, marks)
                if r in ('compare', 'item'):
                    compared.add(name)
                elif r != 'text':
                    reason = r
                    break
        if reason:
            excluded[name] = reason
        else:
            elig[name] = raw[:-1]
    popup = {n for n in elig if n in compared}

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
    rep += ['', '[text arguments (trz_text/trz_item marks, 0-based positions)]'] + marks.describe()
    rep += ['', '[checked definitions]'] + ['  ' + c for c in marks.checked]
    rep += ['', '[mark warnings]'] + ['  ' + w for w in marks.warnings]
    write_if_changed(os.path.join(args.out, 'trz_report.txt'), '\n'.join(rep) + '\n')
    print('TRZ: %d strings, %d B -> %d B blob + 256 B dict' % (len(elig), raw_size, len(blob)))


if __name__ == '__main__':
    main()
